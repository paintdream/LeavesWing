cmake_minimum_required(VERSION 3.21)

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

project(LeavesWing VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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
    set(qtKitPatterns "msvc*")

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

# Windows
if(WIN32)
    set_target_properties(LeavesWing PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    if(MSVC)
        string(REPLACE "/EHsc" "/EHa" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        if(NOT CMAKE_CXX_FLAGS MATCHES "/EHa")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
        endif()
    endif()
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS LeavesWing
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
