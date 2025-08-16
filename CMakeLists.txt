cmake_minimum_required(VERSION 3.25)
project(ClapWorkbench VERSION 0.1 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)
setupDependencies(0.0.1 Darwin arm64)

find_package(Qt6 6.9.1 COMPONENTS Quick CONFIG REQUIRED)
find_package(RtAudio      CONFIG REQUIRED)
find_package(RtMidi       CONFIG REQUIRED)
find_package(clap         CONFIG REQUIRED)
find_package(clap-helpers CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.9.1)

qt_add_executable(${PROJECT_NAME}
        src/Utils/RecursiveFileSystemWatcher.cpp
        src/Utils/RecursiveFileSystemWatcher.h
        src/Nodes/Node.h
        src/Nodes/Node.cpp
        src/Nodes/MidiFilePlayer.cpp
        src/Nodes/ChannelStrip.cpp
        src/Nodes/PluginHost.h
        src/Nodes/PluginHost.cpp
        src/PluginLibrary.h
        src/PluginLibrary.cpp
        src/PluginInfo.h
        src/Components/PluginQuickView.h
        src/Components/PluginQuickView.cpp
        src/Utils.h
        src/Utils.cpp
        src/ParameterModel.cpp
        src/ParameterModel.h
        src/PluginManager.h
        src/PluginManager.cpp
        src/AudioEngine.h
        src/AudioEngine.cpp
        src/Commands.h
        src/Commands.cpp
        src/App.h
        src/App.cpp
        src/Main.cpp
)

qt_add_resources(${PROJECT_NAME} "res"
    PREFIX "/"
    FILES
        qml/Components/icons.ttf
)

qt_add_qml_module(${PROJECT_NAME}
    URI ClapWorkbench
    VERSION 1.0
    IMPORTS
        QtQuick
        QtQuick.Controls.Basic
        QtQuick.Window/2.2
        QtQuick.Dialogs
        QtQml.WorkerScript
    QML_FILES
        qml/Components/Button.qml
        qml/Components/ComboBox.qml
        qml/Components/Fader.qml
        qml/Components/MidiFilePlayer.qml
        qml/Components/GenericPluginGUI.qml
        qml/Components/LightButton.qml
        qml/Components/MainMenuWindow.qml
        qml/Components/MainMenuWindowItem.qml
        qml/Components/MainWindowPluginGUI.qml
        qml/Components/NativePluginGUI.qml
        qml/Components/PluginBrowserListItem.qml
        qml/Components/PluginBrowserListItemText.qml
        qml/Components/PluginBrowserWindow.qml
        qml/Components/PluginGUI.qml
        qml/Components/PluginSlot.qml
        qml/Components/ReorderableListView.qml
        qml/Components/Slider.qml
        qml/TopBar.qml
        qml/ChannelStrip.qml
        qml/Main.qml
)

qt_import_qml_plugins(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "src" "src/Components" "src/Nodes" ${CMAKE_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    RtMidi::rtmidi
    RtAudio::rtaudio
    Qt6::Quick
)

if (APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(${PROJECT_NAME} PUBLIC
            "-framework Cocoa"
            "-framework WebKit")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    QT_QMLCACHEGEN_ARGUMENTS "--only-bytecode"
    QT_QMLTC_EXPORT_ALL_SYMBOLS TRUE  # Needed for static builds

    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.witte.${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE

    WIN32_EXECUTABLE TRUE
)
