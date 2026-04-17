cmake_minimum_required(VERSION 3.21)

option(LEAVESWING_PREFER_MINGW "Auto-detect a MinGW toolchain on Windows" OFF)

function(leaveswing_resolve_qt_root resultVar searchPath)
    set(resolvedQtRoot "")

    if(NOT searchPath)
        set(${resultVar} "" PARENT_SCOPE)
        return()
    endif()

    cmake_path(NORMAL_PATH searchPath OUTPUT_VARIABLE normalizedSearchPath)

    if(EXISTS "${normalizedSearchPath}/lib/cmake/Qt6/Qt6Config.cmake")
        set(resolvedQtRoot "${normalizedSearchPath}")
    else()
        get_filename_component(searchPathParent "${normalizedSearchPath}" DIRECTORY)
        if(EXISTS "${searchPathParent}/lib/cmake/Qt6/Qt6Config.cmake")
            set(resolvedQtRoot "${searchPathParent}")
        else()
            file(GLOB qtSubdirectories LIST_DIRECTORIES true "${normalizedSearchPath}/*")
            foreach(qtSubdirectory IN LISTS qtSubdirectories)
                if(EXISTS "${qtSubdirectory}/lib/cmake/Qt6/Qt6Config.cmake")
                    set(resolvedQtRoot "${qtSubdirectory}")
                    break()
                endif()
            endforeach()
        endif()
    endif()

    set(${resultVar} "${resolvedQtRoot}" PARENT_SCOPE)
endfunction()

function(leaveswing_find_mingw_bin_dir resultVar)
    set(mingwSearchRoots "")
    set(mingwBinDir "")

    if(DEFINED ENV{QT_DIR} AND NOT "$ENV{QT_DIR}" STREQUAL "")
        list(APPEND mingwSearchRoots "$ENV{QT_DIR}")
    endif()

    list(APPEND mingwSearchRoots
        "C:/Qt"
        "$ENV{USERPROFILE}/Qt"
        "$ENV{LOCALAPPDATA}/Qt"
        "C:/msys64"
        "C:/mingw64"
    )
    list(REMOVE_DUPLICATES mingwSearchRoots)

    foreach(mingwSearchRoot IN LISTS mingwSearchRoots)
        if(NOT mingwSearchRoot)
            continue()
        endif()

        cmake_path(NORMAL_PATH mingwSearchRoot OUTPUT_VARIABLE normalizedMingwSearchRoot)

        file(GLOB mingwBinCandidates LIST_DIRECTORIES true
            "${normalizedMingwSearchRoot}/bin"
            "${normalizedMingwSearchRoot}/mingw*/bin"
            "${normalizedMingwSearchRoot}/Tools/mingw*/bin"
            "${normalizedMingwSearchRoot}/../Tools/mingw*/bin"
            "${normalizedMingwSearchRoot}/../../Tools/mingw*/bin"
            "${normalizedMingwSearchRoot}/*/mingw*/bin"
        )

        foreach(candidateBinDir IN LISTS mingwBinCandidates)
            if(EXISTS "${candidateBinDir}/gcc.exe" AND EXISTS "${candidateBinDir}/g++.exe")
                set(mingwBinDir "${candidateBinDir}")
                break()
            endif()
        endforeach()

        if(mingwBinDir)
            break()
        endif()
    endforeach()

    set(${resultVar} "${mingwBinDir}" PARENT_SCOPE)
endfunction()

if(WIN32 AND LEAVESWING_PREFER_MINGW AND NOT CMAKE_C_COMPILER AND NOT CMAKE_CXX_COMPILER)
    leaveswing_find_mingw_bin_dir(leaveswingMingwBinDir)

    if(leaveswingMingwBinDir)
        set(ENV{PATH} "${leaveswingMingwBinDir};$ENV{PATH}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}")
        file(TO_NATIVE_PATH "${leaveswingMingwBinDir}" leaveswingMingwBinDirNative)

        set(leaveswingGccWrapper "${CMAKE_BINARY_DIR}/leaveswing-gcc.cmd")
        set(leaveswingGxxWrapper "${CMAKE_BINARY_DIR}/leaveswing-gxx.cmd")

        file(WRITE "${leaveswingGccWrapper}" "@echo off\r\nset \"PATH=${leaveswingMingwBinDirNative};%PATH%\"\r\n\"${leaveswingMingwBinDirNative}\\gcc.exe\" %*\r\n")
        file(WRITE "${leaveswingGxxWrapper}" "@echo off\r\nset \"PATH=${leaveswingMingwBinDirNative};%PATH%\"\r\n\"${leaveswingMingwBinDirNative}\\g++.exe\" %*\r\n")

        set(CMAKE_C_COMPILER "${leaveswingGccWrapper}" CACHE FILEPATH "MinGW C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${leaveswingGxxWrapper}" CACHE FILEPATH "MinGW C++ compiler" FORCE)
        message(STATUS "Using MinGW toolchain: ${leaveswingMingwBinDir}")
    endif()
