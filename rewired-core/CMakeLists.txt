cmake_minimum_required(VERSION 3.26)
project(twilit_rewired_controls CXX)
set(DUSKLIGHT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/dusklight" CACHE PATH "Dusklight source")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)
option(REWIRED_TEST_DRIVER "Build isolated virtual-controller test harness" OFF)
set(rewired_features game)
if(REWIRED_TEST_DRIVER)
    list(APPEND rewired_features webgpu)
endif()
add_mod(twilit_rewired_controls
    FEATURES ${rewired_features}
    SOURCES src/mod.cpp src/kcrux_controls.cpp src/manual_jump.cpp src/core_service.cpp src/settings_ui.cpp src/crash_compat.cpp
    MOD_JSON mod.json)
target_include_directories(twilit_rewired_controls PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(REWIRED_TEST_DRIVER)
    target_sources(twilit_rewired_controls PRIVATE src/kcrux_test_driver.cpp)
    target_compile_definitions(twilit_rewired_controls PRIVATE KCRUX_TEST_DRIVER=1)
endif()
