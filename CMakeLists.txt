cmake_minimum_required(VERSION 3.20)
project(SickMenu LANGUAGES CXX)

if(WIN32)
    include(cmake/minhook.cmake)
endif()

add_library(sick_native STATIC
    src/core/hooking/DetourHook.cpp
    src/core/hooking/HookManager.cpp
    src/core/memory/MemoryManager.cpp
    src/core/memory/Module.cpp
    src/core/memory/ModuleManager.cpp
    src/core/memory/Pattern.cpp
    src/core/memory/PatternScanner.cpp
    src/game/enhanced/BuildManager.cpp
    src/game/enhanced/EnhancedGame.cpp
    src/game/enhanced/EnhancedPointers.cpp
    src/game/enhanced/NativeBootstrap.cpp
    src/game/enhanced/NativeCrossmap.cpp
    src/game/enhanced/NativeTable.cpp
    src/game/enhanced/ScriptGlobal.cpp
    src/game/natives/NativeHandlerTable.cpp
    src/game/natives/NativeRegistry.cpp
    src/game/natives/NativeResolver.cpp
    src/game/natives/NativeSystem.cpp
    src/game/scheduler/GameScheduler.cpp
    src/game/services/PlayerService.cpp
)

target_include_directories(sick_native PUBLIC src)
target_compile_features(sick_native PUBLIC cxx_std_20)

if(WIN32)
    target_include_directories(sick_native PRIVATE "${minhook_SOURCE_DIR}/include")
    target_link_libraries(sick_native PRIVATE minhook)
endif()

option(SICK_NATIVE_BUILD_TESTS "Build Sick native backend tests" OFF)

if(SICK_NATIVE_BUILD_TESTS)
    enable_testing()

    if(WIN32)
        add_executable(hook_manager_tests
            tests/hook_manager_tests.cpp
        )
        target_link_libraries(hook_manager_tests PRIVATE sick_native)
        target_compile_features(hook_manager_tests PRIVATE cxx_std_20)
        add_test(NAME hook_manager_tests COMMAND hook_manager_tests)
    endif()

    add_executable(enhanced_pointers_tests
        tests/enhanced_pointers_tests.cpp
    )
    target_link_libraries(enhanced_pointers_tests PRIVATE sick_native)
    target_compile_features(enhanced_pointers_tests PRIVATE cxx_std_20)
    add_test(NAME enhanced_pointers_tests COMMAND enhanced_pointers_tests)

    add_executable(memory_manager_tests
        tests/memory_manager_tests.cpp
    )
    target_link_libraries(memory_manager_tests PRIVATE sick_native)
    target_compile_features(memory_manager_tests PRIVATE cxx_std_20)
    add_test(NAME memory_manager_tests COMMAND memory_manager_tests)

    add_executable(native_crossmap_tests
        tests/native_crossmap_tests.cpp
    )
    target_link_libraries(native_crossmap_tests PRIVATE sick_native)
    target_compile_features(native_crossmap_tests PRIVATE cxx_std_20)
    add_test(NAME native_crossmap_tests COMMAND native_crossmap_tests)

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
endif()
