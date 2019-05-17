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

constexpr uint64_t defaultClusterID = 128;

constexpr char addresses[3][16] = {
  "localhost:63001",
  "localhost:63002",
  "localhost:63003",
};

int main(int argc, char **argv, char **env)
{

}