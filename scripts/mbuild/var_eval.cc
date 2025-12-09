#include "var_eval.hpp"

std::string mbuild::evaluate_vars(std::string_view input, const std::unordered_map<std::string, std::string> &vars) {
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '{') {
            size_t end = input.find('}', i + 1);
            if (end != std::string::npos) {
                std::string key = std::string(input.substr(i + 1, end - (i + 1)));

                auto it = vars.find(key);
                if (it != vars.end()) {
                    out += it->second;
                } else {
                    out.append(input.substr(i, end - i + 1));
                }

                i = end;
                continue;
            }
        }
        out.push_back(input[i]);
    }

    return out;
}
