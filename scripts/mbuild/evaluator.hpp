#ifndef MBUILD_EVALUATOR
#define MBUILD_EVALUATOR

#include "source_tree.hpp"
#include <filesystem>
#include <queue>
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

    void eval_manifest(const std::filesystem::path &dir);
    void eval_module(const std::filesystem::path &dir);

private:
    bool m_verbose = false;

    std::filesystem::path m_rootDir;
    Mode                  m_mode = Mode::NONE;
    DefaultSettings       m_defSettings;

    std::queue<std::filesystem::path> m_dirQueue;
    std::queue<std::filesystem::path> m_moduleQueue;

    std::vector<Module> m_modules;
};
}// namespace mbuild

#endif// !MBUILD_EVALUATOR
