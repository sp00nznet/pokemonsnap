#ifndef PATCHES_H
#define PATCHES_H

#include "ultra64.h"

/* Section attributes for patch functions */
#define RECOMP_PATCH        __attribute__((section(".recomp_patch")))
#define RECOMP_FORCE_PATCH  __attribute__((section(".recomp_force_patch")))
#define RECOMP_EXPORT       __attribute__((section(".recomp_export")))
#define RECOMP_CALLBACK     __attribute__((section(".recomp_callback")))
#define RECOMP_EVENT        __attribute__((section(".recomp_event")))

/* Recomp API declarations */
void recomp_puts(const char* str);
void recomp_exit(void);
float recomp_get_target_framerate(void);
float recomp_get_aspect_ratio(void);
float recomp_get_target_aspect_ratio(void);
u32 recomp_get_resolution_scale(void);
float recomp_powf(float base, float exp);
void recomp_load_overlay_by_id(u32 id);

#endif /* PATCHES_H */
