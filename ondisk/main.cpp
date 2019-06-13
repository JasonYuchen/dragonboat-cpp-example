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

#include <iostream>
#include <string>
#include <sstream>
#include <getopt.h>
#include <dragonboat/dragonboat.h>
#include "statemachine.h"
#include "utils.h"

constexpr uint64_t ClusterID = 128;

enum RequestType : int {
  PUT = 0,
  GET = 1,
};

const std::string addresses[] = {
  "localhost:63001",
  "localhost:63002",
  "localhost:63003",
};

void printUsage()
{
  std::cout
    << "Usage - \n"
    << "put key value\n"
    << "get key\n"
    << "exit" << std::endl;
}

int main(int argc, char **argv, char **env)
{
  // TODO: use getopt
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

  dragonboat::Config config(ClusterID, nodeID);
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

  dragonboat::Status status;
  std::unique_ptr<dragonboat::NodeHost> nh(new dragonboat::NodeHost(nhconfig));
  status = nh->StartCluster(
    peers, join,
    [](uint64_t clusterID, uint64_t nodeID)
    {
      return new DiskKV(clusterID, nodeID);
    }, config);
  if (!status.OK()) {
    std::cerr << "failed to StartCluster: " << status.Code() << std::endl;
    return -1;
  }

  auto timeout = dragonboat::Milliseconds(3000);
  for (std::string message; std::getline(std::cin, message);) {
    auto parts = split(message);
    if (parts.size() < 1 || parts.size() > 3) {
      std::cerr << "undefined command: " << message << std::endl;
      continue;
    }
    if (parts.size() == 3 && parts[0] != "put") {
      std::cerr << "Usage: put key value" << std::endl;
      continue;
    }
    if (parts.size() == 2 && parts[0] != "get") {
      std::cerr << "Usage: get key" << std::endl;
      continue;
    }
    if (parts[0] == "exit") {
      break;
    }
    dragonboat::Buffer result(1024);
    switch (parts.size()) {
      case 2: {
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(parts[1].c_str()),
          parts[1].size());
        status = nh->SyncRead(ClusterID, query, &result, timeout);
        break;
      }
      case 3: {
        auto msg = parts[1] + " " + parts[2];
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(msg.c_str()),
          msg.size());
        std::unique_ptr<dragonboat::Session> session(
          nh->GetNoOPSession(ClusterID));
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