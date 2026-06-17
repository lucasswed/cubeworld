cmake_minimum_required(VERSION 3.20)
project(cubeworld CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ── GLFW ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM (header-only math) ────────────────────────────────────────────────────
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ── GLAD (bundled in vendor/) ─────────────────────────────────────────────────
add_library(glad STATIC vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)

# ── Game executable ───────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(cubeworld ${SOURCES})

target_include_directories(cubeworld PRIVATE src)
target_link_libraries(cubeworld PRIVATE glad glfw glm::glm)

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(cubeworld PRIVATE opengl32)
    # Static link runtime so the .exe runs without DLLs
    if(MINGW)
        target_link_options(cubeworld PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
else()
    target_link_libraries(cubeworld PRIVATE GL)
endif()

# Copy assets next to the executable after build
add_custom_command(TARGET cubeworld POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:cubeworld>/assets"
)
