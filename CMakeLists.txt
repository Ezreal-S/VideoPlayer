cmake_minimum_required(VERSION 3.16)

project(Ezreal-Player VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Ezreal-Player
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Ezreal-Player APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Ezreal-Player SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Ezreal-Player
            ${PROJECT_SOURCES}
            ctrlbar.h ctrlbar.cpp ctrlbar.ui
            videoslider.h videoslider.cpp
            player.h player.cpp
            videowidget.h videowidget.cpp
            audioplayer.h audioplayer.cpp
        )
    endif()
endif()

target_link_libraries(Ezreal-Player PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(Ezreal-Player PRIVATE ${CMAKE_SOURCE_DIR}/3rdParty/ffmpeg/Win64/include
                                                    ${CMAKE_SOURCE_DIR}/3rdParty/SDL2/Win64/include)
target_link_directories(Ezreal-Player PRIVATE
                        ${CMAKE_SOURCE_DIR}/3rdParty/ffmpeg/Win64/lib
                        ${CMAKE_SOURCE_DIR}/3rdParty/SDL2/Win64/lib)
target_link_libraries(Ezreal-Player PRIVATE SDL2 SDL2main avcodec avformat avutil swresample swscale)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Ezreal-Player)
endif()
set_target_properties(Ezreal-Player PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)
include(GNUInstallDirs)
install(TARGETS Ezreal-Player
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Ezreal-Player)
endif()
