cmake_minimum_required(VERSION 4.2)
project(terraingen LANGUAGES C CXX)

# -- c++ --
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# --- dependencies ---
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED)

# --- vars ---
set(CODE_DIR ${CMAKE_SOURCE_DIR}/src/)
set(EXTERN_DIR ${CMAKE_SOURCE_DIR}/external/)
set(LIB_SRC_DIR /usr/local/src)
set(LIB_INC_DIR /usr/local/include)

# --- ImGui ---
set(IMGUI_DIR ${EXTERN_DIR}/imgui)

set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp

  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
  ${EXTERN_DIR}
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
  OpenGL::GL
  glfw
)

# -- abstractions and other dependencies ---
add_subdirectory(${EXTERN_DIR}/glabstractions)
add_library(glad STATIC ${EXTERN_DIR}/glad/src/gl.c)
target_include_directories(glad PUBLIC ${EXTERN_DIR}/glad/include)

# --- main executable ---
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${CODE_DIR}/*.cpp)
add_executable(
  ${PROJECT_NAME}
  ${SRC}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CODE_DIR}
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  imgui
  OpenGL::GL
  glm::glm
  glfw
  glad
  glabstractions
  Threads::Threads
  assimp::assimp
  minizip
  z
)
