#ifndef _MBUILD_VAR_EVAL_H
#define _MBUILD_VAR_EVAL_H

#include <string>
#include <unordered_map>

namespace mbuild {
std::string evaluate_vars(std::string_view input, const std::unordered_map<std::string, std::string> &vars);
}

#endif
