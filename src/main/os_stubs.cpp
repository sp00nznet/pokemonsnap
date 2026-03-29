/**
 * Stub implementations for OS functions that N64Recomp renames to _recomp
 * but librecomp doesn't provide implementations for.
 *
 * These are low-level OS internals that either:
 * - Are handled by ultramodern at a higher level
 * - Are not needed in the recompiled context
 * - Need proper implementation later for full functionality
 */

#include "recomp.h"

extern "C" {

// Controller Pak / Rumble Pak internals
RECOMP_FUNC void __osContAddressCrc_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osContRamRead_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osContRamWrite_recomp(uint8_t* rdram, recomp_context* ctx) {}

// PFS (Controller Pak filesystem) internals
RECOMP_FUNC void __osPfsRWInode_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osPfsSelectBank_recomp(uint8_t* rdram, recomp_context* ctx) {}

// Timer internals
RECOMP_FUNC void __osInsertTimer_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osSetTimerIntr_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osTimerInterrupt_recomp(uint8_t* rdram, recomp_context* ctx) {}

// COP0 register access (handled by ultramodern)
RECOMP_FUNC void __osSetCompare_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osSetWatchLo_recomp(uint8_t* rdram, recomp_context* ctx) {}

// SI (serial interface) internals
RECOMP_FUNC void __osSiCreateAccessQueue_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osSiDeviceBusy_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osSiRawStartDma_recomp(uint8_t* rdram, recomp_context* ctx) {}

// VI (video interface) internals
RECOMP_FUNC void __osViGetCurrentContext_recomp(uint8_t* rdram, recomp_context* ctx) {}
RECOMP_FUNC void __osViSwapContext_recomp(uint8_t* rdram, recomp_context* ctx) {}

// EPI (external peripheral interface)
RECOMP_FUNC void osEPiWriteIo_recomp(uint8_t* rdram, recomp_context* ctx) {}

} // extern "C"
