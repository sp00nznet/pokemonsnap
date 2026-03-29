/**
 * Pokemon Snap: Recompiled - Main Entry Point
 *
 * Initializes SDL, creates the window, sets up the recomp runtime,
 * and launches the recompiled game.
 */

#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "librecomp/game.hpp"
#include "librecomp/recomp.h"
#include "ultramodern/ultramodern.hpp"
#include "ovl_patches.hpp"

// ROM SHA1 for Pokemon Snap (US)
static const std::string expected_rom_sha1 = "edc7c49cc568c045fe48be0d18011c30f393cbaf";

int main(int argc, char* argv[]) {
    // TODO: Initialize SDL2, create window, set up RT64 render context
    // TODO: Load ROM file (pokemonsnap.us.z64)
    // TODO: Verify ROM SHA1
    // TODO: Register overlay sections
    // TODO: Register patches
    // TODO: Initialize recomp runtime
    // TODO: Start game execution

    printf("Pokemon Snap: Recompiled\n");
    printf("ROM SHA1: %s\n", expected_rom_sha1.c_str());

    return 0;
}
