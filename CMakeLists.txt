cmake_minimum_required(VERSION 3.16)
project(EinkAssistant VERSION 2.3.0 LANGUAGES CXX)

if(WIN32)
    enable_language(RC)
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
        src/platform/windows/WindowsTrayIntegration.cpp
        src/platform/windows/WindowsTrayIntegration.h
    )
endif()

add_library(eink_app STATIC ${EINK_APP_SOURCES})
target_include_directories(eink_app PUBLIC src)
target_link_libraries(eink_app PUBLIC eink_core Qt5::Core Qt5::Gui Qt5::Widgets)
if(WIN32)
    target_compile_definitions(eink_app PRIVATE _WIN32_WINNT=0x0601 WINVER=0x0601 NOMINMAX UNICODE _UNICODE)
    target_link_libraries(eink_app PRIVATE user32 gdi32 advapi32 shell32 mscms dwmapi taskschd ole32 oleaut32)
endif()

add_executable(EinkAssistant WIN32 src/main.cpp)
if(WIN32)
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
