cmake_minimum_required(VERSION 3.28)

project(SickMenu VERSION 0.1.0 LANGUAGES CXX)

add_library(SickMenu SHARED
    src/bootstrap/dllmain.cpp
    src/core/application.cpp
    src/core/core.cpp
    src/core/logger.cpp
    src/core/scheduler.cpp
    src/core/build_info.cpp
    src/memory/module.cpp
    src/memory/pattern_scanner.cpp
)

target_include_directories(SickMenu PRIVATE include)
target_compile_features(SickMenu PRIVATE cxx_std_20)
target_compile_definitions(SickMenu PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(SickMenu PRIVATE Version)

if(MSVC)
    target_compile_options(SickMenu PRIVATE /W4 /permissive- /EHsc)
    set_property(TARGET SickMenu PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set_target_properties(SickMenu PROPERTIES
    OUTPUT_NAME "SickMenu"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
