cmake_minimum_required (VERSION 3.15)

#FILES
file(GLOB_RECURSE Project_SOURCES "*.cpp")
file(GLOB_RECURSE Imgui_SOURCES "../imgui/*.cpp")
file(GLOB_RECURSE Whereami_SOURCES "../whereami/*.c")
file(COPY "../assets/" DESTINATION "${CMAKE_BINARY_DIR}/src")

if(WINDOWS)
    enable_language("RC")
    set(WIN32_RESOURCES "icon.rc")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set(SFML_DIR "${SFML_LOC}/lib/cmake/SFML")
    find_package(SFML 3.1 COMPONENTS System Window Graphics Main REQUIRED)
    add_executable (${PROJECT_NAME} ${Project_SOURCES} ${Imgui_SOURCES} ${WIN32_RESOURCES} ${Whereami_SOURCES})

    if (${BUILD_ARCH} STREQUAL "x86")
        target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_X86)
    elseif(${BUILD_ARCH} STREQUAL "x64")
        target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_X64)
    elseif(${BUILD_ARCH} STREQUAL "ARM64")
        target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_ARM64)
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)

elseif(LINUX)

    set(SFML_DIR "${SFML_LOC}/lib64/cmake/SFML")
    find_package(SFML 3.1 COMPONENTS System Window Graphics REQUIRED)
    add_executable (${PROJECT_NAME} ${Project_SOURCES} ${Imgui_SOURCES} ${Whereami_SOURCES} "assets.S")

    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)

elseif(MACOS)

    set(SFML_DIR "${SFML_LOC}/lib/cmake/SFML")
    find_package(SFML 3.1 COMPONENTS System Window Graphics REQUIRED)

    set(ICON_NAME "AppLogo.icns")
    set(ICON_PATH "${CMAKE_SOURCE_DIR}/src/${ICON_NAME}")

    add_executable (${PROJECT_NAME} MACOSX_BUNDLE ${Project_SOURCES} ${Imgui_SOURCES} ${ICON_PATH} ${Whereami_SOURCES} "assetsMacOS.S")

    set_source_files_properties( ${ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE_ICON_FILE ${ICON_NAME}
        MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IDENTIFIER}
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/MacOSXBundleInfo.plist.in
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.app/Contents/Libs"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LOC}/lib/libsfml-system.3.1.dylib" "${SFML_LOC}/lib/libsfml-system-d.3.1.dylib"
        "${SFML_LOC}/lib/libsfml-window.3.1.dylib" "${SFML_LOC}/lib/libsfml-window-d.3.1.dylib"
        "${SFML_LOC}/lib/libsfml-graphics.3.1.dylib" "${SFML_LOC}/lib/libsfml-graphics-d.3.1.dylib"
        "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.app/Contents/Libs"
        COMMENT "Copying dylibs to the app bundle..."
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.app/Contents/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SFML_LOC}/Frameworks" "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.app/Contents/Frameworks"
	    COMMENT "Copying frameworks to target location..."
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND install_name_tool -add_rpath "@loader_path/../Libs" "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Fixing dylib paths in the executable..."
    )

    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)

elseif(ANDROID)

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
    set(PATH_TO_SFML ${ANDROID_NDK}/sources/third_party/sfml)
    set(SFML_DIR ${PATH_TO_SFML}/lib/${ANDROID_ABI}/cmake/SFML)

    add_library(EGL::EGL INTERFACE IMPORTED)
    set_target_properties(EGL::EGL PROPERTIES
            INTERFACE_LINK_LIBRARIES EGL)

    add_library(GLES::GLES INTERFACE IMPORTED)
    set_target_properties(GLES::GLES PROPERTIES
            INTERFACE_LINK_LIBRARIES GLESv1_CM)

    find_package(SFML 3.1 COMPONENTS System Window Graphics Main REQUIRED)
    add_library(${PROJECT_NAME} SHARED ${Project_SOURCES} ${Imgui_SOURCES} ${Whereami_SOURCES} "assets.S")
    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)

