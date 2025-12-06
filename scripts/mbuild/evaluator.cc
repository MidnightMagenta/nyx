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

const static std::unordered_set<std::string> manifest_allowed = {
        "modules",
        "subdirs",
};

const static std::unordered_set<std::string> module_allowed = {
        "target", "sources", "CFLAGS", "CPPFLAGS", "CXXFLAGS", "ASFLAGS", "LDFLAGS",

};

const static std::unordered_set<std::string> module_file_override_allowed = {
        "CFLAGS",
        "CPPFLAGS",
        "CXXFLAGS",
        "ASFLAGS",
};

static bool verify_keys(const toml::table &tbl, const std::unordered_set<std::string> &keys) {
    for (auto &[key, value] : tbl) {
        if (keys.find(std::string(key.str())) == keys.end()) {
            std::cerr << "Invalid key " << std::string(key.data());
            return false;
        }
    }

    return true;
}

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

static std::filesystem::path make_absolute(const std::filesystem::path &path) {
    return std::filesystem::weakly_canonical(path);
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

    if (m_verbose) { std::cout << "Found build_cfg.toml at " << m_rootDir << "\n"; }

    if (!verify_keys(cfg, build_cfg_allowed)) { throw std::runtime_error(""); }

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
    m_defSettings.dirs.source_dir = make_absolute(get_required_key<std::string>(cfg, "SRC_DIR"));
    m_defSettings.dirs.build_dir  = make_absolute(get_required_key<std::string>(cfg, "BUILD_DIR")) / m_defSettings.arch;
    m_defSettings.dirs.arch_dir   = make_absolute(get_required_key<std::string>(cfg, "ARCH_DIR")) / m_defSettings.arch;
    {
        std::vector<std::string> incPaths = get_optional_array<std::string>(cfg, "INCLUDE_DIRS");
        for (const auto &i : incPaths) { m_defSettings.dirs.include_dirs.push_back(make_absolute(i)); }
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

    if (!verify_keys(arch_cfg, arch_cfg_allowed)) { throw std::runtime_error(""); }

    if (cfg["CROSS_COMPILE"].value_or(false)) {
        m_defSettings.tools.cross_prefix = arch_cfg["CROSS_PREFIX"].value_or("");
        if (m_verbose) {
            std::cout << std::left << std::setw(20) << "Cross compile: " << m_defSettings.tools.cross_prefix << "\n";
        }
    }

    m_defSettings.linker_script =
            make_absolute((m_defSettings.dirs.arch_dir / get_required_key<std::string>(arch_cfg, "LINKER_SCRIPT")));

    if (m_verbose) { std::cout << std::left << std::setw(20) << "Linker: " << m_defSettings.linker_script << "\n"; }

    // get arch include_dirs
    {
        std::vector<std::string> incPaths = get_optional_array<std::string>(arch_cfg, "INCLUDE_DIRS");
        for (const auto &i : incPaths) {
            m_defSettings.dirs.include_dirs.push_back(make_absolute(m_defSettings.dirs.arch_dir / i));
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

    m_dirQueue.push(m_rootDir);
    if (cfg["INCLUDE_ARCH_MANIFEST"].value_or(false)) { m_dirQueue.push(m_defSettings.dirs.arch_dir); }
}

void mbuild::Evaluator::eval_manifest(const std::filesystem::path &dir) {
    if (m_verbose) { std::cout << "Evaluating manifest " << dir / "manifest.toml" << "\n"; }

    toml::table manifest;
    try {
        manifest = toml::parse_file((dir / "manifest.toml").string());
    } catch (const toml::parse_error &e) { throw std::runtime_error(e.what()); }

    if (m_verbose) { std::cout << "Modules:\n"; }
    std::vector<std::string> modules = get_optional_array<std::string>(manifest, "modules");
    for (const auto &d : modules) {
        if (m_verbose) { std::cout << std::setw(5) << "" << make_absolute(dir / d) << "\n"; }
        m_moduleQueue.push(make_absolute(dir / d));
    }

    if (m_verbose) { std::cout << "Subdirectories:\n"; }
    std::vector<std::string> subdirs = get_optional_array<std::string>(manifest, "subdirs");
    for (const auto &d : subdirs) {
        if (m_verbose) { std::cout << std::setw(5) << "" << make_absolute(dir / d) << "\n"; }
        m_dirQueue.push(make_absolute(dir / d));
    }
}

void mbuild::Evaluator::eval_module(const std::filesystem::path &dir) {
    if (m_verbose) { std::cout << "Evaluating module " << dir << "\n"; }

    toml::table module;
    try {
        module = toml::parse_file((dir / "module.toml").string());
    } catch (const toml::parse_error &e) { throw std::runtime_error(e.what()); }

    std::vector<std::string> sources = get_required_array<std::string>(module, "sources");

    {
        std::unordered_set<std::string> source_set(sources.begin(), sources.end());
        for (auto &[key, value] : module) {
            std::string k = std::string(key.str());

            if (module_allowed.find(k) != module_allowed.end()) { continue; }
            if (source_set.find(k) != source_set.end()) {
                if (!value.is_table()) { throw std::runtime_error("Source override \'" + k + "\' must be a table"); }
                if (verify_keys(*value.as_table(), module_file_override_allowed)) { continue; }
            }

            throw std::runtime_error("Unknown key in module: " + k);
        }
    }

    if (module["CONTAINS_MANIFEST"].value_or(false)) { m_dirQueue.push(dir); }

    Module m;
    m.target = get_required_key<std::string>(module, "target");
    if (m.target == "builtin") {
        m.target = m_defSettings.target;
    } else if (m.target == "lib") {
        m.target = m_defSettings.lib_target;
    }

    if (m_verbose) { std::cout << std::left << std::setw(10) << "Target: " << m.target << "\n"; }

    auto append = [](auto &dst, const auto &src) { dst.insert(dst.end(), src.begin(), src.end()); };

    {
        std::vector<std::string> cflags   = get_optional_array<std::string>(module, "CFLAGS");
        std::vector<std::string> cppflags = get_optional_array<std::string>(module, "CPPFLAGS");
        std::vector<std::string> cxxflags = get_optional_array<std::string>(module, "CXXFLAGS");
        std::vector<std::string> asflags  = get_optional_array<std::string>(module, "ASFLAGS");
        std::vector<std::string> ldflags  = get_optional_array<std::string>(module, "LDFLAGS");

        append(m.cflags.flags, cflags);
        append(m.cppflags.flags, cppflags);
        append(m.cxxflags.flags, cxxflags);
        append(m.asflags.flags, asflags);
        append(m.ldflags.flags, ldflags);

        m.cflags.append   = module["OVERRIDE_CFLAGS"].value_or(false);
        m.cppflags.append = module["OVERRIDE_CXXFLAGS"].value_or(false);
        m.cxxflags.append = module["OVERRIDE_CPPFLAGS"].value_or(false);
        m.asflags.append  = module["OVERRIDE_ASFLAGS"].value_or(false);
        m.ldflags.append  = module["OVERRIDE_LDFLAGS"].value_or(false);
    }

    if (m_verbose) { std::cout << "Sources:\n"; }
    for (const auto &s : sources) {
        Source src;
        src.source_path = make_absolute(dir / s);
        src.object_path = m_defSettings.dirs.build_dir / (dir / s).lexically_relative(m_defSettings.dirs.source_dir);
        src.object_path = make_absolute(src.object_path);
        src.object_path.replace_extension(".o");

        const static std::unordered_map<std::string, std::string> rule_map = {
                {".c", "cc"}, {".cpp", "cxx"}, {".cc", "cxx"}, {".s", "as"}, {".asm", "as"}, {".S", "cc_asm"},
        };

        auto rule_it = rule_map.find(src.source_path.extension());
        if (rule_it == rule_map.end()) { throw std::runtime_error("Unknown file: " + src.source_path.string()); }
        src.rule = rule_it->second;

        if (m_verbose) {
            std::cout << src.source_path.lexically_relative(m_rootDir) << " -" << src.rule << "-> "
                      << src.object_path.lexically_relative(m_rootDir) << "\n";
        }

        if (auto *tbl = module[s].as_table()) {
            std::vector<std::string> cflags   = get_optional_array<std::string>(*tbl, "CFLAGS");
            std::vector<std::string> cppflags = get_optional_array<std::string>(*tbl, "CPPFLAGS");
            std::vector<std::string> cxxflags = get_optional_array<std::string>(*tbl, "CXXFLAGS");
            std::vector<std::string> asflags  = get_optional_array<std::string>(*tbl, "ASFLAGS");

            append(src.cflags.flags, cflags);
            append(src.cppflags.flags, cppflags);
            append(src.cxxflags.flags, cxxflags);
            append(src.asflags.flags, asflags);

            m.cflags.append   = (*tbl)["OVERRIDE_CFLAGS"].value_or(false);
            m.cppflags.append = (*tbl)["OVERRIDE_CXXFLAGS"].value_or(false);
            m.cxxflags.append = (*tbl)["OVERRIDE_CPPFLAGS"].value_or(false);
            m.asflags.append  = (*tbl)["OVERRIDE_ASFLAGS"].value_or(false);
        }

        m.sources.push_back(src);
    }

    m_modules.push_back(m);
}

int mbuild::Evaluator::update() {
    std::cout << "Updatig build configuration...\n";
    int res = 0;

    res = read_config();
    if (res != 0) { return res; }

    try {
        while (!m_moduleQueue.empty() || !m_dirQueue.empty()) {
            if (!m_moduleQueue.empty()) {
                eval_module(m_moduleQueue.front());
                m_moduleQueue.pop();
            }
            if (!m_dirQueue.empty()) {
                eval_manifest(m_dirQueue.front());
                m_dirQueue.pop();
            }
        }
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

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
