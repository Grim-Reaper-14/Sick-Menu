cmake_minimum_required(VERSION 4.2)
project(SickMenu VERSION 0.7.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(sick_native STATIC
    src/backend/BackendApi.cpp
    src/backend/BackendCore.cpp
    src/backend/calls/GameCallHub.cpp
    src/backend/features/FeatureManager.cpp
    src/backend/features/player/PlayerFeatures.cpp
    src/backend/features/vehicle/VehicleFeatures.cpp
    src/backend/features/vehicle/handling/HandlingFeatures.cpp
    src/backend/system/AssetCatalog.cpp
    src/backend/system/BackgroundCore.cpp
    src/backend/system/ConfigManager.cpp
    src/backend/system/FileSystem.cpp
    src/backend/system/HandlingProfileManager.cpp
    src/backend/system/IoService.cpp
    src/backend/system/Logger.cpp
    src/backend/system/LoggerApi.cpp
    src/backend/system/PerformanceMonitor.cpp
    src/backend/system/SettingsManager.cpp
    src/backend/system/logging/EmergencyLogWriter.cpp
    src/backend/system/logging/LoggingEngine.cpp
    src/backend/tasking/GameFiberScheduler.cpp
    src/backend/tasking/ThreadPool.cpp
    src/frontend/FrontendCore.cpp
    src/game/enhanced/BuildManager.cpp
    src/game/enhanced/EnhancedGame.cpp
    src/game/enhanced/EnhancedScriptHost.cpp
    src/game/enhanced/NativeBootstrap.cpp
    src/game/enhanced/NativeTable.cpp
    src/game/enhanced/ScriptGlobal.cpp
    src/game/handling/HandlingBackend.cpp
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
    src/game/services/VehicleService.cpp
    src/ui/menu/Menu.cpp
    src/ui/menu/MenuRenderer.cpp
    src/ui/menu/SickMenu.cpp
)

target_include_directories(sick_native PUBLIC src)
target_compile_features(sick_native PUBLIC cxx_std_20)
target_link_libraries(sick_native PRIVATE nlohmann_json::nlohmann_json)

if(WIN32)
    target_compile_definitions(sick_native PUBLIC _WIN32_WINNT=0x0601)
    target_compile_definitions(sick_native PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(MSVC)
    target_compile_options(sick_native PRIVATE /W4 /permissive- /EHsc)
endif()

option(SICK_MENU_BUILD_DLL "Build the injectable GTA V Enhanced menu DLL" ${WIN32})

if(SICK_MENU_BUILD_DLL)
    if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "SickMenu.dll requires 64-bit Windows")
    endif()

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.8
        GIT_SHALLOW TRUE
    )
    FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG v1.3.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui minhook)

    add_library(sick_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
    target_include_directories(sick_imgui PUBLIC ${imgui_SOURCE_DIR})
    target_compile_definitions(sick_imgui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

    add_library(SickMenu MODULE
        src/runtime/DllMain.cpp
        src/runtime/Dx12Renderer.cpp
        src/runtime/GtaRuntime.cpp
        src/runtime/PatternScanner.cpp
        src/runtime/RuntimeLog.cpp
    )
    target_include_directories(SickMenu PRIVATE src)
    target_compile_features(SickMenu PRIVATE cxx_std_20)
    target_compile_definitions(SickMenu PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0601)
    target_compile_options(SickMenu PRIVATE /W4 /permissive- /EHsc)
    target_link_libraries(SickMenu PRIVATE
        sick_native
        sick_imgui
        minhook
        d3d12
        dxgi
        user32
        ole32
        windowscodecs
    )
    set_target_properties(SickMenu PROPERTIES
        OUTPUT_NAME "SickMenu"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
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

    add_executable(sick_menu_tests
        tests/menu_tests.cpp
    )
    target_link_libraries(sick_menu_tests PRIVATE sick_native)
    target_compile_features(sick_menu_tests PRIVATE cxx_std_20)
    add_test(NAME sick_menu_tests COMMAND sick_menu_tests)

    add_executable(vehicle_feature_tests
        tests/vehicle_feature_tests.cpp
    )
    target_link_libraries(vehicle_feature_tests PRIVATE sick_native)
    target_compile_features(vehicle_feature_tests PRIVATE cxx_std_20)
    add_test(NAME vehicle_feature_tests COMMAND vehicle_feature_tests)

    add_executable(handling_backend_tests
        tests/handling_backend_tests.cpp
    )
    target_link_libraries(handling_backend_tests PRIVATE sick_native)
    target_compile_features(handling_backend_tests PRIVATE cxx_std_20)
    add_test(NAME handling_backend_tests COMMAND handling_backend_tests)

    add_executable(handling_feature_tests
        tests/handling_feature_tests.cpp
    )
    target_link_libraries(handling_feature_tests PRIVATE sick_native)
    target_compile_features(handling_feature_tests PRIVATE cxx_std_20)
    add_test(NAME handling_feature_tests COMMAND handling_feature_tests)

    add_executable(handling_profile_tests
        tests/handling_profile_tests.cpp
    )
    target_link_libraries(handling_profile_tests PRIVATE sick_native)
    target_compile_features(handling_profile_tests PRIVATE cxx_std_20)
    add_test(NAME handling_profile_tests COMMAND handling_profile_tests)

    add_executable(handling_menu_tests
        tests/handling_menu_tests.cpp
    )
    target_link_libraries(handling_menu_tests PRIVATE sick_native)
    target_compile_features(handling_menu_tests PRIVATE cxx_std_20)
    add_test(NAME handling_menu_tests COMMAND handling_menu_tests)

    add_executable(backend_tasking_tests
        tests/backend_tasking_tests.cpp
    )
    target_link_libraries(backend_tasking_tests PRIVATE sick_native)
    target_compile_features(backend_tasking_tests PRIVATE cxx_std_20)
    add_test(NAME backend_tasking_tests COMMAND backend_tasking_tests)

    add_executable(backend_config_tests
        tests/backend_config_tests.cpp
    )
    target_link_libraries(backend_config_tests PRIVATE sick_native)
    target_compile_features(backend_config_tests PRIVATE cxx_std_20)
    add_test(NAME backend_config_tests COMMAND backend_config_tests)

    add_executable(background_system_tests
        tests/background_system_tests.cpp
    )
    target_link_libraries(background_system_tests PRIVATE sick_native)
    target_compile_features(background_system_tests PRIVATE cxx_std_20)
    add_test(NAME background_system_tests COMMAND background_system_tests)

    set_tests_properties(
        sick_native_tests
        sick_enhanced_backend_tests
        reaper_indexed_backend_tests
        script_function_tests
        enhanced_script_host_tests
        sick_menu_tests
        vehicle_feature_tests
        handling_backend_tests
        handling_feature_tests
        handling_profile_tests
        handling_menu_tests
        backend_tasking_tests
        backend_config_tests
        background_system_tests
        PROPERTIES TIMEOUT 30
    )
endif()
