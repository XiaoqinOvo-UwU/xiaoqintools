cmake_minimum_required(VERSION 3.21)

project(XiaoQinTools VERSION 3.5.16 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 required modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Widgets Network Svg QuickControls2 Concurrent)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

# ---- core app sources ----
set(APP_SOURCES
    src/main.cpp
    src/Core/AppCore.h
    src/Core/AppCore.cpp
    src/Services/ProxyService.h
    src/Services/ProxyService.cpp
    src/Services/NetworkService.h
    src/Services/NetworkService.cpp
    src/Services/SystemService.h
    src/Services/SystemService.cpp
    src/Services/MoodService.h
    src/Services/MoodService.cpp
    src/Services/ConfigService.h
    src/Services/ConfigService.cpp
    src/Services/UpdateService.h
    src/Services/UpdateService.cpp
    src/Services/SyncService.h
    src/Services/SyncService.cpp
    src/Services/PluginManager.h
    src/Services/PluginManager.cpp
    src/Services/AiService.h
    src/Services/AiService.cpp
    src/Services/StatsService.h
    src/Services/StatsService.cpp
    src/Services/ContactService.h
    src/Services/ContactService.cpp
)

qt_add_executable(XiaoQinTools WIN32
    ${APP_SOURCES}
    qml/Main.qml
    qml/Components/
    qml/Pages/
    resources/resources.qrc
)

# embed the app icon (cute icon) into the exe
if(WIN32)
    target_sources(XiaoQinTools PRIVATE resources/app.rc)
endif()

target_compile_definitions(XiaoQinTools PRIVATE
    QT_DEPRECATED_WARNINGS
    "NIGHTLOG_VERSION=\"${PROJECT_VERSION}\""
)

target_link_libraries(XiaoQinTools PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::QuickControls2
    Qt6::Concurrent
    winmm
)

# QML deployment
qt_finalize_executable(XiaoQinTools)


















