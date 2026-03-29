cmake_minimum_required(VERSION 3.20)

project(PokemonSnapRecomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_PLAYING "Build the main game executable" ON)

# N64ModernRuntime (contains librecomp, ultramodern, and the N64Recomp tool)
add_subdirectory(lib/N64ModernRuntime)

# RT64 renderer
add_subdirectory(lib/rt64)

# RmlUi for menus
set(RMLUI_BACKEND "" CACHE STRING "" FORCE)
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
        set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/release static/win64/freetype.lib")
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
set_target_properties(RecompiledFuncs PROPERTIES
    C_STANDARD 11
    COMPILE_FLAGS "$<$<COMPILE_LANGUAGE:C>:-w>"
)

# ============================================================
# Patches library (output of N64Recomp on patches.elf)
# ============================================================

# Custom target to build patches MIPS ELF
add_custom_target(PatchesBuild
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/patches
    COMMENT "Building MIPS patches..."
)

# Custom command to run N64Recomp on patches
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches/patches.c
        ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches/patches_bin.c
        ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches/recomp_overlays.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches/funcs.h
    COMMAND $<TARGET_FILE:N64Recomp> ${CMAKE_CURRENT_SOURCE_DIR}/patches.toml
    DEPENDS PatchesBuild
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Recompiling patches..."
)

add_library(PatchesLib STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches/patches.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches/patches_bin.c
)
target_include_directories(PatchesLib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)
target_link_libraries(PatchesLib PRIVATE librecomp)
set_target_properties(PatchesLib PROPERTIES
    C_STANDARD 11
    COMPILE_FLAGS "$<$<COMPILE_LANGUAGE:C>:-w>"
)

# ============================================================
# RSP recompiled audio microcode
# ============================================================
add_library(RSPRecomp STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/rsp/aspMain.cpp
)
target_include_directories(RSPRecomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)
target_link_libraries(RSPRecomp PRIVATE librecomp)

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
    )

    add_executable(PokemonSnapRecomp WIN32 ${POKEMONSNAP_SOURCES})

    target_include_directories(PokemonSnapRecomp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rt64/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/RmlUi/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/lunasvg/include
        ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledPatches
    )

    # Link order matters: PatchesLib before RecompiledFuncs so patches override
    target_link_libraries(PokemonSnapRecomp PRIVATE
        PatchesLib
        RecompiledFuncs
        RSPRecomp
        librecomp
        ultramodern
        rt64
        RmlCore
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
