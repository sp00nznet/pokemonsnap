#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <array>
#include <cinttypes>

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include "SDL.h"
#include "SDL_syswm.h"
#else
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#endif

#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/sections.h"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/renderer_context.hpp"

#include "ovl_patches.hpp"
#include "snap_config.h"
#include "snap_render.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

// From recompiled output
extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();

// RSP microcode
extern RspUcodeFunc aspMain;

RspUcodeFunc* get_rsp_microcode(const OSTask* task) {
    switch (task->t.type) {
        case M_AUDTASK:
            return aspMain;
        default:
            fprintf(stderr, "Unknown RSP task type: %" PRIu32 "\n", task->t.type);
            return nullptr;
    }
}

// SDL window
static SDL_Window* window = nullptr;

ultramodern::gfx_callbacks_t::gfx_data_t create_gfx() {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) > 0) {
        fprintf(stderr, "Failed to initialize SDL2: %s\n", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }
    return {};
}

ultramodern::renderer::WindowHandle create_window(ultramodern::gfx_callbacks_t::gfx_data_t) {
    window = SDL_CreateWindow("Pokemon Snap: Recompiled",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1600, 960, SDL_WINDOW_RESIZABLE);

    if (window == nullptr) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

#if defined(_WIN32)
    return ultramodern::renderer::WindowHandle{ wmInfo.info.win.window, GetCurrentThreadId() };
#elif defined(__linux__) || defined(__ANDROID__)
    return ultramodern::renderer::WindowHandle{ window };
#elif defined(__APPLE__)
    SDL_MetalView view = SDL_Metal_CreateView(window);
    return ultramodern::renderer::WindowHandle{ wmInfo.info.cocoa.window, SDL_Metal_GetLayer(view) };
#endif
}

void update_gfx(void*) {
    // Process SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                ultramodern::quit();
                break;
        }
    }
}

// Audio
static SDL_AudioDeviceID audio_device = 0;
static uint32_t sample_rate = 48000;

void queue_samples(int16_t* audio_data, size_t sample_count) {
    // Convert s16 stereo to float stereo and queue
    static std::vector<float> float_buffer;
    if (float_buffer.size() < sample_count) {
        float_buffer.resize(sample_count);
    }
    for (size_t i = 0; i < sample_count; i += 2) {
        float_buffer[i + 0] = audio_data[i + 1] * (1.0f / 32768.0f);
        float_buffer[i + 1] = audio_data[i + 0] * (1.0f / 32768.0f);
    }
    SDL_QueueAudio(audio_device, float_buffer.data(), sample_count * sizeof(float));
}

size_t get_frames_remaining() {
    uint64_t buffered = SDL_GetQueuedAudioSize(audio_device);
    return static_cast<size_t>(buffered / (2 * sizeof(float)));
}

void set_frequency(uint32_t freq) {
    sample_rate = freq;
}

// Input - basic stub (keyboard only for now)
void poll_input(void) {
    // SDL events handled in update_gfx
}

bool get_n64_input(int controller_num, uint16_t* buttons, float* x, float* y) {
    if (controller_num != 0) return false;
    *buttons = 0;
    *x = 0.0f;
    *y = 0.0f;

    const uint8_t* keys = SDL_GetKeyboardState(nullptr);
    // Basic WASD + arrow key mapping for testing
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    *y =  1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  *y = -1.0f;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  *x = -1.0f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) *x =  1.0f;
    if (keys[SDL_SCANCODE_RETURN]) *buttons |= 0x8000; // A
    if (keys[SDL_SCANCODE_BACKSPACE]) *buttons |= 0x4000; // B
    if (keys[SDL_SCANCODE_SPACE]) *buttons |= 0x2000; // Z
    if (keys[SDL_SCANCODE_TAB]) *buttons |= 0x1000; // Start
    return true;
}

void set_rumble(uint8_t player, bool active) {
    // TODO: Controller rumble
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

#ifdef _WIN32
    SDL_setenv("SDL_AUDIODRIVER", "wasapi", true);
#endif

    // Init audio
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_AudioSpec spec{};
    spec.freq = 48000;
    spec.format = AUDIO_F32;
    spec.channels = 2;
    spec.samples = 0x100;
    audio_device = SDL_OpenAudioDevice(nullptr, false, &spec, nullptr, 0);
    if (audio_device != 0) {
        SDL_PauseAudioDevice(audio_device, 0);
    }

    // Register game
    recomp::GameEntry entry{};
    entry.rom_hash = 0;
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

    // Callbacks
    recomp::rsp::callbacks_t rsp_callbacks{
        .get_rsp_microcode = get_rsp_microcode,
    };

    ultramodern::renderer::callbacks_t renderer_callbacks{
        .create_render_context = snap::renderer::create_render_context,
    };

    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
    };

    ultramodern::audio_callbacks_t audio_callbacks{
        .queue_samples = queue_samples,
        .get_frames_remaining = get_frames_remaining,
        .set_frequency = set_frequency,
    };

    ultramodern::input::callbacks_t input_callbacks{
        .poll_input = poll_input,
        .get_input = get_n64_input,
    };

    ultramodern::events::callbacks_t events_callbacks{};
    ultramodern::error_handling::callbacks_t error_handling_callbacks{};
    ultramodern::threads::callbacks_t threads_callbacks{};

    // Start
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

    return EXIT_SUCCESS;
}
