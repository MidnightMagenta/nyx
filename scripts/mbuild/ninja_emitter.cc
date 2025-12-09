#include "ninja_emitter.hpp"
#include "var_eval.hpp"
#include <fmt/core.h>
#include <sstream>

std::string mbuild::NinjaEmitter::build_flags(std::string_view name, const std::vector<std::string> &flags) {
    std::stringstream out;
    out << name << " = ";
    for (const auto &f : flags) { out << fmt::format("{} ", f); }
    return out.str();
}

std::string mbuild::NinjaEmitter::build_rule(const Rule &r) {
    std::stringstream out;
    out << fmt::format("rule {}\n", r.name);
    out << "\tcommand = " << r.command << "\n";
    if (!r.description.empty()) { out << "\tdescription = " << r.description << "\n"; }
    if (r.has_deps) { out << "\tdepfile = $out.d\n" << "\tdeps = gcc\n"; }

    return out.str();
}

void mbuild::NinjaEmitter::write_head(std::ofstream &out) {
    // write global variables
    out << build_flags("cflags", m_defSettings.cflags) << "\n";
    out << build_flags("cppflags", m_defSettings.cppflags) << "\n";
    out << build_flags("cxxflags", m_defSettings.cxxflags) << "\n";
    out << build_flags("asflags", m_defSettings.asflags) << "\n";
    out << build_flags("ldflags", m_defSettings.ldflags) << "\n";
    out << "\n";

    // write build rules
    std::stringstream inc;
    for (const auto &include_f : m_defSettings.dirs.include_dirs) { inc << fmt::format(" -I{}", include_f.string()); }

    std::unordered_map<std::string, std::string> vars = {
            {"cc", m_defSettings.tools.cc}, {"cxx", m_defSettings.tools.cxx}, {"as", m_defSettings.tools.as},
            {"ld", m_defSettings.tools.ld}, {"ar", m_defSettings.tools.ar},   {"include", inc.str()},
    };

    for (const auto &r : m_rules) { out << evaluate_vars(build_rule(r), vars) << "\n"; }
}

void mbuild::NinjaEmitter::emit(const std::vector<Module> &srcs) {
    if ((m_defSettings.dirs.build_dir / "build.ninja").has_parent_path()) {
        std::filesystem::create_directories((m_defSettings.dirs.build_dir / "build.ninja").parent_path());
    }
    std::ofstream out(m_defSettings.dirs.build_dir / "build.ninja");
    if (!out.is_open()) { throw std::runtime_error("Failed to open build.ninja"); }
    write_head(out);

    std::unordered_map<std::string, std::vector<const Module *>> modules_by_target;

    for (auto &m : srcs) { modules_by_target[m.target].push_back(&m); }

    for (const auto &[key, value] : modules_by_target) {
        std::vector<std::string> objs;
        for (const auto &m : value) {
            for (const auto &s : m->sources) {
                objs.push_back(s.object_path);
                out << "build " << s.object_path << ": " << s.rule << " " << s.source_path << "\n";
            }
        }
        out << "build " << key << ": ld";
        for (const auto &o : objs) { out << " " << o; }
        out << "\n";
    }
}
