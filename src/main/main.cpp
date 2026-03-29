#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/recomp.h"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/renderer_context.hpp"
#include "ultramodern/rsp.hpp"
#include "ultramodern/error_handling.hpp"
#include "ultramodern/events.hpp"
#include "ultramodern/input.hpp"
#include "ultramodern/threads.hpp"

#include "ovl_patches.hpp"
#include "snap_config.h"

// From recompiled output
extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();

// RSP microcode (recompiled by RSPRecomp)
extern "C" void aspMain(uint8_t* rdram, recomp_context* ctx);

// RSP callbacks
void rsp_init() {
    // No RSP init needed
}

bool rsp_run_task(RDRAM_ARG const OSTask* task) {
    if (task->t.type == M_AUDTASK) {
        aspMain(rdram, nullptr);
        return true;
    }
    // GFX tasks are handled by RT64 directly
    return false;
}

// RT64 render context creation
std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    // TODO: Create and return RT64 render context
    return nullptr;
}

// Stub callbacks for initial bootstrap
void gfx_create_window(ultramodern::renderer::WindowHandle) {}
ultramodern::renderer::WindowHandle gfx_get_window_handle() { return {}; }

int main(int argc, char* argv[]) {
    // Register the game
    recomp::GameEntry entry{};
    entry.rom_hash = 0; // TODO: compute from ROM
    entry.internal_name = "POKEMON SNAP";
    entry.game_id = u8"pokemonsnap_us";
    entry.mod_game_id = "pokemonsnap";
    entry.save_type = recomp::SaveType::AllowAll;
    entry.is_enabled = true;
    entry.entrypoint_address = get_entrypoint_address();
    entry.entrypoint = recomp_entrypoint;

    recomp::register_game(entry);

    // Register overlays and patches
    snap::register_overlays();
    snap::register_patches();

    // Set up callbacks
    recomp::rsp::callbacks_t rsp_callbacks{
        .init = rsp_init,
        .run_task = rsp_run_task,
    };

    ultramodern::renderer::callbacks_t renderer_callbacks{
        .create_render_context = create_render_context,
    };

    ultramodern::audio_callbacks_t audio_callbacks{};
    ultramodern::input::callbacks_t input_callbacks{};
    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_window = gfx_create_window,
        .get_window_handle = gfx_get_window_handle,
    };
    ultramodern::events::callbacks_t events_callbacks{};
    ultramodern::error_handling::callbacks_t error_handling_callbacks{};
    ultramodern::threads::callbacks_t threads_callbacks{};

    // Build configuration
    recomp::Configuration cfg{};
    cfg.project_version = { .major = 0, .minor = 1, .patch = 0, .suffix = "-dev" };
    cfg.rsp_callbacks = rsp_callbacks;
    cfg.renderer_callbacks = renderer_callbacks;
    cfg.audio_callbacks = audio_callbacks;
    cfg.input_callbacks = input_callbacks;
    cfg.gfx_callbacks = gfx_callbacks;
    cfg.events_callbacks = events_callbacks;
    cfg.error_handling_callbacks = error_handling_callbacks;
    cfg.threads_callbacks = threads_callbacks;

    // Start the recomp runtime
    recomp::start(cfg);
    recomp::start_game(u8"pokemonsnap_us");

    return 0;
}
