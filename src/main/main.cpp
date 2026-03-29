#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/sections.h"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/renderer_context.hpp"

#include "ovl_patches.hpp"
#include "snap_config.h"

// From recompiled output
extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();

// RSP microcode
extern RspUcodeFunc aspMain;

// RSP callback: return the microcode function for a given task
RspUcodeFunc* get_rsp_microcode(const OSTask* task) {
    if (task->t.type == M_AUDTASK) {
        return aspMain;
    }
    return nullptr;
}

// GFX callbacks
void* create_gfx() { return nullptr; }
ultramodern::renderer::WindowHandle create_window(void*) { return {}; }
void update_gfx(void*) {}

// Render context
std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    return nullptr; // TODO: RT64 integration
}

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
        .get_rsp_microcode = get_rsp_microcode,
    };

    ultramodern::renderer::callbacks_t renderer_callbacks{
        .create_render_context = create_render_context,
    };

    ultramodern::audio_callbacks_t audio_callbacks{};

    ultramodern::input::callbacks_t input_callbacks{};

    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
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

    recomp::start(cfg);
    recomp::start_game(u8"pokemonsnap_us");

    return 0;
}
