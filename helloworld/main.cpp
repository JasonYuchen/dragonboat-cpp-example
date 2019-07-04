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

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include "dragonboat/dragonboat.h"
#include "statemachine.h"
#include "utils.h"

constexpr uint64_t defaultClusterID = 128;

constexpr char addresses[3][16] = {
  "localhost:63001",
  "localhost:63002",
  "localhost:63003",
};

class ProposeComplete : public dragonboat::Event {
 public:
  ProposeComplete() noexcept : set_(false)
  {}
  void Wait() noexcept
  {
    std::unique_lock<std::mutex> lk(mtx_);
    while (!set_) {
      cv_.wait(
        lk, [this]()
        { return set_; });
    }
  }
 protected:
  void set() noexcept override
  {
    std::unique_lock<std::mutex> lk(mtx_);
    set_ = true;
    cv_.notify_all();
  }
 private:
  bool set_;
  std::condition_variable cv_;
  std::mutex mtx_;
};

int main(int argc, char **argv, char **env)
{
  int ret;
  uint64_t nodeID = 0;
  bool join = false;
  std::string address;
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

  dragonboat::Config config(defaultClusterID, nodeID);
  config.ElectionRTT = 5;
  config.HeartbeatRTT = 1;
  config.CheckQuorum = true;
  config.SnapshotEntries = 10;
  config.CompactionOverhead = 5;

  dragonboat::Peers peers;
  if (!join) {
    for (auto idx = 0; idx < 3; ++idx) {
      peers.AddMember(addresses[idx], idx + 1);
    }
  }

  std::stringstream path;
  path << "example-data/helloworld-data/node" << nodeID;
  dragonboat::NodeHostConfig nhconfig(path.str(), path.str());
  nhconfig.RTTMillisecond = dragonboat::Milliseconds(200);
  nhconfig.RaftAddress = address;

  dragonboat::Status status;
  std::unique_ptr<dragonboat::NodeHost> nh(new dragonboat::NodeHost(nhconfig));
  status = nh->StartCluster(peers, join, createDragonboatStateMachine, config);
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
  auto statusAssert = [](const std::string &desc, dragonboat::Status &s)
  {
    if (!s.OK()) {
      std::cerr << desc << " failed, error code: " << s.Code() << std::endl;
    }
  };
  for (std::string message; std::getline(std::cin, message);) {
    auto parts = split(message);
    if (parts[0] == "exit") {
      readyToExit = true;
      break;
    } else if (parts[0] == "add") {
      auto addr = parts[1];
      auto id = std::stoi(parts[2]);
      status = nh->SyncRequestAddNode(defaultClusterID, id, addr, timeout);
      statusAssert("Add node " + parts[1], status);
    } else if (parts[0] == "remove") {
      auto id = std::stoi(parts[1]);
      status = nh->SyncRequestDeleteNode(defaultClusterID, id, timeout);
      statusAssert("Remove node " + parts[1], status);
    } else {
      dragonboat::UpdateResult result;
      dragonboat::Buffer buf(
        reinterpret_cast<const dragonboat::Byte *>(parts[0].c_str()),
        parts[0].size());
      if (parts[0].size() % 2) {
        status = nh->SyncPropose(session.get(), buf, timeout, &result);
        statusAssert("SyncPropose " + parts[0], status);
      } else {
        ProposeComplete pc;
        status = nh->Propose(session.get(), buf, timeout, &pc);
        statusAssert("AsyncPropose " + parts[0], status);
        pc.Wait();
        result = pc.Get().result;
        ResultCode resultCode = pc.Get().code;
        std::cout
          << "Result for AsyncPropose: "
          << static_cast<int>(resultCode)
          << std::endl;
      }
    }
  }
  readThread.join();
  nh->Stop();
  return 0;
}