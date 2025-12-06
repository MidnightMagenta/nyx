#ifndef _MBUILD_SOURCE_TREE
#define _MBUILD_SOURCE_TREE

#include <filesystem>
#include <string>
#include <vector>

namespace mbuild {
struct ModuleFlags {
    std::vector<std::string> flags;
    bool                     append;
};

struct Source {
    std::filesystem::path source_path;
    std::filesystem::path object_path;
    std::string           rule;
    ModuleFlags           cflags;
    ModuleFlags           cppflags;
    ModuleFlags           cxxflags;
    ModuleFlags           asflags;
};

struct Module {
    std::string         target;
    std::vector<Source> sources;
    ModuleFlags         cflags;
    ModuleFlags         cppflags;
    ModuleFlags         cxxflags;
    ModuleFlags         asflags;
    ModuleFlags         ldflags;
};
}// namespace mbuild

#endif// !_MBUILD_SOURCE_TREE
