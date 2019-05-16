//
// Created by jason on 2019/5/16.
//

#include <iostream>
#include <cstring>
#include <sstream>
#include <cassert>
#include "statemachines.h"
#include "utils.h"

uint64_t KVStoreStateMachine::update(
  const dragonboat::Byte *data,
  size_t size) noexcept
{
  std::string query(reinterpret_cast<const char *>(data), size);
  auto parts = split(query);
  if (parts[0] == "set") {
    kvstore_[parts[1]] = parts[2];
  } else if (parts[0] == "del") {
    kvstore_.erase(parts[1]);
  } else if (parts[0] == "clr") {
    kvstore_.clear();
  } else {
    return update_count_;
  }
  update_count_++;
  return update_count_;
}

LookupResult KVStoreStateMachine::lookup(
  const dragonboat::Byte *data,
  size_t size) const noexcept
{
  LookupResult r;
  std::string query(reinterpret_cast<const char *>(data), size);
  std::string result;
  auto it = kvstore_.find(query);
  if (it == kvstore_.end()) {
    char nf[] = "not found";
    r.result = new char[sizeof(nf)];
    r.size = sizeof(nf);
    std::memcpy(r.result, nf, r.size);
  } else {
    r.result = new char[it->second.size()];
    r.size = it->second.size();
    std::memcpy(r.result, it->second.data(), r.size);
  }
  return r;
}

uint64_t KVStoreStateMachine::getHash() const noexcept
{
  return static_cast<uint64_t>(update_count_);
}

SnapshotResult KVStoreStateMachine::saveSnapshot(
  dragonboat::SnapshotWriter *writer,
  dragonboat::SnapshotFileCollection *collection,
  const dragonboat::DoneChan &done) const noexcept
{
  SnapshotResult r;
//  TODO
//  dragonboat::IOResult ret;
//  r.error = SNAPSHOT_OK;
//  r.size = 0;
//  ret = writer->Write(
//    reinterpret_cast<const dragonboat::Byte *>(&update_count_),
//    sizeof(int));
//  if (ret.size != sizeof(int)) {
//    r.error = FAILED_TO_SAVE_SNAPSHOT;
//    return r;
//  }
//  r.size = sizeof(int);
  return r;
}

int KVStoreStateMachine::recoverFromSnapshot(
  dragonboat::SnapshotReader *reader,
  const std::vector<dragonboat::SnapshotFile> &files,
  const dragonboat::DoneChan &done) noexcept
{
//  TODO
//  dragonboat::IOResult ret;
//  dragonboat::Byte data[sizeof(int)];
//  ret = reader->Read(data, sizeof(int));
//  if (ret.size != sizeof(int)) {
//    return FAILED_TO_RECOVER_FROM_SNAPSHOT;
//  }
//  std::memcpy(&update_count_, data, sizeof(int));
  return SNAPSHOT_OK;
}

void KVStoreStateMachine::freeLookupResult(LookupResult r) noexcept
{
  delete[] r.result;
}

CPPStateMachine *createDragonboatStateMachine(
  uint64_t clusterID,
  uint64_t nodeID)
{
  auto cppsm = new CPPStateMachine;
  cppsm->sm = new KVStoreStateMachine(clusterID, nodeID);
  return cppsm;
}