endif()

project(LeavesWing VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(WIN32 AND leaveswingMingwBinDir)
    file(TO_NATIVE_PATH "${leaveswingMingwBinDir}" leaveswingMingwBinDirNative)
    set(leaveswingRuleLauncher "${CMAKE_BINARY_DIR}/leaveswing-mingw-env.bat")
    file(WRITE "${leaveswingRuleLauncher}" "@echo off\r\nset \"PATH=${leaveswingMingwBinDirNative};%PATH%\"\r\ncall %*\r\n")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "\"${leaveswingRuleLauncher}\"")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "\"${leaveswingRuleLauncher}\"")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_CUSTOM "\"${leaveswingRuleLauncher}\"")
endif()

# ── Qt6 ──────────────────────────────────────────────────────────────────────
set(qtRootDir "")
set(qtSearchBases "")

if(DEFINED ENV{QT_DIR} AND NOT "$ENV{QT_DIR}" STREQUAL "")
    list(APPEND qtSearchBases "$ENV{QT_DIR}")
endif()

if(CMAKE_PREFIX_PATH)
    list(APPEND qtSearchBases ${CMAKE_PREFIX_PATH})
endif()

if(WIN32)
    set(qtKitPatterns "")

    if(LEAVESWING_PREFER_MINGW OR MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND qtKitPatterns "mingw*")
    elseif(MSVC)
        list(APPEND qtKitPatterns "msvc*")
    else()
        list(APPEND qtKitPatterns "mingw*" "msvc*")
    endif()

    foreach(qtBaseDir IN ITEMS "C:/Qt" "$ENV{USERPROFILE}/Qt" "$ENV{LOCALAPPDATA}/Qt")
        foreach(qtKitPattern IN LISTS qtKitPatterns)
            file(GLOB qtWindowsCandidates LIST_DIRECTORIES true "${qtBaseDir}/*/${qtKitPattern}")
            list(APPEND qtSearchBases ${qtWindowsCandidates})
        endforeach()
    endforeach()
endif()

list(REMOVE_DUPLICATES qtSearchBases)

foreach(qtSearchBase IN LISTS qtSearchBases)
    leaveswing_resolve_qt_root(qtCandidateRoot "${qtSearchBase}")
    if(qtCandidateRoot)
        set(qtRootDir "${qtCandidateRoot}")
        break()
    endif()
endforeach()

if(qtRootDir)
    list(PREPEND CMAKE_PREFIX_PATH "${qtRootDir}")
    message(STATUS "Using Qt from: ${qtRootDir}")
elseif(DEFINED ENV{QT_DIR} AND NOT "$ENV{QT_DIR}" STREQUAL "")
    message(STATUS "QT_DIR is set, but no Qt6 package configuration was found under: $ENV{QT_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)

# ── Executable ───────────────────────────────────────────────────────────────
FILE(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h)
FILE(GLOB_RECURSE IRIS_SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ref/iris/src/*.cpp ${CMAKE_SOURCE_DIR}/ref/iris/src/*.h)
FILE(GLOB_RECURSE IRIS_LUA_SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ref/iris/src/lua/src/*.c)
LIST(REMOVE_ITEM IRIS_LUA_SOURCE_FILES ${CMAKE_SOURCE_DIR}/ref/iris/src/lua/src/lua.c ${CMAKE_SOURCE_DIR}/ref/iris/src/lua/src/luac.c)

qt_add_executable(LeavesWing
    ${SOURCE_FILES} ${IRIS_SOURCE_FILES} ${IRIS_LUA_SOURCE_FILES}
)

# ── QML Module ───────────────────────────────────────────────────────────────
qt_add_qml_module(LeavesWing
    URI LeavesWingQML
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/HomePage.qml
        src/qml/ExplorePage.qml
        src/qml/ProfilePage.qml
)

# ── Link Libraries ───────────────────────────────────────────────────────────
target_link_libraries(LeavesWing PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
)

# ── Platform-specific settings ───────────────────────────────────────────────

# Android
if(ANDROID)
    set_target_properties(LeavesWing PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/platform/android"
    )
endif()

# iOS
if(IOS)
    set_target_properties(LeavesWing PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/platform/ios/Info.plist"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.paintdream.leaveswing"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    )
endif()

# Windows
if(WIN32)
    set_target_properties(LeavesWing PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS LeavesWing
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