elseif(EMSCRIPTEN)

    set(SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_DEBUG -g3 -gsource-map -O0)

    set(SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_RELEASE -O3)

    set(SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS
            $<$<CONFIG:Debug>:${SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_DEBUG}>
            $<$<CONFIG:Release>:${SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_RELEASE}>
            -pthread
    )

    # -sEXCEPTION_STACK_TRACES=1 # Exceptions will contain stack traces and uncaught exceptions will display stack traces
    # -sGL_ASSERTIONS=1          # Adds extra checks for error situations in the GL library
    # -sDETERMINISTIC=1          # Force `Date.now()`, `Math.random`, etc. to return deterministic results
    # -fwasm-exceptions          # TODO P1: -fwasm-exceptions seems to break examples
    # -sEXCEPTION_DEBUG=1        # Print out exceptions in emscriptened code (SPEWS WARNINGS)
    # -sGL_DEBUG=1               # Enables more verbose debug printing of WebGL related operations (SPEWS WARNINGS)

    set(SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS_DEBUG -g3 -gsource-map -sASSERTIONS=2 -sCHECK_NULL_WRITES=1 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=1 -sNO_DISABLE_EXCEPTION_CATCHING)

    set(SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS_RELEASE -O3)

    set(SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS_ASYNCIFY -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=5120000 ) #-sASYNCIFY_IGNORE_INDIRECT=1)

    set(SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS
            $<$<CONFIG:Debug>:${SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS_DEBUG}>
            $<$<CONFIG:Release>:${SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS_RELEASE}>

            ${SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS_ASYNCIFY}

            -pthread                           # Enable threading support

            -Wno-limited-postlink-optimizations # warning: running limited binaryen optimizations because DWARF info requested (or indirectly required)
            -Wno-pthreads-mem-growth            # warning: -pthread + ALLOW_MEMORY_GROWTH may run non-wasm code slowly, see https://github.com/WebAssembly/design/issues/1271

            -sALLOW_MEMORY_GROWTH=1             # Grow the memory arrays at runtime
            -sEXIT_RUNTIME=1                    # Execute cleanup (e.g. `atexit`) after `main` completes
            -sFETCH=1                           # Enables `emscripten_fetch` API
            -sFORCE_FILESYSTEM=1                # Makes full filesystem support be included
            -sFULL_ES2=1                        # Forces support for all GLES2 features, not just the WebGL-friendly subset
            -sMAX_WEBGL_VERSION=1               # Specifies the highest WebGL version to target
            -sMIN_WEBGL_VERSION=1               # Specifies the lowest WebGL version to target
            -sSTACK_SIZE=4mb                    # Set the total stack size
            -sUSE_PTHREADS=1                   # Enable threading support
            -sPTHREAD_POOL_SIZE=2
            -sOFFSCREENCANVAS_SUPPORT=1
            -sWASM=1                            # Compile code to WebAssembly

            -sGL_EXPLICIT_UNIFORM_LOCATION=1  # TODO P0:
            -sGL_EXPLICIT_UNIFORM_BINDING=1   # TODO P0:

            --emrun                             # Add native support for `emrun` (I/O capture)
            --embed-file ${CMAKE_SOURCE_DIR}/assets@/
            -sTOTAL_MEMORY=32mb

            --shell-file=${CMAKE_SOURCE_DIR}/shell.html
    )
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(Freetype_DIR ${SFML_ROOT}/lib/cmake/freetype)
    set(Vorbis_DIR ${SFML_ROOT}/lib/cmake/Vorbis)
    set(Ogg_DIR ${SFML_ROOT}/lib/cmake/Ogg)
    set(FLAC_DIR ${SFML_ROOT}/lib/cmake/FLAC)
    set(SFML_DIR ${SFML_ROOT}/lib/cmake/SFML)

    find_package(SFML 3.1 COMPONENTS Graphics Window System REQUIRED)
    add_executable (${PROJECT_NAME} ${Project_SOURCES} ${Imgui_SOURCES})

    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)
    target_compile_options(${PROJECT_NAME} PRIVATE ${SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS})
    target_link_options(${PROJECT_NAME} PRIVATE ${SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS})

endif()

if (ANDROID OR EMSCRIPTEN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_OPENGL_ES)
endif()

if (MACOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()

if (ANDROID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_OPENGL_ES1)
elseif (EMSCRIPTEN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_OPENGL_ES2)
endif()

if (ANDROID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_JNI)
endif()

if (ANDROID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_MOBILE)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_DESKTOP)
endif()

#CPP VERSION
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set (LINK_SFML SFML::System SFML::Window SFML::Graphics)

if (WINDOWS)
    target_link_libraries(${PROJECT_NAME} ${LINK_SFML} SFML::Main)
elseif(LINUX)
    target_link_libraries(${PROJECT_NAME} ${LINK_SFML} -lGL -lX11)
elseif(MACOS)
    target_link_libraries(${PROJECT_NAME} ${LINK_SFML} "-framework OpenGL")
elseif(ANDROID)
    target_link_libraries(${PROJECT_NAME}
            log
            android
            ${LINK_SFML}
            -Wl,--whole-archive SFML::Main -Wl,--no-whole-archive
            EGL
            GLESv1_CM)
elseif(EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} ${LINK_SFML})
endif()

