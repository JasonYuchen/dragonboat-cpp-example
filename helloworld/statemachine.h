//
// Created by jason on 2019/5/13.
//

#ifndef DRAGONBOAT_CPP_EXAMPLE_STATEMACHINE_H
#define DRAGONBOAT_CPP_EXAMPLE_STATEMACHINE_H

#include "dragonboat/statemachine.h"
#include <vector>

// HelloWorldStateMachine is an example CPP StateMachine. It shows how to implement
// a StateMachine with your own application logic and interact with the rest of
// the Dragonboat system.
// Basically, the logic is simple - this data store increases the update_count_
// member variable for each incoming update request no matter what is in the
// update request. Lookup requests always return the integer value stored in
// update_count_, same as the getHash method.
//
// See statemachine.h for more details about the StateMachine interface.
class HelloWorldStateMachine : public dragonboat::StateMachine {
 public:
  HelloWorldStateMachine(uint64_t clusterID, uint64_t nodeID) noexcept;
  ~HelloWorldStateMachine();
 protected:
  uint64_t update(const dragonboat::Byte *data, size_t size) noexcept override;
  LookupResult lookup(const dragonboat::Byte *data,
                      size_t size) const noexcept override;
  uint64_t getHash() const noexcept override;
  SnapshotResult saveSnapshot(dragonboat::SnapshotWriter *writer,
                              dragonboat::SnapshotFileCollection *collection,
                              const dragonboat::DoneChan &done) const noexcept override;
  int recoverFromSnapshot(dragonboat::SnapshotReader *reader,
                          const std::vector<dragonboat::SnapshotFile> &files,
                          const dragonboat::DoneChan &done) noexcept override;
  void freeLookupResult(LookupResult r) noexcept override;
 private:
  DISALLOW_COPY_MOVE_AND_ASSIGN(HelloWorldStateMachine);
  int update_count_;
};

#endif //DRAGONBOAT_CPP_EXAMPLE_STATEMACHINE_H
