cmake_minimum_required(VERSION 3.20)
project(MetaTrain VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    CACHE PATH "Path to ImGui headers+sources from the Nexus SDK")

add_library(MetaTrain SHARED
    src/dllmain.cpp
    src/events.cpp
    src/state.cpp
    src/ui.cpp
    src/chat.cpp
    # ImGui compiled directly into DLL for cross-compile (Lessons_Learned §29)
    ${IMGUI_INCLUDE_DIR}/imgui.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_tables.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp
)

target_include_directories(MetaTrain PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # Nexus.h at project root
    ${IMGUI_INCLUDE_DIR}          # imgui.h
    src
)

set_target_properties(MetaTrain PROPERTIES
    PREFIX      ""
    SUFFIX      ".dll"
    OUTPUT_NAME "MetaTrain"
)

target_compile_definitions(MetaTrain PRIVATE
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# Static-link MinGW runtimes — prevents Error 126 on a clean GW2 install (Lessons_Learned §4)
target_link_libraries(MetaTrain PRIVATE
    -static-libgcc
    -static-libstdc++
    -Wl,-Bstatic,--whole-archive,-lwinpthread,--no-whole-archive
)
