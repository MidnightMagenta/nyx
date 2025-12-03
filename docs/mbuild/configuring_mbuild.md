## Mbuild configuration

Mbuild is configured using TOML files that contain specific information. There are three types of configuration files. A global configuration, a manifest, and a module configuration. The global config specifies global variables, such as the compiler, or global compiler flags. A manifest declares what subdirectories exist, and what modules should be evaluated. A module config specifies how a specific module should be built and linked.

### Definitions

| Term               | Definition                                                                        |
| ------------------ | --------------------------------------------------------------------------------- |
| Module file        | A configuration file describing a module. See [Module files]                      |
| Global flags       | A set of all flags that are not defined inside a module file                      |
| Architecture flags | A set of flags defined in [Architecture specific configuration file]              |
| Module flags       | A set of flags defined in a module, that are applied to all sources defined in it |
| File flags         | A set of flags defined in a module applied only to a specific source file         |

### Global configuration files

There can be at most two global configuration files used at once (a project may define many, but only two at most may be used), the main configuration file, and the architecture specific configuration file.

#### Main configuration file

The main configuration file specifies architecture independent information, such as the host compiler, or global, non-architecture specific compiler flags. The main configuration file must be located in the root directory of the project, and must be named `build_cfg.toml`

The main configuration file may specify the following variables:

Toolchain configuration:

| Variable | Type   | Synopsis                                         |
| -------- | ------ | ------------------------------------------------ |
| HOSTCC   | string | Host C compiler                                  |
| HOSTCXX  | string | Host C++ compiler                                |
| HOSTAS   | string | Host assembler used to assemble .s or .asm files |
| HOSTLD   | string | Host linker                                      |
| HOSTAR   | string | Host archiver (for linking static libraries)     |

Compilation options:

