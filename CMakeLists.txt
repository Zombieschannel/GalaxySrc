cmake_minimum_required (VERSION 3.15)
project (Galaxy LANGUAGES ASM C CXX)
set(APP_VERSION "0.4.0")
set(APP_IDENTIFIER "com.Zombieschannel.Galaxy")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MACOS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ANDROID TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(EMSCRIPTEN TRUE)
endif()

#SFML SHARED LIBRARY
if (WINDOWS OR LINUX OR ANDROID OR EMSCRIPTEN)
    set(SFML_STATIC_LIBRARIES TRUE)
else()
    set(SFML_STATIC_LIBRARIES FALSE)
endif()

if (WINDOWS)
    #set your SFML dir for each architecture
    if (${BUILD_ARCH} STREQUAL "x86")
        set(SFML_LOC "<SFML location here>/SFML-3.1.0-x86-nodep")
    elseif(${BUILD_ARCH} STREQUAL "x64")
        set(SFML_LOC "<SFML location here>/SFML-3.1.0-x64-nodep")
    elseif(${BUILD_ARCH} STREQUAL "ARM64")
        set(SFML_LOC "<SFML location here>/SFML-3.1.0-ARM64-nodep")
    endif()
elseif(LINUX)
    set(SFML_LOC "<SFML location here>/SFML-3.1.0")
elseif(MACOS)
    set(SFML_LOC "<SFML location here>/SFML-3.1.0")
elseif(EMSCRIPTEN)
    set(SFML_ROOT "<SFML location here>/SFML-3.1.0")
endif()

#FILES
if (NOT EXISTS "assets/")
    file(MAKE_DIRECTORY "assets/")
endif()
if (NOT EXISTS "versions/")
    file(MAKE_DIRECTORY "versions/")
endif()

add_subdirectory("src")

#Debian package
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
        DESTINATION share/${PROJECT_NAME})

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zombieschannel")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple image editor.")

include(CPack)
