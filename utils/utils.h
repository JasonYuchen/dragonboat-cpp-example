//
// Created by jason on 2019/5/16.
//

#ifndef DRAGONBOAT_CPP_EXAMPLE_UTILS_UTILS_H_
#define DRAGONBOAT_CPP_EXAMPLE_UTILS_UTILS_H_

#include <vector>
#include <string>

std::vector<std::string> split(const std::string &cmd, const char &delim = ' ');

template<typename Iter>
void printAll(std::ostream &out, Iter begin, Iter end)
{
  static_assert(
    std::is_base_of<std::forward_iterator_tag,
                    typename Iter::iterator_category>::value,
    "printAll only accept iterator");
}

#endif //DRAGONBOAT_CPP_EXAMPLE_UTILS_UTILS_H_
