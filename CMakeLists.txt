cmake_minimum_required(VERSION 3.16)

project(SonaeLabelMaker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Gui Core Widgets Svg Xml Quick QuickControls2)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Core Widgets Svg Xml Quick QuickControls2)

# sets default startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SonaeLabelMaker)

if (MSVC)
    add_compile_options(/MP) # Multi-threaded compilation
    add_compile_options(/permissive- /W3 /WX) # Enforce stricter conformance to C++ standard
    add_compile_options(/wd4244 /wd4267 /wd4101 /wd4996)
endif()


file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.qml"
    "resources.qrc"
)

file(GLOB UI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui")
file(GLOB STYLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/res/*.qss")

# Add csv-parser header files
#add_subdirectory(csv-parser)

if (EMSCRIPTEN)
    include_directories($ENV{EMSDK}/upstream/emscripten/cache/sysroot/include)
endif()

source_group("Project" FILES ${CORE_SOURCES})
source_group("UI" FILES ${UI_SOURCES})
source_group("Styles" FILES ${STYLE_SOURCES})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(SonaeLabelMaker
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET SonaeLabelMaker APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(SonaeLabelMaker SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(SonaeLabelMaker
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(SonaeLabelMaker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

#target_link_libraries(SonaeLabelMaker PRIVATE csv)

#if (EMSCRIPTEN)
#    target_compile_options(SonaeLabelMaker target> PRIVATE -s EXPORTED_RUNTIME_METHODS=['ccall'])
#    target_link_options(SonaeLabelMaker PRIVATE -s EXPORTED_RUNTIME_METHODS=['ccall'])
#endif()

target_sources(SonaeLabelMaker
    PRIVATE
    resources.qrc
)

target_link_libraries(SonaeLabelMaker
    PRIVATE Qt${QT_VERSION_MAJOR}::Gui
    PRIVATE Qt${QT_VERSION_MAJOR}::Core
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE Qt${QT_VERSION_MAJOR}::Svg
    PRIVATE Qt${QT_VERSION_MAJOR}::Xml
    PRIVATE Qt${QT_VERSION_MAJOR}::Quick
    PRIVATE Qt${QT_VERSION_MAJOR}::QuickControls2
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SonaeLabelMaker)
endif()
set_target_properties(SonaeLabelMaker PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS SonaeLabelMaker
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SonaeLabelMaker)
endif()
