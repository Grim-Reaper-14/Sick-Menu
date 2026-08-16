cmake_minimum_required(VERSION 3.20)
project(SickMenu LANGUAGES CXX)

add_library(sick_native STATIC
    src/game/enhanced/BuildManager.cpp
    src/game/enhanced/EnhancedGame.cpp
    src/game/enhanced/EnhancedScriptHost.cpp
    src/game/enhanced/NativeBootstrap.cpp
    src/game/enhanced/NativeTable.cpp
    src/game/enhanced/ScriptGlobal.cpp
    src/game/natives/NativeHandlerTable.cpp
    src/game/natives/NativeRegistry.cpp
    src/game/natives/NativeResolver.cpp
    src/game/natives/NativeSystem.cpp
    src/game/scheduler/GameScheduler.cpp
    src/game/scripts/ScriptFunction.cpp
    src/game/scripts/ScriptFunctionCatalog.cpp
    src/game/scripts/ScriptPointer.cpp
    src/game/scripts/ScriptRuntime.cpp
    src/game/services/PlayerService.cpp
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

    add_executable(sick_enhanced_backend_tests
        tests/enhanced_backend_tests.cpp
    )
    target_link_libraries(sick_enhanced_backend_tests PRIVATE sick_native)
    target_compile_features(sick_enhanced_backend_tests PRIVATE cxx_std_20)
    add_test(NAME sick_enhanced_backend_tests COMMAND sick_enhanced_backend_tests)

    add_executable(reaper_indexed_backend_tests
        tests/reaper_indexed_backend_tests.cpp
    )
    target_link_libraries(reaper_indexed_backend_tests PRIVATE sick_native)
    target_compile_features(reaper_indexed_backend_tests PRIVATE cxx_std_20)
    add_test(NAME reaper_indexed_backend_tests COMMAND reaper_indexed_backend_tests)

    add_executable(script_function_tests
        tests/script_function_tests.cpp
    )
    target_link_libraries(script_function_tests PRIVATE sick_native)
    target_compile_features(script_function_tests PRIVATE cxx_std_20)
    add_test(NAME script_function_tests COMMAND script_function_tests)

    add_executable(enhanced_script_host_tests
        tests/enhanced_script_host_tests.cpp
    )
    target_link_libraries(enhanced_script_host_tests PRIVATE sick_native)
    target_compile_features(enhanced_script_host_tests PRIVATE cxx_std_20)
    add_test(NAME enhanced_script_host_tests COMMAND enhanced_script_host_tests)
endif()
