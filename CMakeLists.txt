cmake_minimum_required (VERSION 3.15)
project (Galaxy)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MACOS TRUE)
else(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ANDROID TRUE)
endif()

#SFML SHARED LIBRARY
if (WINDOWS OR LINUX)
    set(SFML_STATIC_LIBRARIES TRUE)
else()
    set(SFML_STATIC_LIBRARIES FALSE)
endif()

if (WINDOWS)
    #set your SFML dir for each architecture
    if (${BUILD_ARCH} STREQUAL "x86")
        set(SFML_LOC "<SFML location here>/SFML-3.0.2-x86-nodep")
    elseif(${BUILD_ARCH} STREQUAL "x64")
        set(SFML_LOC "<SFML location here>/SFML-3.0.2-x64-nodep")
    elseif(${BUILD_ARCH} STREQUAL "ARM64")
        set(SFML_LOC "<SFML location here>/SFML-3.0.2-ARM64-nodep")
    endif()
elseif(LINUX)
    set(SFML_LOC "<SFML location here>/SFML-3.0.2")
elseif(MACOS)
    set(SFML_LOC "<SFML location here>/SFML-3.0.2")
endif()

#FILES
if (NOT EXISTS "assets/")
    file(MAKE_DIRECTORY "assets/")
endif()
if (NOT EXISTS "versions/")
    file(MAKE_DIRECTORY "versions/")
endif()

add_subdirectory("src")
