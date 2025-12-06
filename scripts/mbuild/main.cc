#include "evaluator.hpp"
#include <getopt.h>
#include <iostream>

void print_help() {
    std::cout << R"(
mbuild evaluator tool

Usage: 
    mbuild [options]

Options:
    -h, --help
        Prints this help message.
    
    -m, --mode
        Set the evaluator mode. The available modes are:
            - update
            - build
            - clean
        By default the "update" option will be used.

    -r, --root-dir
        Set the root directory of the project. By default this option is the current working directory.
)";
}

void print_help_update() { std::cout << R"(Update help)"; }

int cmd_update(int argc, char **argv) {
    mbuild::Evaluator e;

    static struct option opts[] = {
            {"help", no_argument, NULL, 'h'},
            {"verbose", no_argument, NULL, 'v'},
            {"root-dir", required_argument, NULL, 'r'},
            {0, 0, 0, 0},
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "hvr:", opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help_update();
                return 0;
            case 'v':
                e.set_verbose(true);
                break;
            case 'r':
                e.set_root_path(optarg);
                break;
            case '?':
                std::cerr << "Unknown options: " << optopt << "\n";
                [[fallthrough]];
            default:
                return 1;
        }
    }

    return e.update();
}

void print_help_build() { std::cout << R"(Build help)"; }

int cmd_build(int argc, char **argv) {
    mbuild::Evaluator e;

    static struct option opts[] = {
            {"help", no_argument, NULL, 'h'},
            {"verbose", no_argument, NULL, 'v'},
            {"root-dir", required_argument, NULL, 'r'},
            {0, 0, 0, 0},
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "hvr:", opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help_build();
                return 0;
            case 'v':
                e.set_verbose(true);
                break;
            case 'r':
                e.set_root_path(optarg);
                break;
            case '?':
                std::cerr << "Unknown options: " << optopt << "\n";
                [[fallthrough]];
            default:
                return 1;
        }
    }

    return e.build();
}

void print_help_clean() { std::cout << R"(Clean help)"; }

int cmd_clean(int argc, char **argv) {
    mbuild::Evaluator e;

    static struct option opts[] = {
            {"help", no_argument, NULL, 'h'},
            {"verbose", no_argument, NULL, 'v'},
            {"root-dir", required_argument, NULL, 'r'},
            {"clean-all", no_argument, NULL, 'a'},
            {0, 0, 0, 0},
    };

    int  opt;
    bool clean_all = false;

    while ((opt = getopt_long(argc, argv, "hvr:a", opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help_clean();
                return 0;
            case 'v':
                e.set_verbose(true);
                break;
            case 'r':
                e.set_root_path(optarg);
                break;
            case 'a':
                clean_all = true;
                break;
            case '?':
                std::cerr << "Unknown options: " << optopt << "\n";
                [[fallthrough]];
            default:
                return 1;
        }
    }

    return e.clean(clean_all);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Invalid command line options\n";
        print_help();
        return 1;
    }

    std::string option = argv[1];
    argc -= 1;
    argv += 1;

    if (option == "update") {
        return cmd_update(argc, argv);
    } else if (option == "build") {
        return cmd_build(argc, argv);
    } else if (option == "clean") {
        return cmd_clean(argc, argv);
    } else if (option == "help" || option == "-h" || option == "--help") {
        print_help();
        return 0;
    } else {
        std::cerr << "Unknown option \"" << option << "\"\n";
        print_help();
        return 1;
    }
}
