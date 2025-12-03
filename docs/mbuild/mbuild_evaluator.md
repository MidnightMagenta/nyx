## mbuild evaluator

The mbuild evaluator constructs a build dependency graph in the selected backend (by default Ninja) inside the build directory (or root directory is BUILD_DIR = "") based on the detected configuration files (see [Configuring mbuild](configuring_mbuild.md)). The evaluator can also automatically trigger the build as per the used backend.

### Mbuild rules

Mbuild can generate rules for C, C++, and Assembly compilation, as well as linking and archiving static libraries.

> [!NOTE]
> A future version of this system will be able to generate custom rules. For this purpose the filename `build_rules.toml` is reserved in the root directory of the project.

The rules are defined as follows (Using Ninja syntax. Values delimited with \<\> refer to variables defined in build configuration files, and are automatically substituted during build dependency graph generation):

C compilation rule:

```
rule cc
    depfile = $out.d
    deps = gcc
    description = CC $out
    command = <CC> -I<INCLUDE_DIRS> -MD -MF $out.d $cppflags $cflags -c $in -o $out
```

> [!NOTE]
> Include directory flags are specified for each entry in the INCLUDE_DIRS array. For readability this expansion is omitted in these rule descriptions.

C++ compilation rule:

```
rule cxx
    depfile = $out.d
    deps = gcc
    description = CXX $out
    command = <CXX> -I<INCLUDE_DIRS> -MD -MF $out.d $cppflags $cxxflags -c $in -o $out
```

> [!NOTE]
> Include directory flags are specified for each entry in the INCLUDE_DIRS array. For readability this expansion is omitted in these rule descriptions.

Assembly (\*.s & \*.asm) compilation rule:

```
rule as
    description = AS $out
    command = <AS> $asflags -c $in -o $out
```

Preprocessed assembly (\*.S) compilation rule:

```
rule as_cpp
    description = AC $out
    command = <CC> -x assembler-with-cpp -D_ASSEMBLY_ $cppflags $asflags -c $in -o $out
```

Linking rule:

```
rule ld
    description = LD $out
    command = <LD> -T <LINKER_SCRIPT> $ldflags -o $out $in <LIB_TARGET>
```

Archiving rule:

```
rule ar
    description = AR $out
    command = <AR> rcs $out $in
```

### Module discovery

The first scanned file is always `manifest.toml` in the root directory of the project. The modules that are specified in that file are then evaluated in the order in which they're specified. The evaluation order of further modules is not specified, and reliance on any particular evaluation order should be avoided.

> [!NOTE]
> Future versions of this software will allow modules to define dependencies. If a module defines a dependency, it is guaranteed that the dependent module is compiled and linked first in the generated build graph. This however does not affect the evaluation order of module configuration files.

### Flag concentration

Tool flags are concentrated in the following order:

```
flags = main_config_flags + arch_config_flags + module_flags + file_flags
```

If any of these flags are not specified, they're assumed to be an empty array, and are concentrated as such (i.e. they will not be included in the final flags)
If a module specifies the OVERRIDE\_\*FLAGS boolean as `true`, the flags are as follows:

```
flags = module_flags + file_flags
```

If a file specifies the OVERRIDE\_\*FLAGS boolean as `true`, the flags are as follows

```
flags = file_flags
```

### Source resolution

When a module file is evaluated, all source files are prefixed with the path to the module relative to the root directory of the project (the location of `build_cfg.toml`).

As an example, if we have the following file structure

```
root_dir/
├─ foo/
│  ├─ bar.c
│  ├─ module.toml
├─ build_cfg.toml
├─ manifest.toml
```

`./manifest.toml` includes the module `foo`. `./foo/module.toml` specifies the source `bar.c`. When `./foo/module.toml` is being evaluated, `bar.c` will become `foo/bar.c`. If a build directory is specified, the resulting object file will also be prefixed with the build directory. For example is BUILD_DIR = "build", then `foo/bar.c` will become `build/foo/bar.o`.

The relative path of the module directory is always preserved to prevent filename collisions. Objects inside the build directory are laid out in an identical directory structure as the sources within the source directory.

### Object naming convention

When the build graph is generated, the names of object files must be specified. By default, these names will just be the root of the source file, with the file extension `.o`. For example, for a file `foo.c` the object file will be named `foo.o`. If a file exists in a subdirectory relative to the source directory, the relative path will be appended. For example `foo/bar.c` will become `foo/bar.o`

All link targets (both global $TARGET and $LIB_TARGET, as well as module specified link targets) are built in the build directory (or the root directory if a build directory is not specified).
