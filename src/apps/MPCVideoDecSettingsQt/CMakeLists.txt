# Standalone Qt6 Widgets companion app for the MPCVideoDec filter's
# settings - a side-by-side comparison with the WinUI3 version at
# src/apps/MPCVideoDecSettings. Deliberately not part of MPCVideoDec.sln:
# it talks to the filter only via the registry and the shared-memory status
# block in MPCVideoDecStatusShare.h, so it has no build-time dependency on
# the native filter and the native build has none on Qt.
#
# Configure with Qt6's install location on CMAKE_PREFIX_PATH, e.g.:
#   cmake -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64

cmake_minimum_required(VERSION 3.21)
project(MPCVideoDecSettingsQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(MPCVideoDecSettingsQt WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MPCVideoDecRegistry.h
)

target_link_libraries(MPCVideoDecSettingsQt PRIVATE Qt6::Widgets)

if(WIN32)
    get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET MPCVideoDecSettingsQt POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" $<TARGET_FILE:MPCVideoDecSettingsQt>
            COMMENT "Deploying Qt runtime DLLs next to the executable"
        )
    endif()
endif()
