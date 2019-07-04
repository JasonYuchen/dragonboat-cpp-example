// Copyright 2019 JasonYuchen (jasonyuchen@foxmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <random>
#include <cstring>
#include <rocksdb/db.h>
#include "statemachine.h"
#include "zupply.hpp"

RocksDB::~RocksDB()
{
  if (db_) {
    db_->Close();
  }
}

DiskKV::DiskKV(uint64_t clusterID, uint64_t nodeID) noexcept
  : dragonboat::OnDiskStateMachine(clusterID, nodeID)
{
}

DiskKV::~DiskKV()
{
}

OpenResult DiskKV::open(const dragonboat::DoneChan &done) noexcept
{
  OpenResult r;
  auto dir = getNodeDBDirName(cluster_id_, node_id_);
  createNodeDataDir(dir);
  std::string dbdir;
  if (!isNewRun(dir)) {
    cleanupNodeDataDir(dir);
    dbdir = getCurrentDBDirName(dir);
    if (!zz::os::is_directory(dbdir)) {
      std::cerr << "db dir unexpectedly deleted" << std::endl;
      r.result = 0;
      r.errcode = -1;
      return r;
    }
  } else {
    dbdir = getNewRandomDBDirName(dir);
    saveCurrentDBDirName(dir, dbdir);
    replaceCurrentDBFile(dir);
  }
  auto rocks = createDB(dbdir);
  {
    std::lock_guard<std::mutex> guard(mtx_);
    rocks_.swap(rocks);
  }
  lastApplied_ = queryAppliedIndex(rocks_.get());
  r.result = lastApplied_;
  r.errcode = 0;
  return r;
}

void DiskKV::batchedUpdate(std::vector<dragonboat::Entry> &ents) noexcept
{
  std::shared_ptr<RocksDB> rocks;
  {
    std::lock_guard<std::mutex> guard(mtx_);
    rocks = rocks_;
  }
  auto wb = rocksdb::WriteBatch();
  for (auto &ent : ents) {
    std::stringstream ss({reinterpret_cast<const char *>(ent.cmd), ent.cmdLen});
    std::string key, value;
    ss >> key >> value;
    wb.Put(key, value);
    ent.result = ent.index;
  }
  wb.Put(appliedIndexKey, std::to_string(ents.back().index));
  auto s = rocks->db_->Write(rocks->wo_, &wb);
  if (!s.ok()) {
    std::cerr << "failed to update: " << s.ToString() << std::endl;
  }
}

LookupResult DiskKV::lookup(
  const dragonboat::Byte *data,
  size_t size) const noexcept
{
  std::shared_ptr<RocksDB> rocks;
  {
    std::lock_guard<std::mutex> guard(mtx_);
    rocks = rocks_;
  }
  std::string value;
  auto s = rocks->db_->Get(
    rocks->ro_,
    {reinterpret_cast<const char *>(data), size},
    &value);
  LookupResult r;
  if (!s.ok()) {
    std::cerr << "failed to lookup: " << s.ToString() << std::endl;
    r.result = nullptr;
    r.size = 0;
    return r;
  }
  if (value.empty()) {
    r.result = nullptr;
    r.size = 0;
  } else {
    r.size = value.length();
    r.result = new char[r.size];
    memcpy(r.result, value.data(), r.size);
  }
  return r;
}

int DiskKV::sync() const noexcept
{
  // FIXME: use fsync here
  return 0;
}

uint64_t DiskKV::getHash() const noexcept
{
  // FIXME: use MD5 and rocksdb::Snapshot
  return 0;
}

PrepareSnapshotResult DiskKV::prepareSnapshot() const noexcept
{
  std::shared_ptr<RocksDB> rocks;
  {
    std::lock_guard<std::mutex> guard(mtx_);
    rocks = rocks_;
  }
  auto snapshotptr = rocks->db_->GetSnapshot();
  PrepareSnapshotResult r;
  r.result = (void *)snapshotptr;
  r.errcode = SNAPSHOT_OK;
  return r;
}

