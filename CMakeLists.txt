cmake_minimum_required(VERSION 3.20)
project(CinematicShatteredGlass CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Plugin DLL ───────────────────────────────────────────────────────────────
add_library(CinematicShatteredGlass SHARED
    src/CinematicShatteredGlass.cpp
    src/CinematicShatteredGlass.def   # export ordinals for FFGL host
)

target_include_directories(CinematicShatteredGlass PRIVATE
    src         # Shaders.h, glext.h
    FFGLSDK     # FFGL.h
)

target_link_libraries(CinematicShatteredGlass PRIVATE opengl32)

# Resolume requires no 'lib' prefix and the name exactly as-is
set_target_properties(CinematicShatteredGlass PROPERTIES
    PREFIX ""
    OUTPUT_NAME "CinematicShatteredGlass"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ─── MSVC flags ───────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(CinematicShatteredGlass PRIVATE
        /W3     # warnings
        /MP     # parallel compilation
        /O2     # optimise for speed
        /GL     # whole-program optimisation
    )
    target_compile_definitions(CinematicShatteredGlass PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    # Link-time code generation to go with /GL
    target_link_options(CinematicShatteredGlass PRIVATE /LTCG)
endif()

# ─── Status ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "  cmake --build . --config Release")
message(STATUS "  → bin/Release/CinematicShatteredGlass.dll")
message(STATUS "")
