cmake_minimum_required(VERSION 3.25)
project(ClapWorkbenchPluginProcess VERSION 0.1 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(clap         CONFIG REQUIRED)
find_package(clap-helpers CONFIG REQUIRED)


qt_add_executable(${PROJECT_NAME}
        src/native/NativePluginGUI_macOS.mm
        src/Utils.cpp
        src/PluginHost.cpp
        src/PluginLibrary.cpp
        src/PluginManager.cpp
        src/main.cpp
)


target_include_directories(${PROJECT_NAME} PRIVATE ${choc_SOURCE_DIR} "src")

if (APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
