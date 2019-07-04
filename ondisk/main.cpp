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
#include <cassert>
#include <getopt.h>
#include <dragonboat/dragonboat.h>
#include "zupply.hpp"
#include "statemachine.h"

constexpr uint64_t ClusterID = 128;

enum RequestType : int {
  EXIT = 0,
  PUT = 1,
  GET = 2,
  ADD_NODE = 3,
  REMOVE_NODE = 4,
  UNKNOWN,
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

std::tuple<RequestType, std::string, std::string> parseRequest(std::string &msg)
{
  auto parts = zz::fmt::split(msg);
  if (parts.empty() || parts.size() > 3) {
    return {UNKNOWN, "", ""};
  } else if (zz::fmt::to_lower_ascii(parts[0]) == "exit") {
    return {EXIT, "", ""};
  } else if (zz::fmt::to_lower_ascii(parts[0]) == "put") {
    assert(parts.size() == 3);
    return {PUT, std::move(parts[1]), std::move(parts[2])};
  } else if (zz::fmt::to_lower_ascii(parts[0]) == "get") {
    return {GET, std::move(parts[1]), ""};
  } else if (zz::fmt::to_lower_ascii(parts[0]) == "add") {
    return {ADD_NODE, std::move(parts[1]), std::move(parts[2])};
  } else if (zz::fmt::to_lower_ascii(parts[0]) == "remove") {
    return {REMOVE_NODE, std::move(parts[1]), ""};
  } else {
    return {UNKNOWN, "", ""};
  }
}

int main(int argc, char **argv, char **env)
{
  int ret;
  uint64_t nodeID = 0;
  bool join = false;
  std::string address;
  // for simplicity, membership change is removed in this example
  struct ::option opts[] = {
    {"nodeid", required_argument, nullptr, 0},
    {"addr", required_argument, nullptr, 1},
    {"join", no_argument, nullptr, 2},
  };

  while ((ret = getopt_long_only(argc, argv, "", opts, nullptr)) != -1) {
    switch (ret) {
      case 0:nodeID = std::stoull(optarg);
        break;
      case 1:address = optarg;
        break;
      case 2:join = true;
        break;
      default:std::cerr << "unknown ret " << ret << std::endl;
        break;
    }
  }

  if (nodeID < 1) {
    std::cerr << "invalid node id: " << nodeID << std::endl;
    return -1;
  } else if (nodeID > 3 && address.empty()) {
    std::cerr << "undefined node address" << std::endl;
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
  path << "example-data/ondisk-data/node" << nodeID;
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

  printUsage();
  auto statusAssert = [](const std::string &desc, dragonboat::Status &s)
  {
    if (!s.OK()) {
      std::cerr
        << desc << " failed, error("
        << s.Code() << "): " << s.String() << std::endl;
    }
  };
  auto timeout = dragonboat::Milliseconds(3000);
  std::unique_ptr<dragonboat::Session> session(nh->GetNoOPSession(ClusterID));
  dragonboat::Buffer result(1024);
  for (std::string message; std::getline(std::cin, message);) {
    auto request = parseRequest(message);
    auto &type = std::get<0>(request);
    auto &key = std::get<1>(request);
    auto &value = std::get<2>(request);
    switch (type) {
      case EXIT: {
        break;
      }
      case PUT: {
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(key.c_str()),
          key.size());
        dragonboat::UpdateResult ret;
        status = nh->SyncPropose(session.get(), query, timeout, &ret);
        break;
      }
      case GET: {
        dragonboat::Buffer query(
          reinterpret_cast<const dragonboat::Byte *>(key.c_str()),
          key.size());
        status = nh->SyncRead(ClusterID, query, &result, timeout);
        break;
      }
      case ADD_NODE: {
        status =
          nh->SyncRequestAddNode(ClusterID, std::stoi(value), key, timeout);
        break;
      }
      case REMOVE_NODE: {
        status = nh->SyncRequestDeleteNode(ClusterID, std::stoi(key), timeout);
        break;
      }
      case UNKNOWN: {
        printUsage();
        break;
      }
      default: {
        std::cerr << "Fatal" << std::endl;
        break;
      }
    }
    statusAssert(message, status);
  }
  nh->Stop();
}