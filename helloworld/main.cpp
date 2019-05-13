#include <iostream>
#include <sstream>
#include <memory>
#include <atomic>
#include <thread>
#include "dragonboat/dragonboat.h"
#include "statemachine.h"

constexpr uint64_t defaultClusterID = 128;

constexpr char addresses[3][16] = {
  "localhost:63001",
  "localhost:63002",
  "localhost:63003",
};

int main(int argc, char **argv, char **env)
{
  uint64_t nodeID;
  if (argc == 2) {
    nodeID = std::stoi(argv[1]);
    if (nodeID > 3 || nodeID < 1) {
      std::cerr << "invalid node id: " << nodeID << std::endl;
      return -1;
    }
  } else {
    std::cerr << "Usage: helloworld nodeID" << std::endl;
    return -1;
  }

  dragonboat::Config config(defaultClusterID, nodeID);
  config.ElectionRTT = 5;
  config.HeartbeatRTT = 1;
  config.CheckQuorum = true;
  config.SnapshotEntries = 10;
  config.CompactionOverhead = 5;

  dragonboat::Peers peers;
  for (auto idx = 0; idx < 3; ++idx) {
    peers.AddMember(addresses[idx], idx + 1);
  }

  std::stringstream path;
  path << "example-data/helloworld-data/node" << nodeID;
  dragonboat::NodeHostConfig nhconfig(path.str(), path.str());
  nhconfig.RTTMillisecond = dragonboat::Milliseconds(200);
  nhconfig.RaftAddress = addresses[nodeID - 1];
  nhconfig.APIAddress = "";

  dragonboat::Status status;
  std::unique_ptr<dragonboat::NodeHost> nh(new dragonboat::NodeHost(nhconfig));
  status = nh->StartCluster(peers, false, createDragonboatStateMachine, config);
  if (!status.OK()) {
    std::cerr << "failed to StartCluster: " << status.Code() << std::endl;
    return -1;
  }
  std::atomic_bool readyToExit(false);
  auto timeout = dragonboat::Milliseconds(3000);
  auto readThread = std::thread(
    [&nh, &readyToExit, &timeout]()
    {
      size_t count = 0;
      dragonboat::Buffer query(1);
      dragonboat::Buffer result(sizeof(int));
      while (true) {
        if (count == 100) {
          count = 0;
          auto rs = nh->SyncRead(defaultClusterID, query, &result, timeout);
          if (rs.OK()) {
            auto c = result.Data();
            std::cout << "count: " << *reinterpret_cast<const int *>(c)
                      << std::endl;
          }
        } else {
          count++;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (readyToExit.load()) {
          return;
        }
      }
    });
  std::unique_ptr<dragonboat::Session> session(
    nh->GetNoOPSession(defaultClusterID));
  for (std::string message; std::getline(std::cin, message);) {
    if (message == "exit") {
      readyToExit = true;
      break;
    }
    dragonboat::UpdateResult result;
    dragonboat::Buffer buf(
      reinterpret_cast<const dragonboat::Byte *>(message.c_str()),
      message.size());
    status = nh->SyncPropose(session.get(), buf, timeout, &result);
    if (!status.OK()) {
      std::cerr << "failed to make proposal: " << message << ", error code: "
                << status.Code() << std::endl;
    }
  }
  readThread.join();
  nh->Stop();
  return 0;
}