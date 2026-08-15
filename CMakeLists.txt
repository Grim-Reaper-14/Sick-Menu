cmake_minimum_required(VERSION 3.20)
project(SickMenu LANGUAGES CXX)

add_library(sick_native STATIC
    src/game/natives/NativeResolver.cpp
)

target_include_directories(sick_native PUBLIC src)
target_compile_features(sick_native PUBLIC cxx_std_20)

option(SICK_NATIVE_BUILD_TESTS "Build Sick native backend tests" OFF)

if(SICK_NATIVE_BUILD_TESTS)
    enable_testing()

    add_executable(sick_native_tests
        tests/native_invoker_tests.cpp
    )

    target_link_libraries(sick_native_tests PRIVATE sick_native)
    target_compile_features(sick_native_tests PRIVATE cxx_std_20)

    add_test(NAME sick_native_tests COMMAND sick_native_tests)
endif()