SnapshotResult DiskKV::saveSnapshot(
  const void *context,
  dragonboat::SnapshotWriter *writer,
  const dragonboat::DoneChan &done) const noexcept
{
  std::shared_ptr<RocksDB> rocks;
  {
    std::lock_guard<std::mutex> guard(mtx_);
    rocks = rocks_;
  }
  SnapshotResult r;
  r.size = 0;
  auto snapshotptr = reinterpret_cast<const rocksdb::Snapshot *>(context);
  auto ro = rocksdb::ReadOptions();
  ro.snapshot = snapshotptr;
  rocksdb::Iterator *iter = rocks->db_->NewIterator(ro);
  uint64_t count = 0;
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    count++;
  }
  auto ioret = writer->Write(
    reinterpret_cast<dragonboat::Byte *>(&count), sizeof(uint64_t));
  if (ioret.error != 0) {
    std::cerr
      << "failed to save snapshot: "
      << std::to_string(ioret.error) << std::endl;
    r.errcode = FAILED_TO_SAVE_SNAPSHOT;
    return r;
  }
  r.size += sizeof(uint64_t);
  // FIXME: use buffer and check IOResult
  uint64_t len;
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    auto key = iter->key();
    auto val = iter->value();
    len = key.size();
    writer->Write(reinterpret_cast<dragonboat::Byte *>(&len), sizeof(uint64_t));
    writer->Write(reinterpret_cast<const dragonboat::Byte *>(key.data()), len);
    len = val.size();
    writer->Write(reinterpret_cast<dragonboat::Byte *>(&len), sizeof(uint64_t));
    writer->Write(reinterpret_cast<const dragonboat::Byte *>(val.data()), len);
    r.size += 2 * sizeof(uint64_t) + key.size() + val.size();
  }
  r.errcode = SNAPSHOT_OK;
  delete iter;
  rocks->db_->ReleaseSnapshot(snapshotptr);
  return r;
}

int DiskKV::recoverFromSnapshot(
  dragonboat::SnapshotReader *reader,
  const dragonboat::DoneChan &done) noexcept
{
  auto dir = getNodeDBDirName(cluster_id_, node_id_);
  auto dbdir = getNewRandomDBDirName(dir);
  auto oldDirName = getCurrentDBDirName(dir);
  auto rocks = createDB(dbdir);
  uint64_t count = 0;
  auto ioret = reader->Read(
    reinterpret_cast<dragonboat::Byte *>(&count), sizeof(uint64_t));
  if (ioret.error != 0) {
    std::cerr
      << "failed to recover from snapshot: "
      << std::to_string(ioret.error) << std::endl;
    return FAILED_TO_RECOVER_FROM_SNAPSHOT;
  }
  uint64_t len = 0;
  std::vector<dragonboat::Byte> key, val;
  for (uint64_t i = 0; i < count; ++i) {
    reader->Read(reinterpret_cast<dragonboat::Byte *>(&len), sizeof(uint64_t));
    key.resize(len);
    reader->Read(key.data(), len);
    reader->Read(reinterpret_cast<dragonboat::Byte *>(&len), sizeof(uint64_t));
    val.resize(len);
    reader->Read(val.data(), len);
    rocksdb::WriteBatch wb;
    wb.Put(
      {reinterpret_cast<const char *>(key.data()), key.size()},
      {reinterpret_cast<const char *>(val.data()), val.size()});
    auto s = rocks->db_->Write(rocks->wo_, &wb);
    if (!s.ok()) {
      std::cerr
        << "failed to recover from snapshot: " << s.ToString() << std::endl;
      return FAILED_TO_RECOVER_FROM_SNAPSHOT;
    }
  }
  saveCurrentDBDirName(dir, dbdir);
  replaceCurrentDBFile(dir);
  auto newLastApplied = queryAppliedIndex(rocks.get());
  if (lastApplied_ > newLastApplied) {
    std::cerr
      << "snapshot not moving forward, lastApplied " << lastApplied_
      << ", newLastApplied " << newLastApplied << std::endl;
    return FAILED_TO_RECOVER_FROM_SNAPSHOT;
  }
  lastApplied_ = newLastApplied;
  {
    std::lock_guard<std::mutex> guard(mtx_);
    rocks_.swap(rocks);
  }
  zz::os::remove_all(oldDirName);
  return SNAPSHOT_OK;
}

void DiskKV::freeLookupResult(LookupResult r) noexcept
{
  delete[] r.result;
}

