//
// Created by jason on 2019/5/16.
//

#include <algorithm>
#include <type_traits>
#include "utils.h"

std::vector<std::string> split(const std::string &cmd, const char &delim)
{
  std::vector<std::string> parts(1);
  std::for_each(
    cmd.cbegin(), cmd.cend(), [&parts, &delim](const char &ch)
    {
      if (ch == delim) {
        parts.emplace_back();
      } else {
        parts.back().push_back(ch);
      }
    });
  return parts;
}