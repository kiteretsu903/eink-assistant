cmake_minimum_required(VERSION 3.16)
project(EinkAssistant VERSION 1.1.0 LANGUAGES CXX)

if(WIN32)
    enable_language(RC)

    find_program(EINK_CSC_EXECUTABLE csc.exe
        PATHS
            "$ENV{WINDIR}/Microsoft.NET/Framework64/v4.0.30319"
            "$ENV{WINDIR}/Microsoft.NET/Framework/v4.0.30319"
        NO_DEFAULT_PATH)
    file(GLOB EINK_UIAUTOMATION_CLIENT "$ENV{WINDIR}/Microsoft.NET/assembly/GAC_MSIL/UIAutomationClient/*/UIAutomationClient.dll")
    file(GLOB EINK_UIAUTOMATION_TYPES "$ENV{WINDIR}/Microsoft.NET/assembly/GAC_MSIL/UIAutomationTypes/*/UIAutomationTypes.dll")
    file(GLOB EINK_UIAUTOMATION_PROVIDER "$ENV{WINDIR}/Microsoft.NET/assembly/GAC_MSIL/UIAutomationProvider/*/UIAutomationProvider.dll")
    if(NOT EINK_CSC_EXECUTABLE OR NOT EINK_UIAUTOMATION_CLIENT OR NOT EINK_UIAUTOMATION_TYPES OR NOT EINK_UIAUTOMATION_PROVIDER)
        message(FATAL_ERROR "The Windows Night Light helper requires the built-in .NET Framework UI Automation assemblies.")
    endif()
    list(GET EINK_UIAUTOMATION_CLIENT 0 EINK_UIAUTOMATION_CLIENT_DLL)
    list(GET EINK_UIAUTOMATION_TYPES 0 EINK_UIAUTOMATION_TYPES_DLL)
    list(GET EINK_UIAUTOMATION_PROVIDER 0 EINK_UIAUTOMATION_PROVIDER_DLL)
    set(EINK_NIGHT_LIGHT_HELPER "${CMAKE_BINARY_DIR}/bin/EinkNightLightControl.exe")
    file(TO_NATIVE_PATH "${EINK_NIGHT_LIGHT_HELPER}" EINK_NIGHT_LIGHT_HELPER_NATIVE)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/NightLightControl.cs" EINK_NIGHT_LIGHT_SOURCE_NATIVE)
    file(TO_NATIVE_PATH "${EINK_UIAUTOMATION_CLIENT_DLL}" EINK_UIAUTOMATION_CLIENT_NATIVE)
    file(TO_NATIVE_PATH "${EINK_UIAUTOMATION_TYPES_DLL}" EINK_UIAUTOMATION_TYPES_NATIVE)
    file(TO_NATIVE_PATH "${EINK_UIAUTOMATION_PROVIDER_DLL}" EINK_UIAUTOMATION_PROVIDER_NATIVE)
    add_custom_command(
        OUTPUT "${EINK_NIGHT_LIGHT_HELPER}"
        COMMAND "${EINK_CSC_EXECUTABLE}" /nologo /target:exe /optimize+
            "/out:${EINK_NIGHT_LIGHT_HELPER_NATIVE}"
            "/reference:${EINK_UIAUTOMATION_CLIENT_NATIVE}"
            "/reference:${EINK_UIAUTOMATION_TYPES_NATIVE}"
            "/reference:${EINK_UIAUTOMATION_PROVIDER_NATIVE}"
            "${EINK_NIGHT_LIGHT_SOURCE_NATIVE}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/NightLightControl.cs"
        VERBATIM)
    add_custom_target(eink_night_light_helper ALL DEPENDS "${EINK_NIGHT_LIGHT_HELPER}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Test)

set(EINK_CORE_SOURCES
    src/core/AppState.cpp
    src/core/AppState.h
    src/core/IccProfile.cpp
    src/core/IccProfile.h
    src/core/NightLightStateCodec.cpp
    src/core/NightLightStateCodec.h
    src/core/SettingsStore.cpp
    src/core/SettingsStore.h
    src/core/ToneCurve.cpp
    src/core/ToneCurve.h
    src/platform/PlatformServices.h
)

add_library(eink_core STATIC ${EINK_CORE_SOURCES})
target_include_directories(eink_core PUBLIC src)
target_link_libraries(eink_core PUBLIC Qt5::Core Qt5::Gui)

set(EINK_APP_SOURCES
    src/app/ApplicationController.cpp
    src/app/ApplicationController.h
    src/ui/CurvePlot.cpp
    src/ui/CurvePlot.h
    src/ui/BusyDialog.cpp
    src/ui/BusyDialog.h
    src/ui/DisplayCard.cpp
    src/ui/DisplayCard.h
    src/ui/EinkSwitch.cpp
    src/ui/EinkSwitch.h
    src/ui/Localization.cpp
    src/ui/Localization.h
    src/ui/MainPanel.cpp
    src/ui/MainPanel.h
    src/ui/SmoothLabel.cpp
    src/ui/SmoothLabel.h
    src/ui/TrayIcon.cpp
    src/ui/TrayIcon.h
    src/ui/UiStyle.cpp
    src/ui/UiStyle.h
    src/ui/WelcomeDialog.cpp
    src/ui/WelcomeDialog.h
    src/resources.qrc
)

if(WIN32)
    list(APPEND EINK_APP_SOURCES
        src/platform/windows/WindowsPlatformServices.cpp
        src/platform/windows/WindowsPlatformServices.h
        src/platform/windows/WindowsNative.h
        src/platform/windows/WindowsCompatibility.cpp
        src/platform/windows/WindowsCompatibility.h
        src/platform/windows/WindowsGpuControlPanel.cpp
        src/platform/windows/WindowsGpuControlPanel.h
    )
endif()

add_library(eink_app STATIC ${EINK_APP_SOURCES})
target_include_directories(eink_app PUBLIC src)
target_link_libraries(eink_app PUBLIC eink_core Qt5::Core Qt5::Gui Qt5::Widgets)
if(WIN32)
    target_compile_definitions(eink_app PRIVATE _WIN32_WINNT=0x0601 WINVER=0x0601 NOMINMAX UNICODE _UNICODE)
    target_link_libraries(eink_app PRIVATE user32 gdi32 advapi32 shell32 shlwapi dxgi mscms dwmapi taskschd ole32 oleaut32)
endif()

add_executable(EinkAssistant WIN32 src/main.cpp)
if(WIN32)
    add_dependencies(EinkAssistant eink_night_light_helper)
    set(EINK_WINDOWS_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/app.manifest")
    target_sources(EinkAssistant PRIVATE src/platform/windows/app.rc ${EINK_WINDOWS_MANIFEST})
    set_source_files_properties(${EINK_WINDOWS_MANIFEST} PROPERTIES HEADER_FILE_ONLY TRUE)
    set_property(SOURCE src/platform/windows/app.rc APPEND PROPERTY OBJECT_DEPENDS ${EINK_WINDOWS_MANIFEST})
endif()
target_link_libraries(EinkAssistant PRIVATE eink_app)

enable_testing()
add_executable(eink_core_tests tests/CoreTests.cpp tests/FakePlatform.cpp tests/FakePlatform.h)
target_link_libraries(eink_core_tests PRIVATE eink_app Qt5::Test)
add_test(NAME core COMMAND eink_core_tests)

add_executable(eink_e2e_tests tests/E2ETests.cpp tests/FakePlatform.cpp tests/FakePlatform.h)
target_link_libraries(eink_e2e_tests PRIVATE eink_app Qt5::Test Qt5::Widgets)
add_test(NAME e2e COMMAND eink_e2e_tests -platform offscreen)

if(WIN32)
    add_executable(eink_hardware_smoke tests/HardwareSmoke.cpp)
    target_link_libraries(eink_hardware_smoke PRIVATE eink_app)

    add_executable(eink_night_light_smoke tests/NightLightSmoke.cpp)
    target_link_libraries(eink_night_light_smoke PRIVATE eink_app)

    add_executable(eink_launch_task_self_test WIN32 tests/LaunchTaskSelfTest.cpp)
    target_sources(eink_launch_task_self_test PRIVATE src/platform/windows/app.rc ${EINK_WINDOWS_MANIFEST})
    set_source_files_properties(${EINK_WINDOWS_MANIFEST} PROPERTIES HEADER_FILE_ONLY TRUE)
    set_property(SOURCE src/platform/windows/app.rc APPEND PROPERTY OBJECT_DEPENDS ${EINK_WINDOWS_MANIFEST})
    target_link_libraries(eink_launch_task_self_test PRIVATE eink_app)
endif()
