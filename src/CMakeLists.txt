cmake_minimum_required(VERSION 3.10)
project(Rendering_Engine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ../include
    ../include/imgui
    ../include/imgui/backends
)

if(WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE ../lib)
    target_link_libraries(${PROJECT_NAME} glad glfw3 stb_image imgui opengl32 gdi32)
elseif(APPLE)
    target_link_directories(${PROJECT_NAME} PRIVATE ../lib)
    target_link_libraries(${PROJECT_NAME} glad glfw3 stb_image imgui
        "-framework Cocoa" "-framework OpenGL" "-framework IOKit")
else()
    target_link_directories(${PROJECT_NAME} PRIVATE ../lib_linux)
    target_link_libraries(${PROJECT_NAME} glad glfw3 stb_image imgui GL)
endif()