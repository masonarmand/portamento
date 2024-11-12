cmake_minimum_required(VERSION 3.10)

project(portamento VERSION 0.1)

# set(CMAKE_C_STANDARD 89)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

file(GLOB SOURCES "src/*.c" "src/audio/*.c" "src/audio/filetypes/*.c")
add_executable(portamento ${SOURCES})

target_include_directories(portamento PUBLIC ${PROJECT_BINARY_DIR})

# Add and build sqlite
add_library(sqlite3 STATIC deps/sqlite/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${PROJECT_SOURCE_DIR}/deps/sqlite)
target_link_libraries(portamento PRIVATE sqlite3)

# Find and link libcurl
find_package(CURL REQUIRED)
target_include_directories(portamento PRIVATE ${CURL_INCLUDE_DIR})
target_link_libraries(portamento PRIVATE ${CURL_LIBRARIES})

# Find and link libFLAC using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FLAC REQUIRED flac)
target_include_directories(portamento PRIVATE ${FLAC_INCLUDE_DIRS})
target_link_libraries(portamento PRIVATE ${FLAC_LIBRARIES})

# Include headers for stb_image and nuklear, including nuklear_xlib.h
target_include_directories(portamento PRIVATE ${PROJECT_SOURCE_DIR}/deps)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(portamento PRIVATE glfw m pthread)
endif()

cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

# Find and link OpenGL
find_package(OpenGL REQUIRED)
target_include_directories(portamento PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(portamento PRIVATE ${OPENGL_LIBRARIES})

# Find and link GLFW
find_package(glfw3 REQUIRED)
target_include_directories(portamento PRIVATE ${GLFW_INCLUDE_DIRS})
target_link_libraries(portamento PRIVATE glfw)

# Find and link GLEW
find_package(GLEW REQUIRED)
target_include_directories(portamento PRIVATE ${GLEW_INCLUDE_DIRS})
target_link_libraries(portamento PRIVATE ${GLEW_LIBRARIES})

# Specify the resource directory
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)

# Add an install rule for the executable
# install(TARGETS portamento DESTINATION bin)

# Add an install rule for the resource files
install(DIRECTORY ${RESOURCE_DIR} DESTINATION share/portamento)

# file(COPY res/ DESTINATION /usr/share/portamento/res/)
