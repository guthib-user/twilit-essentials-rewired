cmake_minimum_required(VERSION 3.26)
project(twilit_rewired_hud CXX)
set(DUSKLIGHT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/dusklight" CACHE PATH "Dusklight source")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)
add_mod(twilit_rewired_hud
    FEATURES game
    SOURCES src/mod.cpp src/kcrux_prompts.cpp src/core_adapter.cpp src/config.cpp
    MOD_JSON mod.json
    RES_DIR res)
target_include_directories(twilit_rewired_hud PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../rewired-core/include")
