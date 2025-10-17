cmake_minimum_required(VERSION 3.5)

project (main)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O0")

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

# Explicitly set the SDK path for macOS
execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE SDKROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_OSX_SYSROOT ${SDKROOT})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Wconversion -Wfatal-errors")

# Enable GLM experimental features
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

include_directories(.)
include_directories(/opt/homebrew/include)
include_directories(/opt/homebrew/include/SDL2)

# Find packages that work well with CONFIG
find_package(fmt CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Manually find all SDL and other libraries to avoid framework issues
find_library(SDL2_LIBRARY NAMES SDL2 PATHS /opt/homebrew/lib REQUIRED)
find_library(SDL2MAIN_LIBRARY NAMES SDL2main PATHS /opt/homebrew/lib REQUIRED)
find_library(SDL2_GFX_LIBRARY NAMES SDL2_gfx PATHS /opt/homebrew/lib REQUIRED)
find_library(SDL2_TTF_LIBRARY NAMES SDL2_ttf PATHS /opt/homebrew/lib REQUIRED)
find_library(GLFW_LIBRARY NAMES glfw glfw3 PATHS /opt/homebrew/lib REQUIRED)
find_library(GLEW_LIBRARY NAMES GLEW glew PATHS /opt/homebrew/lib REQUIRED)

add_executable(main main.cpp opengl_support.cpp)

target_link_libraries(main PRIVATE
    fmt::fmt
    ${SDL2MAIN_LIBRARY}
    ${SDL2_LIBRARY}
    ${SDL2_GFX_LIBRARY}
    ${SDL2_TTF_LIBRARY}
    ${GLFW_LIBRARY}
    ${GLEW_LIBRARY}
    OpenGL::GL
    assimp::assimp
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)
