#ifndef CPU_H
#define CPU_H

#include <stdint.h>

struct cpu_info {
    char vendor[13];
    char brand[49];
    uint32_t family, model, stepping;
    uint32_t features_ecx, features_edx;
};

void cpu_identify(struct cpu_info *info);
void cpu_print_features(const struct cpu_info *info);

#endif
