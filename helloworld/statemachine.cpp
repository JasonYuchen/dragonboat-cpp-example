//
// Created by jason on 2019/5/13.
//

#include <iostream>
#include <cstring>
#include "statemachine.h"

uint64_t HelloWorldStateMachine::update(
  const dragonboat::Byte *data,
  size_t size) noexcept
{
  auto c = reinterpret_cast<const char *>(data);
  std::cout << "message: " << std::string(c, size) << std::endl;
  update_count_++;
  return update_count_;
}

LookupResult HelloWorldStateMachine::lookup(
  const dragonboat::Byte *data,
  size_t size) const noexcept
{
  LookupResult r;
  r.result = new char[sizeof(int)];
  r.size = sizeof(int);
  std::memcpy(r.result, &update_count_, sizeof(int));
  return r;
}

uint64_t HelloWorldStateMachine::getHash() const noexcept
{
  return static_cast<uint64_t>(update_count_);
}

SnapshotResult HelloWorldStateMachine::saveSnapshot(
  dragonboat::SnapshotWriter *writer,
  dragonboat::SnapshotFileCollection *collection,
  const dragonboat::DoneChan &done) const noexcept
{
  SnapshotResult r;
  dragonboat::IOResult ret;
  r.error = SNAPSHOT_OK;
  r.size = 0;
  ret = writer->Write(
    reinterpret_cast<const dragonboat::Byte *>(&update_count_),
    sizeof(int));
  if (ret.size != sizeof(int)) {
    r.error = FAILED_TO_SAVE_SNAPSHOT;
    return r;
  }
  r.size = sizeof(int);
  return r;
}

int HelloWorldStateMachine::recoverFromSnapshot(
  dragonboat::SnapshotReader *reader,
  const std::vector<dragonboat::SnapshotFile> &files,
  const dragonboat::DoneChan &done) noexcept
{
  dragonboat::IOResult ret;
  dragonboat::Byte data[sizeof(int)];
  ret = reader->Read(data, sizeof(int));
  if (ret.size != sizeof(int)) {
    return FAILED_TO_RECOVER_FROM_SNAPSHOT;
  }
  std::memcpy(&update_count_, data, sizeof(int));
  return SNAPSHOT_OK;
}

void HelloWorldStateMachine::freeLookupResult(LookupResult r) noexcept
{
  delete[] r.result;
}

CPPStateMachine *createDragonboatStateMachine(
  uint64_t clusterID,
  uint64_t nodeID)
{
  auto cppsm = new CPPStateMachine;
  cppsm->sm = new HelloWorldStateMachine(clusterID, nodeID);
  return cppsm;
}