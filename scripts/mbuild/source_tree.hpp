#ifndef _MBUILD_SOURCE_TREE
#define _MBUILD_SOURCE_TREE

#include <filesystem>
#include <string>
#include <vector>

namespace mbuild {
struct DefaultSettings {
    std::string arch;
    struct Toolchain {
        std::string cross_prefix;
        std::string cc;
        std::string cxx;
        std::string as;
        std::string ld;
        std::string ar;
    };

    Toolchain tools;

    std::vector<std::string> cflags;
    std::vector<std::string> cppflags;
    std::vector<std::string> cxxflags;
    std::vector<std::string> asflags;
    std::vector<std::string> ldflags;

    struct Directories {
        std::filesystem::path              source_dir;
        std::filesystem::path              build_dir;
        std::filesystem::path              arch_dir;
        std::vector<std::filesystem::path> include_dirs;
    };

    Directories dirs;

    std::filesystem::path linker_script;

    std::filesystem::path target;
    std::filesystem::path lib_target;
};

struct ModuleFlags {
    std::vector<std::string> flags;
    bool                     override;
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
    std::string         name;
    std::string         target;
    std::vector<Source> sources;
    ModuleFlags         cflags;
    ModuleFlags         cppflags;
    ModuleFlags         cxxflags;
    ModuleFlags         asflags;
};
}// namespace mbuild

#endif// !_MBUILD_SOURCE_TREE
