# Zombie's Galaxy image editor
## About
This is a simple image editor inspired by Paint.NET and Pinta. 
My goal is to have an image editor with cross platform capabilities like Pinta and many features similar to Paint.NET along with some capabilities designed for my use cases.

Based on the work done for my project ZLE. 
Just like ZLE, it uses the ImGui library for the user interface and SFML for other things. 
Also uses stb_image_write and stb_image_resize2.

## Compilation
Currently supported platforms are Windows, MacOS and Linux.

The project uses CMake, before compilation the installation path of SFML needs to be set in the root CMakeLists.txt.
For Windows versions MSVC is used, along with CMake presets from CMakePresets.json.
 
Note on Windows: SFML used should be compiled with `SFML_USE_STATIC_STD_LIBS` turned on.

For example, to build on Windows a x86 release build, from the root of the project use:
```
cmake --preset Windows-x86-Release
cmake --build out/build/Windows-x86-Release --config Release
```

To build on Linux/MacOS, from the root of the project:
```
mkdir build
cd build
cmake ..
cmake -DCMAKE_BUILD_TYPE=Release .
cmake --build . -j
```
