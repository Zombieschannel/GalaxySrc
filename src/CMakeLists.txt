cmake_minimum_required (VERSION 3.15)
set(APP_NAME "Galaxy")
set(APP_VERSION "0.1.1")
set(APP_IDENTIFIER "com.Zombieschannel.Galaxy")

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
    find_package(SFML 3.0 COMPONENTS System Window Graphics Main REQUIRED)
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

    set(SFML_DIR "${SFML_LOC}/lib/cmake/SFML")
    find_package(SFML 3.0 COMPONENTS System Window Graphics REQUIRED)
    add_executable (${PROJECT_NAME} ${Project_SOURCES} ${Imgui_SOURCES} ${Whereami_SOURCES})

    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)

elseif(MACOS)

    set(SFML_DIR "${SFML_LOC}/lib/cmake/SFML")
    find_package(SFML 3.0 COMPONENTS System Window Graphics REQUIRED)

    set(ICON_NAME "AppLogo.icns")
    set(ICON_PATH "${CMAKE_SOURCE_DIR}/src/${ICON_NAME}")

    add_executable (${PROJECT_NAME} MACOSX_BUNDLE ${Project_SOURCES} ${Imgui_SOURCES} ${ICON_PATH} ${Whereami_SOURCES})

    set_source_files_properties( ${ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE_ICON_FILE ${ICON_NAME}
        MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IDENTIFIER}
        MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/MacOSXBundleInfo.plist.in
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.app/Contents/Libs"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LOC}/lib/libsfml-system.3.0.dylib" "${SFML_LOC}/lib/libsfml-system-d.3.0.dylib"
        "${SFML_LOC}/lib/libsfml-window.3.0.dylib" "${SFML_LOC}/lib/libsfml-window-d.3.0.dylib"
        "${SFML_LOC}/lib/libsfml-graphics.3.0.dylib" "${SFML_LOC}/lib/libsfml-graphics-d.3.0.dylib"
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
    find_package(SFML 3.0 COMPONENTS System Window Graphics Main)
    add_library(${PROJECT_NAME} SHARED ${Project_SOURCES} ${Imgui_SOURCES} ${Whereami_SOURCES})
    target_include_directories(${PROJECT_NAME} PRIVATE ../imgui ../stb_image)

endif()

if (ANDROID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_MOBILE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_OPENGL_ES)
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
            GLESv2)
endif()

