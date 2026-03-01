#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PML5_IDX(a)  ((a >> 48) & 0x1FF)
#define PML4_IDX(a)  ((a >> 39) & 0x1FF)
#define PDP_IDX(a)   ((a >> 30) & 0x1FF)
#define PD_IDX(a)    ((a >> 21) & 0x1FF)
#define PT_IDX(a)    ((a >> 12) & 0x1FF)
#define OFFSET_4K(a) (a & 0xFFF)
#define OFFSET_2M(a) (a & ((1ULL << 21) - 1))
#define OFFSET_1G(a) (a & ((1ULL << 30) - 1))

void usage(const char *appname) {
    fprintf(stderr, "Usage: %s [-s 4K|2M|1G]? [-l 4|5]? [vaddr]\n", appname);
}

enum {
    PGSZ_1GiB,
    PGSZ_2MiB,
    PGSZ_4KiB,
};

void dump_pml4(uint64_t addr, int pgsize) {
    struct {
        const char *name;
        uint16_t    v;
    } levels[] = {
            {"PML4", PML4_IDX(addr)},
            {"PDP", PDP_IDX(addr)},
            {"PD", PD_IDX(addr)},
            {"PT", PT_IDX(addr)},
    };

    int lvl_count = 4;

    switch (pgsize) {
        case PGSZ_1GiB:
            lvl_count = 2;
            break;
        case PGSZ_2MiB:
            lvl_count = 3;
            break;
        case PGSZ_4KiB:
            lvl_count = 4;
            break;
        default:
            lvl_count = 4;
    }

    for (int i = 0; i < lvl_count; ++i) { printf("%-5s %5u\n", levels[i].name, levels[i].v); }

    if (pgsize == PGSZ_1GiB) {
        printf("Offset: 0x%llx\n", OFFSET_1G(addr));
    } else if (pgsize == PGSZ_2MiB) {
        printf("Offset: 0x%llx\n", OFFSET_2M(addr));
    } else {
        printf("Offset: 0x%lx\n", OFFSET_4K(addr));
    }
}

int main(int argc, char **argv) {
    int  opt;
    bool pml5   = false;
    int  pgsize = PGSZ_4KiB;

    while ((opt = getopt(argc, argv, "s:l:")) != -1) {
        switch (opt) {
            case 's':
                if (strcmp(optarg, "4K") == 0) {
                    pgsize = PGSZ_4KiB;
                } else if (strcmp(optarg, "2M") == 0) {
                    pgsize = PGSZ_2MiB;
                } else if (strcmp(optarg, "1G") == 0) {
                    pgsize = PGSZ_1GiB;
                } else {
                    usage(argv[0]);
                    return 1;
                }
                break;
            case 'l': {
                int levels = atoi(optarg);
                if (levels == 4) {
                    pml5 = false;
                } else if (levels == 5) {
                    pml5 = true;
                } else {
                    usage(argv[0]);
                    return 0;
                }
            } break;
            case '?':
                [[fallthrough]];
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        usage(argv[0]);
        return 1;
    }

    char    *end;
    uint64_t addr = strtoull(argv[optind], &end, 0);

    if (pml5) { printf("%-5s %5lu\n", "PML5", PML5_IDX(addr)); }
    dump_pml4(addr, pgsize);
    return 0;
}