| Variable              | Type   | Synopsis                                                         |
| --------------------- | ------ | ---------------------------------------------------------------- |
| ARCH                  | string | Target architecture                                              |
| CROSS_COMPILE         | bool   | If true, use a cross toolchain, otherwise use the host toolchain |
| INCLUDE_ARCH_MANIFEST | bool   | If true, arch/$ARCH/manifest.toml will be read                   |
| WARN_LEVEL            | string | Configures the verbosity of compiler warnings                    |
| WARNING_AS_ERRORS     | bool   | If true, compiler warnings are treated as errors                 |
| DEBUG                 | bool   | Include debug information in the compiled binary                 |
| OPT_LEVEL             | int    | Optimization level                                               |
| CFLAGS                | array  | Flags for the C compiler                                         |
| CPPFLAGS              | array  | Flags for the C preprocessor                                     |
| CXXFLAGS              | array  | Flags for the C++ compiler                                       |
| ASFLAGS               | array  | Flags for the assembler                                          |
| LDFLAGS               | array  | Flags for the linker                                             |
| TARGET                | string | The main compilation target (final executable)                   |
| LIB_TARGET            | string | The library compilation target (Project's static library)        |

Directory options:

| Variable     | Type   | Synopsis                                                                |
| ------------ | ------ | ----------------------------------------------------------------------- |
| SRC_DIR      | string | Directory containing the source files (the root `manifest.toml`)        |
| BUILD_DIR    | string | Build output directory                                                  |
| ARCH_DIR     | string | Directory containing architecture specific sources                      |
| INCLUDE_DIRS | array  | Array of directories containing C/C++ headers to be globally includable |

Available options for WARN_LEVEL are:

- none - Does not add any extra warnings
- normal - Adds `-Wall` and `-Wextra` warnings
- strict - Adds `-Wpedantic` warnings
- paranoid - Adds `-Wconversion -Wsign-conversion -Wundef -Wcast-align -Wshift-overflow -Wdouble-promotion` warnings

> [!NOTE]
> Above flags only apply to C sources. C++ and Assembly warning flags are not defined (the author hasn't made up their mind ;)

#### Architecture specific configuration file

The architecture specific configuration file may be used to configure global variables that vary between different CPU architectures. The architecture specific configuration file must be located at `$ARCH_DIR/$ARCH/arch_cfg.toml`

The architecture specific configuration file may define the following variables:

| Variable     | Type   | Synopsis                                                                                      |
| ------------ | ------ | --------------------------------------------------------------------------------------------- |
| CROSS_PREFIX | string | The prefix used to call the cross toolchain (e.g. $CROSS_PREFIX$CC)                           |
| INCLUDE_DIRS | array  | Array of directories containing architecture specific C/C++ headers to be globally includable |
| CFLAGS       | array  | Flags for the C compiler                                                                      |
| CPPFLAGS     | array  | Flags for the C preprocessor                                                                  |
| CXXFLAGS     | array  | Flags for the C++ compiler                                                                    |
| ASFLAGS      | array  | Flags for the assembler                                                                       |
| LDFLAGS      | array  | Flags for the linker                                                                          |

### Manifest files

Manifest files are used to specify what modules and subdirectories should be included.

2 variables may be defined in a manifest file.

| Variable | Type  | Synopsis                      |
| -------- | ----- | ----------------------------- |
| modules  | array | Modules to be included        |
| subdirs  | array | subdirectories to be included |

Modules are directories that contain a `module.toml` file. Subdirectories are directories which contain a `manifest.toml` file.
A directory may contain both a manifest and a module file, and be specified in both arrays. Alternatively a module may declare that it's directory contains a manifest file, in which case both the `module.toml` and `manifest.toml` files will be included by adding the directory only to the `modules` variable.

Duplicate module or subdirectory inclusions, whether withing the same manifest file, or across multiple manifest files cause the evaluation to stop with an error. No files may be altered if the evaluation fails at this stage.

### Module files

A module file describes a module. A module is a self contained unit composed of source and header files which may be compiled and linked independently. Typically a module will have a specialized function. Module files are the only files which may add sources to the build tree.

A module file must define two variables.

| Variable | Type   | Synopsis                                             |
| -------- | ------ | ---------------------------------------------------- |
| target   | string | The target into which the module will be linked into |
| sources  | array  | An array of sources which compose the module         |

A module may declare the following types of source files:

- \*.c - a C source file
- \*.cc \*.cpp - a C++ source file
- \*.s \*.asm - an assembly source file which is not passed through the C preprocessor
- \*.S - an assembly source file which is passed by the C preprocessor

The target is an opaque identifier referring to one final link output. All modules that specify the same target are linked together. If two modules declare the same target, they will be linked into the same final link output (i.e. they'll be linked together). There are however two special targets:

- `builtin` - The module will be linked into the target specified by the TARGET variable in the main configuration file
- `lib` - The module will be linked into the target specified by the LIB_TARGET variable in the main configuration file

These targets are convenience values, and will behave identically to specifying $TARGET or $LIB_TARGET as the module's target.

A module may also optionally specify the following variables

| Variable | Type  | Synopsis                     |
| -------- | ----- | ---------------------------- |
| CFLAGS   | array | Flags for the C compiler     |
| CPPFLAGS | array | Flags for the C preprocessor |
| CXXFLAGS | array | Flags for the C++ compiler   |
| ASFLAGS  | array | Flags for the assembler      |
| LDFLAGS  | array | Flags for the linker         |

These variables may be specified either in the global scope of the module file, in which case they'll be appended to the global flags for all sources the module specifies. Alternatively these flags may be specified in a scope with the same name as a source file (i.e. `[foo.c]`), in which case they'll be appended only for the compilation of that specific source file. Due to the nature of linking (linking does not happen at file granularity) LDFLAGS may not be specified per source.

It's possible to override all flags by specifying a boolean value named `OVERRIDE_[*FLAGS]`. By default this value is false, if this variable is set to `true`, instead of appending the module or file specific flags to the global flags, they'll fully override all flags (i.e. only these flags will be passed to the compiler, and global flags \[global flags = all flags not defined in a module file\] will be omitted).

A module may also specify a variable `CONTAINS_MANIFEST`. This flag is by default set to `false`. If this flag is set to `true`, a `manifest.toml` file contained in the same directory as the module file will be evaluated.

> [!NOTE]
> All variable names (left side) are reserved for future use. Unknown variables MUST cause an error during evaluation. Defining a new variable must be reflected in this document.
