cmake_minimum_required(VERSION 3.16)
project(MarkdownViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Security flags
if(MSVC)
    add_compile_options(/W4 /WX- /sdl /guard:cf)
    add_link_options(/guard:cf)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter
        -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE)
    add_link_options(-pie -Wl,-z,relro,-z,now)
endif()

# Qt6 - fallback to Qt5
find_package(Qt6 COMPONENTS Core Widgets Gui Charts Svg PrintSupport QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 COMPONENTS Core Widgets Gui Charts Svg PrintSupport REQUIRED)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
endif()

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MarkdownParser.cpp
    src/MarkdownRenderer.cpp
    src/TocWidget.cpp
    src/ChartRenderer.cpp
    src/ImageViewer.cpp
    src/ThemeManager.cpp
)

set(HEADERS
    src/MainWindow.h
    src/MarkdownParser.h
    src/MarkdownRenderer.h
    src/TocWidget.h
    src/ChartRenderer.h
    src/ImageViewer.h
    src/ThemeManager.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

if(Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME}
        Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Charts Qt6::Svg Qt6::PrintSupport)
else()
    target_link_libraries(${PROJECT_NAME}
        Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Charts Qt5::Svg Qt5::PrintSupport)
endif()

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Windows: copy Qt DLLs
if(WIN32)
    if(Qt6_FOUND)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    else()
        get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
    endif()
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-system-d3d-compiler
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running windeployqt...")
    endif()
endif()
