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

#ifndef DRAGONBOAT_CPP_EXAMPLE_ONDISK_STATEMACHINES_H_
#define DRAGONBOAT_CPP_EXAMPLE_ONDISK_STATEMACHINES_H_

#include <dragonboat/statemachine.h>

// TODO: an on-disk sharding kv store backed up by LevelDB/RocksDB
class DiskKV : public dragonboat::OnDiskStateMachine {
 public:
  DiskKV(uint64_t clusterID, uint64_t nodeID) noexcept
    : dragonboat::OnDiskStateMachine(clusterID, nodeID)
  {}
  ~DiskKV() override = default;
 protected:
  OpenResult open(const dragonboat::DoneChan &done) noexcept override;
  uint64_t update(
    const dragonboat::Byte *data,
    size_t size,
    uint64_t index) noexcept override;
  LookupResult lookup(
    const dragonboat::Byte *data,
    size_t size) const noexcept override;
  int sync() const noexcept override;
  uint64_t getHash() const noexcept override;
  PrepareSnapshotResult prepareSnapshot() const noexcept override;
  SnapshotResult saveSnapshot(
    const dragonboat::Byte *ctx,
    size_t size,
    dragonboat::SnapshotWriter *writer,
    const dragonboat::DoneChan &done) const noexcept override;
  int recoverFromSnapshot(
    dragonboat::SnapshotReader *reader,
    const dragonboat::DoneChan &done) noexcept override;
  void freePrepareSnapshotResult(PrepareSnapshotResult r) noexcept override;
  void freeLookupResult(LookupResult r) noexcept override;
 private:
  DISALLOW_COPY_MOVE_AND_ASSIGN(DiskKV);
};

#endif //DRAGONBOAT_CPP_EXAMPLE_ONDISK_STATEMACHINES_H_
