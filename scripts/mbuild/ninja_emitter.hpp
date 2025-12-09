#ifndef _MBUILD_NINJA_EMITTER_H
#define _MBUILD_NINJA_EMITTER_H

#include "source_tree.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace mbuild {
class NinjaEmitter {
public:
    NinjaEmitter() {}
    NinjaEmitter(const DefaultSettings &settings) { m_defSettings = settings; }
    ~NinjaEmitter() {}

    void set_settings(const DefaultSettings &settings) { m_defSettings = settings; }
    void emit(const std::vector<Module> &srcs);

private:
    struct Rule {
        std::string name;
        std::string command;
        std::string description;
        bool        has_deps;
    };

private:
    std::string build_flags(std::string_view name, const std::vector<std::string> &flags);
    std::string build_rule(const Rule &r);
    void        write_head(std::ofstream &out);

private:
    DefaultSettings   m_defSettings;
    std::vector<Rule> m_rules = {
            {"cc", "{cc} -MD -MF $out.d {include} $cppflags $cflags -c $in -o $out", "CC $out", true},
            {"cxx", "{cxx} -MD -MF $out.d {include} $cppflags $cxxflags -c $in -o $out", "CXX $out", true},
            {"as_cpp",
             "{cc} -x assembler-with-cpp -D_ASSEMBLY_ -MD -MF $out.d {include} $cppflags $asflags -c $in -o $out",
             "AS_CPP $out", true},
            {"as", "{as} $asflags -c $in -o $out", "AS $out", false},
            {"ld", "{ld} $ldflags $cflags -o $out $in", "LD $out", false},
            {"ar", "{ar} rcs $out $in", "AR $out", false},
    };
};
}// namespace mbuild

#endif
