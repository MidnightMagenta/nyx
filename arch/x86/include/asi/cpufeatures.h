#ifndef _ASI_CPU_FEATURES_H
#define _ASI_CPU_FEATURES_H

struct cpu_features {
    bool cf_pdpe1g;
};

extern struct cpu_features g_cpu_features;

#endif
