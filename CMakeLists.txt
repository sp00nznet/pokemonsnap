cmake_minimum_required(VERSION 3.20)

project(PokemonSnapRecomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_PLAYING "Build the main game executable" ON)

# N64ModernRuntime (contains librecomp, ultramodern, and the N64Recomp tool)
add_subdirectory(lib/N64ModernRuntime)

# RT64 renderer (static link to avoid DLL export issues)
set(RT64_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(lib/rt64)

# RmlUi for menus
set(RMLUI_BACKEND "native" CACHE STRING "" FORCE)
set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "none" CACHE STRING "" FORCE)
add_subdirectory(lib/RmlUi)

# lunasvg for SVG rendering in UI
add_subdirectory(lib/lunasvg)

# Freetype for font rendering (Windows)
if(WIN32)
    set(FREETYPE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype-windows-binaries")
    set(FREETYPE_INCLUDE_DIRS "${FREETYPE_DIR}/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/release static/vs2026/x64/freetype.lib")
    else()
        set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/release static/win32/freetype.lib")
    endif()
endif()

# ============================================================
# Recompiled game functions (output of N64Recomp tool)
# ============================================================
file(GLOB RECOMPILED_FUNCS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/RecompiledFuncs/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/RecompiledFuncs/*.cpp"
)

add_library(RecompiledFuncs STATIC ${RECOMPILED_FUNCS_SOURCES})
target_include_directories(RecompiledFuncs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)
target_link_libraries(RecompiledFuncs PRIVATE librecomp)
set_target_properties(RecompiledFuncs PROPERTIES C_STANDARD 11)
if(MSVC)
    target_compile_options(RecompiledFuncs PRIVATE /w)
else()
    target_compile_options(RecompiledFuncs PRIVATE -w)
endif()

# ============================================================
# Patches library - DISABLED until patches build infrastructure is set up
# ============================================================
# TODO: Enable when MIPS cross-compilation toolchain is available
# Patches require clang targeting MIPS + ld.lld

# ============================================================
# RSP recompiled audio microcode
# ============================================================
add_library(SnapRSP STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/rsp/aspMain.cpp
)
target_include_directories(SnapRSP PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)
target_link_libraries(SnapRSP PRIVATE librecomp)

# ============================================================
# Main executable
# ============================================================
if(BUILD_PLAYING)
    set(POKEMONSNAP_SOURCES
        src/main/main.cpp
        src/main/register_overlays.cpp
        src/main/register_patches.cpp
        src/main/rt64_render_context.cpp
        src/game/input.cpp
        src/game/config.cpp
        src/game/recomp_api.cpp
        src/main/os_stubs.cpp
    )

    add_executable(PokemonSnapRecomp ${POKEMONSNAP_SOURCES})
    target_compile_definitions(PokemonSnapRecomp PRIVATE SDL_MAIN_HANDLED NOMINMAX)

    target_include_directories(PokemonSnapRecomp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/src
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/src/contrib/plume/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/src/contrib/hlslpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/src/contrib
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/src/contrib/mupen64plus-win32-deps/SDL2-2.26.3/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/RmlUi/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/lunasvg/include
        ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledFuncs
    )

    target_link_libraries(PokemonSnapRecomp PRIVATE
        RecompiledFuncs
        SnapRSP
        librecomp
        ultramodern
        rt64
        rmlui
        lunasvg
    )

    if(WIN32)
        target_link_libraries(PokemonSnapRecomp PRIVATE
            ${FREETYPE_LIBRARIES}
            dxgi d3d12 d3dcompiler
        )
    endif()

    set_target_properties(PokemonSnapRecomp PROPERTIES
        OUTPUT_NAME "PokemonSnap"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
