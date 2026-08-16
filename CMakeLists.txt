cmake_minimum_required(VERSION 4.2)
project(SickMenu VERSION 0.2.0 LANGUAGES CXX)

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
    src/ui/menu/Menu.cpp
    src/ui/menu/MenuRenderer.cpp
    src/ui/menu/SickMenu.cpp
)

target_include_directories(sick_native PUBLIC src)
target_compile_features(sick_native PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(sick_native PRIVATE /W4 /permissive- /EHsc)
endif()

option(SICK_MENU_BUILD_DLL "Build the injectable GTA V Enhanced menu DLL" ${WIN32})

if(SICK_MENU_BUILD_DLL)
    if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "SickMenu.dll requires 64-bit Windows")
    endif()

    include(FetchContent)

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
    )
    target_include_directories(SickMenu PRIVATE src)
    target_compile_features(SickMenu PRIVATE cxx_std_20)
    target_compile_definitions(SickMenu PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_compile_options(SickMenu PRIVATE /W4 /permissive- /EHsc)
    target_link_libraries(SickMenu PRIVATE
        sick_native
        sick_imgui
        minhook
        d3d12
        dxgi
        user32
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
endif()
