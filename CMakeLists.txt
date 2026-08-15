cmake_minimum_required(VERSION 3.20)
project(SickMenu LANGUAGES CXX)

find_package(Threads REQUIRED)

add_library(sick_native STATIC
    src/core/diagnostics/Assertions.cpp
    src/core/diagnostics/CrashReporter.cpp
    src/core/diagnostics/HealthMonitor.cpp
    src/core/diagnostics/Metrics.cpp
    src/core/diagnostics/StackTrace.cpp
    src/core/diagnostics/Trace.cpp
    src/core/logging/AnomalyDetector.cpp
    src/core/logging/LogContext.cpp
    src/core/logging/Logger.cpp
    src/core/logging/RateLimiter.cpp
    src/core/logging/sinks/ConsoleSink.cpp
    src/core/logging/sinks/DebuggerSink.cpp
    src/core/logging/sinks/FileSink.cpp
    src/core/logging/sinks/MemorySink.cpp
    src/game/enhanced/BuildManager.cpp
    src/game/enhanced/EnhancedGame.cpp
    src/game/enhanced/NativeBootstrap.cpp
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
target_link_libraries(sick_native PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(sick_native PUBLIC dbghelp)
endif()

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

    add_executable(reaper_observability_tests
        tests/reaper_observability_tests.cpp
    )
    target_link_libraries(reaper_observability_tests PRIVATE sick_native)
    target_compile_features(reaper_observability_tests PRIVATE cxx_std_20)
    add_test(NAME reaper_observability_tests COMMAND reaper_observability_tests)
endif()
