#include <cstdio>
#include "librecomp/overlays.hpp"
#include "ovl_patches.hpp"

#include "recomp_overlays.inl"

void snap::register_overlays() {
    fprintf(stderr, "[snap] register_overlays: section_table=%p, num_sections=%zu\n",
        (void*)section_table, num_sections);
    fprintf(stderr, "[snap] register_overlays: section_table[0].rom_addr=0x%X\n",
        section_table[0].rom_addr);
    fflush(stderr);

    recomp::overlays::overlay_section_table_data_t sections{
        .code_sections = section_table,
        .num_code_sections = num_sections,
        .total_num_sections = num_sections,
    };

    recomp::overlays::overlays_by_index_t overlays{
        .table = overlay_sections_by_index,
        .len = num_sections,
    };

    recomp::overlays::register_overlays(sections, overlays);
    fprintf(stderr, "[snap] register_overlays: done\n"); fflush(stderr);
}
