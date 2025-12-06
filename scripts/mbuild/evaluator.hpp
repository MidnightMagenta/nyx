#ifndef MBUILD_EVALUATOR
#define MBUILD_EVALUATOR

#include <filesystem>
#include <queue>
#include <string>
#include <vector>

namespace mbuild {
class Evaluator {
public:
    enum Mode {
        NONE,
        UPDATE,
        BUILD,
        CLEAN,
        CLEAN_ALL,
    };

public:
    Evaluator();
    ~Evaluator() {}

    void set_root_path(const std::filesystem::path &path) { m_rootDir = std::filesystem::weakly_canonical(path); }
    void set_mode(Mode m) { m_mode = m; }
    void set_verbose(bool v) { m_verbose = v; }

    int update();
    int build();
    int clean(bool all);

private:
    int  read_config();
    void read_build_cfg();

private:
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

private:
    bool m_verbose = false;

    std::filesystem::path m_rootDir;
    Mode                  m_mode = Mode::NONE;
    DefaultSettings       m_defSettings;

    std::queue<std::filesystem::path> m_dirQueue;
    std::queue<std::filesystem::path> m_moduleQueue;
};
}// namespace mbuild

#endif// !MBUILD_EVALUATOR
