cmake_minimum_required(VERSION 2.8.7)
project(synthrain)

SET(CMAKE_C_COMPILER /usr/bin/gcc-8)
SET(CMAKE_CXX_COMPILER /usr/bin/g++-8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${synthrain_SOURCE_DIR})
#include_directories("")
#link_directories("")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

INCLUDE(FindPkgConfig)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(SWScale REQUIRED)

INCLUDE_DIRECTORIES( ${SWScale_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})

file(GLOB SOURCES "*.cpp")
list(APPEND SOURCES "imgui/imgui.cpp"
            "imgui/imgui_impl_sdl.cpp"
            "imgui/imgui_impl_opengl3.cpp"
            "imgui/imgui_draw.cpp"
            "imgui/imgui_demo.cpp")

add_definitions(-DNO_CUDA)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#  add_definitions(/DYOURDEFINITION)
else()
    add_definitions(-DNDEBUG)  
endif()

add_executable(synthrain ${SOURCES}) 
set_target_properties(synthrain PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} pthread stdc++fs avcodec avformat avutil avdevice swscale ${SWScale_LIBRARIES} ${FFMPEG_LIBRARIES} ${PNG_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${FREETYPE_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
