#include "evaluator.hpp"
#include "toml_inc.hpp"
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

const static std::unordered_set<std::string> build_cfg_allowed = {
        "HOSTCC",        "HOSTCXX",
        "HOSTAS",        "HOSTLD",
        "HOSTAR",        "ARCH",
        "CROSS_COMPILE", "INCLUDE_ARCH_MANIFEST",
        "WARN_LEVEL",    "WARNING_AS_ERRORS",
        "DEBUG",         "OPT_LEVEL",
        "CFLAGS",        "CPPFLAGS",
        "CXXFLAGS",      "ASFLAGS",
        "LDFLAGS",       "TARGET",
        "LIB_TARGET",    "SRC_DIR",
        "BUILD_DIR",     "ARCH_DIR",
        "INCLUDE_DIRS",
};

const static std::unordered_set<std::string> arch_cfg_allowed = {
        "CROSS_PREFIX", "INCLUDE_DIRS", "CFLAGS", "CPPFLAGS", "CXXFLAGS", "ASFLAGS", "LDFLAGS", "LINKER_SCRIPT",
};

template<typename T>
static T get_required_key(const toml::table &tbl, std::string_view key) {
    auto v = tbl[key].template value<T>();
    if (!v) { throw std::runtime_error(std::string(key) + "is required"); }
    return *v;
}

template<typename T>
static std::vector<T> get_optional_array(const toml::table &tbl, std::string_view key) {
    std::vector<T> out;

    if (auto arr = tbl[key].as_array()) {
        for (auto &&v : *arr) {
            if (auto val = v.value<T>()) { out.push_back(*val); }
        }
    }

    return out;
}

template<typename T>
static std::vector<T> get_required_array(const toml::table &tbl, std::string_view key) {
    std::vector<T> out;

    if (auto arr = tbl[key].as_array()) {
        for (auto &&v : *arr) {
            if (auto val = v.value<T>()) { out.push_back(*val); }
        }
    } else {
        throw std::runtime_error(std::string(key) + "is required");
    }

    return out;
}

static std::filesystem::path make_relative(std::string_view path, const std::filesystem::path &base) {
    return std::filesystem::weakly_canonical(path).lexically_relative(base);
}

