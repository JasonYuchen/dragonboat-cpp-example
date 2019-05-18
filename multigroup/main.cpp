#include <getopt.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <atomic>
#include <thread>
#include <algorithm>
#include "dragonboat/dragonboat.h"
#include "statemachines.h"
#include "utils.h"

constexpr uint64_t ClusterID1 = 1;
constexpr uint64_t ClusterID2 = 2;

constexpr char addresses[3][16] = {
  "localhost:63001",
  "localhost:63002",
  "localhost:63003",
};

int main(int argc, char **argv, char **env)
{
  int ret;
  uint64_t nodeID = 0;
  bool join = false;
  std::string address;
  // for simplicity, membership change is removed in this example
  struct ::option opts[] = {
    {"nodeid", required_argument, nullptr, 0},
  };

  while ((ret = getopt_long_only(argc, argv, "", opts, nullptr)) != -1) {
    switch (ret) {
      case 0:nodeID = std::stoull(optarg);
        break;
      default:std::cerr << "unknown ret " << ret << std::endl;
    }
  }

  if (nodeID < 1 || nodeID > 3) {
    std::cerr << "invalid node id: " << nodeID << std::endl;
    return -1;
  } else if (nodeID <= 3) {
    address = addresses[nodeID - 1];
  }

  dragonboat::Config config(ClusterID1, nodeID);
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
  path << "example-data/multigroup-data/node" << nodeID;
  dragonboat::NodeHostConfig nhconfig(path.str(), path.str());
  nhconfig.RTTMillisecond = dragonboat::Milliseconds(200);
  nhconfig.RaftAddress = address;
  nhconfig.APIAddress = "";

  dragonboat::Status status;
  std::unique_ptr<dragonboat::NodeHost> nh(new dragonboat::NodeHost(nhconfig));
  status = nh->StartCluster(peers, join, createDragonboatStateMachine, config);
  if (!status.OK()) {
    std::cerr << "failed to StartCluster: " << status.Code() << std::endl;
    return -1;
  }

  config.ClusterId = ClusterID2;
  status = nh->StartCluster(peers, join, createDragonboatStateMachine, config);
  if (!status.OK()) {
    std::cerr << "failed to StartCluster: " << status.Code() << std::endl;
    return -1;
  }

  // supported command:
  // set key value
  // key
  // display clusterID
  auto timeout = dragonboat::Milliseconds(3000);
  for (std::string message; std::getline(std::cin, message);) {
    auto parts = split(message);
    if (parts.size() < 1 || parts.size() > 3) {
      std::cerr << "undefined command: " << message << std::endl;
      continue;
    }
    if (parts.size() == 3 && parts[0] != "set") {
      std::cerr << "Usage: set key value" << std::endl;
      continue;
    }
    if (parts.size() == 2 && parts[0] != "display") {
      std::cerr << "Usage: display clusterID" << std::endl;
      continue;
    }
    if (parts[0] == "exit") {
      break;
    }
    dragonboat::Buffer result(1024);
    switch (parts.size()) {
      case 1: {
        auto clusterID = std::hash<std::string>()(parts[0]) % 2 + ClusterID1;
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(parts[0].c_str()),
          parts[0].size());
        status = nh->SyncRead(clusterID, query, &result, timeout);
        break;
      }
      case 2: {
        auto clusterID = std::stoi(parts[1]);
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(parts[0].c_str()),
          parts[0].size());
        status = nh->SyncRead(clusterID, query, &result, timeout);
        break;
      }
      case 3: {
        auto clusterID = std::hash<std::string>()(parts[1]) % 2 + ClusterID1;
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(message.c_str()),
          message.size());
        std::unique_ptr<dragonboat::Session> session(
          nh->GetNoOPSession(clusterID));
        dragonboat::UpdateResult ret;
        status = nh->SyncPropose(session.get(), query, timeout, &ret);
        break;
      }
      default:std::cerr << "error" << std::endl;
    }
    if (status.OK() && result.Len() != 0) {
      std::cout << result.Data() << std::endl;
    } else if (!status.OK()) {
      std::cerr << "error code: " << status.Code() << std::endl;
    }
  }
  nh->Stop();
}