std::shared_ptr<RocksDB> DiskKV::createDB(std::string dbdir)
{
  auto rocks = std::make_shared<RocksDB>();
  rocks->opts_ = rocksdb::Options();
  rocks->opts_.create_if_missing = true;
  rocks->opts_.use_fsync = true;
  rocks->ro_ = rocksdb::ReadOptions();
  rocks->wo_ = rocksdb::WriteOptions();
  rocks->wo_.sync = true;
  rocksdb::DB *db = nullptr;
  auto s = rocksdb::DB::Open(rocks->opts_, dbdir, &db);
  if (!s.ok()) {
    throw std::runtime_error("failed to create RocksDB: " + s.ToString());
  }
  rocks->db_.reset(db);
  return rocks;
}

uint64_t DiskKV::queryAppliedIndex(RocksDB *db) const
{
  std::string data;
  rocksdb::Slice slice(appliedIndexKey.data(), appliedIndexKey.length());
  auto s = db->db_->Get(db->ro_, slice, &data);
  if (!s.ok()) {
    std::cerr << "failed to query applied index: " << s.ToString() << std::endl;
    return 0;
  }
  if (data.empty()) {
    return 0;
  }
  return std::stoull(data);
}

bool DiskKV::isNewRun(std::string dir) noexcept
{
  auto fp = zz::os::path_join({dir, currentDBFilename});
  return !zz::os::is_file(fp);
}

std::string DiskKV::getNodeDBDirName(
  uint64_t clusterID,
  uint64_t nodeID) noexcept
{
  std::stringstream ss;
  ss << clusterID << "_" << nodeID;
  return zz::os::path_join({testDBDirName, ss.str()});
}

std::string DiskKV::getNewRandomDBDirName(std::string dir) noexcept
{
  static std::mt19937_64
    generater(std::chrono::steady_clock::now().time_since_epoch().count());
  std::stringstream ss;
  ss << generater()
     << "_"
     << std::chrono::steady_clock::now().time_since_epoch().count();
  return zz::os::path_join({dir, ss.str()});
}

void DiskKV::replaceCurrentDBFile(std::string dir)
{
  auto fp = zz::os::path_join({dir, currentDBFilename});
  auto tmpFp = zz::os::path_join({dir, updatingDBFilename});
  if (!zz::os::rename(tmpFp, fp)) {
    throw std::runtime_error("failed to rename " + tmpFp + " to " + fp);
  }
}

void DiskKV::saveCurrentDBDirName(std::string dir, std::string dbdir)
{
  // FIXME: MD5 verification
  auto fp = zz::os::path_join({dir, updatingDBFilename});
  std::fstream f(fp, std::ios::out);
  if (f.bad()) {
    throw std::runtime_error("failed to save current dbdir name");
  }
  f << dbdir;
  f.sync();
  if (f.bad()) {
    throw std::runtime_error("failed to save current dbdir name");
  }
}

std::string DiskKV::getCurrentDBDirName(std::string dir)
{
  auto fp = zz::os::path_join({dir, currentDBFilename});
  std::fstream f(fp, std::ios::in);
  if (f.bad()) {
    throw std::runtime_error("failed to get current dbdir name");
  }
  std::string data;
  f >> data;
  if (f.bad()) {
    throw std::runtime_error("failed to get current dbdir name");
  }
  return data;
}

void DiskKV::createNodeDataDir(std::string dir)
{
  if (!zz::os::create_directory_recursive(dir)) {
    throw std::runtime_error("failed to create node data dir");
  }
}

void DiskKV::cleanupNodeDataDir(std::string dir)
{
  zz::os::remove_all(zz::os::path_join({dir, updatingDBFilename}));
  auto dbdir = getCurrentDBDirName(dir);
  zz::fs::Directory dir_(dir);
  for (auto &item : dir_) {
    if (!item.is_dir()) {
      continue;
    }
    auto fname = zz::os::path_split_filename(item.abs_path());
    std::cout
      << "dbdir " << dbdir << ", fi.name " << fname
      << ", dir " << dir << std::endl;
    auto toDelete = zz::os::path_join({dir, fname});
    if (toDelete != dbdir) {
      std::cout << "removing " << toDelete << std::endl;
      if (!zz::os::remove_all(toDelete)) {
        throw std::runtime_error("failed to remove " + toDelete);
      }
    }
  }
}