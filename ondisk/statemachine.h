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

#ifndef DRAGONBOAT_CPP_EXAMPLE_ONDISK_STATEMACHINE_H_
#define DRAGONBOAT_CPP_EXAMPLE_ONDISK_STATEMACHINE_H_

#include <mutex>
#include <rocksdb/db.h>
#include <dragonboat/statemachine/ondisk.h>

const std::string appliedIndexKey = "disk_kv_applied_index";
const std::string testDBDirName = "example-data";
const std::string currentDBFilename = "current";
const std::string updatingDBFilename = "current.updating";

struct RocksDB {
  std::unique_ptr<rocksdb::DB> db_;
  rocksdb::Options opts_;
  rocksdb::ReadOptions ro_;
  rocksdb::WriteOptions wo_;
  ~RocksDB();
};

// lookup/saveSnapshot/update/reocoverFromSnapshot can be concurrently invoked
// update/prepareSnapshot can not be concurrently invoked
class DiskKV : public dragonboat::OnDiskStateMachine {
 public:
  DiskKV(uint64_t clusterID, uint64_t nodeID) noexcept;
  ~DiskKV() override;
 protected:
  OpenResult open(const dragonboat::DoneChan &done) noexcept override;
  void update(dragonboat::Entry &ent) noexcept override;
  void batchedUpdate(std::vector<dragonboat::Entry> &ents) noexcept override;
  LookupResult lookup(
    const dragonboat::Byte *data,
    size_t size) const noexcept override;
  int sync() const noexcept override;
  uint64_t getHash() const noexcept override;
  PrepareSnapshotResult prepareSnapshot() const noexcept override;
  SnapshotResult saveSnapshot(
    const void *context,
    dragonboat::SnapshotWriter *writer,
    const dragonboat::DoneChan &done) const noexcept override;
  int recoverFromSnapshot(
    dragonboat::SnapshotReader *reader,
    const dragonboat::DoneChan &done) noexcept override;
  void freeLookupResult(LookupResult r) noexcept override;
 private:
  std::shared_ptr<RocksDB> createDB(std::string dbdir);
  uint64_t queryAppliedIndex(RocksDB *db) const;
  static bool isNewRun(std::string dir) noexcept;
  static std::string getNodeDBDirName(
    uint64_t clusterID,
    uint64_t nodeID) noexcept;
  static std::string getNewRandomDBDirName(std::string dir) noexcept;
  static void replaceCurrentDBFile(std::string dir);
  static void saveCurrentDBDirName(std::string dir, std::string dbdir);
  static std::string getCurrentDBDirName(std::string dir);
  static void createNodeDataDir(std::string dir);
  static void cleanupNodeDataDir(std::string dir);
 private:
  DISALLOW_COPY_MOVE_AND_ASSIGN(DiskKV);
  mutable std::mutex mtx_;
  std::shared_ptr<RocksDB> rocks_;
  uint64_t lastApplied_;
};

#endif //DRAGONBOAT_CPP_EXAMPLE_ONDISK_STATEMACHINE_H_