int mbuild::Evaluator::read_config() {
    try {
        read_build_cfg();
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}

void mbuild::Evaluator::read_build_cfg() {
    toml::table cfg;
    try {
        cfg = toml::parse_file((m_rootDir / "build_cfg.toml").string());
    } catch (const toml::parse_error &e) { throw std::runtime_error(e.what()); }

    if (m_verbose) { std::cout << "Found build_cfg.toml at " << std::filesystem::weakly_canonical(m_rootDir) << "\n"; }

    for (auto &[key, value] : cfg) {
        if (build_cfg_allowed.find(std::string(key.str())) == build_cfg_allowed.end()) {
            throw std::runtime_error("Invalid key " + std::string(key.data()));
        }
    }

    // get toolchain options
    m_defSettings.tools.cc  = cfg["HOSTCC"].value_or("gcc");
    m_defSettings.tools.cxx = cfg["HOSTCXX"].value_or("g++");
    m_defSettings.tools.as  = cfg["HOSTAS"].value_or("as");
    m_defSettings.tools.ld  = cfg["HOSTLD"].value_or("ld");
    m_defSettings.tools.ar  = cfg["HOSTAR"].value_or("ar");

    if (m_verbose) {
        std::cout << std::left << std::setw(20) << "CC: " << m_defSettings.tools.cc << "\n";
        std::cout << std::left << std::setw(20) << "CXX: " << m_defSettings.tools.cxx << "\n";
        std::cout << std::left << std::setw(20) << "AS: " << m_defSettings.tools.as << "\n";
        std::cout << std::left << std::setw(20) << "LD: " << m_defSettings.tools.ld << "\n";
        std::cout << std::left << std::setw(20) << "AR: " << m_defSettings.tools.ar << "\n";
    }

    // get architecture
    m_defSettings.arch = get_required_key<std::string>(cfg, "ARCH");

    // get default targets
    m_defSettings.target     = get_required_key<std::string>(cfg, "TARGET");
    m_defSettings.lib_target = get_required_key<std::string>(cfg, "LIB_TARGET");

    // get directories
    m_defSettings.dirs.source_dir = make_relative(get_required_key<std::string>(cfg, "SRC_DIR"), m_rootDir);
    m_defSettings.dirs.build_dir =
            make_relative(get_required_key<std::string>(cfg, "BUILD_DIR"), m_rootDir) / m_defSettings.arch;
    m_defSettings.dirs.arch_dir =
            make_relative(get_required_key<std::string>(cfg, "ARCH_DIR"), m_rootDir) / m_defSettings.arch;
    {
        std::vector<std::string> incPaths = get_optional_array<std::string>(cfg, "INCLUDE_DIRS");
        for (const auto &i : incPaths) { m_defSettings.dirs.include_dirs.push_back(make_relative(i, m_rootDir)); }
    }

    if (m_verbose) {
        std::cout << std::left << std::setw(20) << "Source dir: " << m_defSettings.dirs.source_dir << "\n";
        std::cout << std::left << std::setw(20) << "Build dir: " << m_defSettings.dirs.build_dir << "\n";
        std::cout << std::left << std::setw(20) << "Arch dir: " << m_defSettings.dirs.arch_dir << "\n";
    }

    auto append = [](auto &dst, const auto &src) { dst.insert(dst.end(), src.begin(), src.end()); };

    // get flags
    {
        // procedural flags
        if (cfg["WARNING_AS_ERRORS"].value_or(false)) {
            m_defSettings.cflags.push_back("-Werror");
            if (m_verbose) { std::cout << "Warnings will be treated as errors\b"; }
        }
        if (cfg["DEBUG"].value_or(false)) {
            m_defSettings.cflags.push_back("-g");
            if (m_verbose) { std::cout << "Including debug information\n"; }
        }

        static std::unordered_map<std::string, std::vector<std::string>> warn_flags = {
                {"c-normal", {}},
                {"c-normal",
                 {
                         "-Wall",
                         "-Wextra",
                 }},
                {"c-strict",
                 {
                         "-Wall",
                         "-Wextra",
                         "-Wpedantic",
                 }},
                {"c-paranoid",
                 {
                         "-Wall",
                         "-Wextra",
                         "-Wpedantic",
                         "-Wconversion",
                         "-Wsign-conversion",
                         "-Wundef",
                         "-Wcast-align",
                         "-Wshift-overflow",
                         "-Wdouble-promotion",
                 }},
        };

        std::string warn_lvl = cfg["WARN_LEVEL"].value_or("none");
        if (m_verbose) { std::cout << std::left << std::setw(20) << "Warning level: " << warn_lvl << "\n"; }

        auto c_warn_flags_it = warn_flags.find("c-" + warn_lvl);
        if (c_warn_flags_it == warn_flags.end()) { throw std::runtime_error("Unsupported warning level: " + warn_lvl); }

        append(m_defSettings.cflags, c_warn_flags_it->second);

        int opt_lvl = cfg["OPT_LEVEL"].value_or(0);
        m_defSettings.cflags.push_back("-O" + std::to_string(opt_lvl));
        if (m_verbose) { std::cout << std::left << std::setw(20) << "Optimize level: " << opt_lvl << "\n"; }

        // explicit flags
        std::vector<std::string> cflags   = get_optional_array<std::string>(cfg, "CFLAGS");
        std::vector<std::string> cppflags = get_optional_array<std::string>(cfg, "CPPFLAGS");
        std::vector<std::string> cxxflags = get_optional_array<std::string>(cfg, "CXXFLAGS");
        std::vector<std::string> asflags  = get_optional_array<std::string>(cfg, "ASFLAGS");
        std::vector<std::string> ldflags  = get_optional_array<std::string>(cfg, "LDFLAGS");

        append(m_defSettings.cflags, cflags);
        append(m_defSettings.cppflags, cppflags);
        append(m_defSettings.cxxflags, cxxflags);
        append(m_defSettings.asflags, asflags);
        append(m_defSettings.ldflags, ldflags);
    }

    // #######################
    // get arch configuration
    // #######################

    toml::table arch_cfg;
    try {
        arch_cfg = toml::parse_file((m_defSettings.dirs.arch_dir / "arch_cfg.toml").string());
    } catch (const toml::parse_error &e) { throw std::runtime_error(e.what()); }

    for (auto &[key, value] : arch_cfg) {
        if (arch_cfg_allowed.find(std::string(key.str())) == arch_cfg_allowed.end()) {
            throw std::runtime_error("Invalid key " + std::string(key.data()));
        }
    }

    if (cfg["CROSS_COMPILE"].value_or(false)) {
        m_defSettings.tools.cross_prefix = arch_cfg["CROSS_PREFIX"].value_or("");
        if (m_verbose) {
            std::cout << std::left << std::setw(20) << "Cross compile: " << m_defSettings.tools.cross_prefix << "\n";
        }
    }

    m_defSettings.linker_script = make_relative(
            (m_defSettings.dirs.arch_dir / get_required_key<std::string>(arch_cfg, "LINKER_SCRIPT")).string(),
            m_rootDir);
    if (m_verbose) { std::cout << std::left << std::setw(20) << "Linker: " << m_defSettings.linker_script << "\n"; }

    // get arch include_dirs
    {
        std::vector<std::string> incPaths = get_optional_array<std::string>(arch_cfg, "INCLUDE_DIRS");
        for (const auto &i : incPaths) {
            m_defSettings.dirs.include_dirs.push_back(
                    make_relative((m_defSettings.dirs.arch_dir / i).string(), m_rootDir));
        }
    }

    // get arch flags
    {
        std::vector<std::string> cflags   = get_optional_array<std::string>(arch_cfg, "CFLAGS");
        std::vector<std::string> cppflags = get_optional_array<std::string>(arch_cfg, "CPPFLAGS");
        std::vector<std::string> cxxflags = get_optional_array<std::string>(arch_cfg, "CXXFLAGS");
        std::vector<std::string> asflags  = get_optional_array<std::string>(arch_cfg, "ASFLAGS");
        std::vector<std::string> ldflags  = get_optional_array<std::string>(arch_cfg, "LDFLAGS");

        append(m_defSettings.cflags, cflags);
        append(m_defSettings.cppflags, cppflags);
        append(m_defSettings.cxxflags, cxxflags);
        append(m_defSettings.asflags, asflags);
        append(m_defSettings.ldflags, ldflags);
    }

    m_dirQueue.push(m_defSettings.dirs.build_dir);
    if (cfg["INCLUDE_ARCH_MANIFEST"].value_or(false)) { m_dirQueue.push(m_defSettings.dirs.arch_dir); }
}

int mbuild::Evaluator::update() {
    std::cout << "Updatig build configuration...\n";
    int res = 0;

    res = read_config();
    if (res != 0) { return res; }

    return 0;
}

int mbuild::Evaluator::build() {
    std::cout << "Building...\n";
    int res = 0;

    res = read_config();
    if (res != 0) { return res; }

    return 0;
}

int mbuild::Evaluator::clean(bool all) {
    std::cout << "Cleaning...\n";
    int res = 0;

    res = read_config();
    if (res != 0) { return res; }

    return 0;
}

mbuild::Evaluator::Evaluator() { m_rootDir = std::filesystem::current_path(); }
