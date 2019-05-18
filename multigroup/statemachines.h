//
// Created by jason on 2019/5/16.
//

#ifndef DRAGONBOAT_CPP_EXAMPLE_MULTIGROUP_STATEMACHINES_H_
#define DRAGONBOAT_CPP_EXAMPLE_MULTIGROUP_STATEMACHINES_H_

#include "dragonboat/statemachine.h"
#include <vector>
#include <unordered_map>

class KVStoreStateMachine : public dragonboat::StateMachine {
 public:
  KVStoreStateMachine(uint64_t clusterID, uint64_t nodeID) noexcept
    : StateMachine(clusterID, nodeID), update_count_(0), kvstore_()
  {}
  ~KVStoreStateMachine() noexcept override = default;
 protected:
  uint64_t update(const dragonboat::Byte *data, size_t size) noexcept override;
  LookupResult lookup(
    const dragonboat::Byte *data,
    size_t size) const noexcept override;
  uint64_t getHash() const noexcept override;
  SnapshotResult saveSnapshot(
    dragonboat::SnapshotWriter *writer,
    dragonboat::SnapshotFileCollection *collection,
    const dragonboat::DoneChan &done) const noexcept override;
  int recoverFromSnapshot(
    dragonboat::SnapshotReader *reader,
    const std::vector<dragonboat::SnapshotFile> &files,
    const dragonboat::DoneChan &done) noexcept override;
  void freeLookupResult(LookupResult r) noexcept override;
 private:
  DISALLOW_COPY_MOVE_AND_ASSIGN(KVStoreStateMachine);
  int update_count_;
  std::unordered_map<std::string, std::string> kvstore_;
};

CPPStateMachine *createDragonboatStateMachine(
  uint64_t clusterID,
  uint64_t nodeID);

#endif //DRAGONBOAT_CPP_EXAMPLE_MULTIGROUP_STATEMACHINES_H_
