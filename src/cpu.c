#include "cpu.h"

#include "kprintf.h"
#include "kstring.h"

static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    // Inline assembly: mapping C variables directly to CPU registers
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(0));
}

void cpu_identify(struct cpu_info *info) {
    uint32_t eax, ebx, ecx, edx;

    // Leaf 0: the 12-character vendor ID is spread across EBX, EDX and ECX
    cpuid(0, &eax, &ebx, &ecx, &edx);
    uint32_t max_leaf = eax;
    memcpy(info->vendor, &ebx, 4);
    memcpy(info->vendor + 4, &edx, 4);
    memcpy(info->vendor + 8, &ecx, 4);
    info->vendor[12] = '\0';

    // Leaf 1: family/model/stepping and feature flags
    info->family = info->model = info->stepping = 0;
    info->features_ecx = info->features_edx = 0;
    if (max_leaf >= 1) {
        cpuid(1, &eax, &ebx, &ecx, &edx);
        info->stepping = eax & 0xF;
        info->model = (eax >> 4) & 0xF;
        info->family = (eax >> 8) & 0xF;
        if (info->family == 0xF) info->family += (eax >> 20) & 0xFF;
        if (info->family == 0x6 || info->family >= 0xF) info->model |= ((eax >> 16) & 0xF) << 4;
        info->features_ecx = ecx;
        info->features_edx = edx;
    }

    // Leaves 0x80000002-0x80000004: the 48-character brand string, if available
    info->brand[0] = '\0';
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000004) {
        uint32_t *brand = (uint32_t *) info->brand;
        for (uint32_t leaf = 0; leaf < 3; leaf++) {
            cpuid(0x80000002 + leaf, &brand[leaf * 4], &brand[leaf * 4 + 1],
                  &brand[leaf * 4 + 2], &brand[leaf * 4 + 3]);
        }
        info->brand[48] = '\0';

        // Intel pads the brand string with leading spaces
        size_t skip = 0;
        while (info->brand[skip] == ' ') skip++;
        memmove(info->brand, info->brand + skip, 49 - skip);
    }
}

struct feature {
    const char *name;
    int bit;
    int in_ecx;
};

static const struct feature features[] = {
    { "fpu", 0, 0 },  { "tsc", 4, 0 },    { "msr", 5, 0 },     { "pae", 6, 0 },
    { "apic", 9, 0 }, { "mmx", 23, 0 },   { "sse", 25, 0 },    { "sse2", 26, 0 },
    { "htt", 28, 0 }, { "sse3", 0, 1 },   { "ssse3", 9, 1 },   { "sse4.1", 19, 1 },
    { "sse4.2", 20, 1 }, { "aes", 25, 1 }, { "avx", 28, 1 },   { "hypervisor", 31, 1 },
};

void cpu_print_features(const struct cpu_info *info) {
    for (size_t i = 0; i < sizeof(features) / sizeof(features[0]); i++) {
        uint32_t reg = features[i].in_ecx ? info->features_ecx : info->features_edx;
        if (reg & (1u << features[i].bit)) kprintf("%s ", features[i].name);
    }
}
