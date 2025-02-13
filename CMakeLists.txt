cmake_minimum_required(VERSION 3.10)
project(CloudRayMarching)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(glfw3 3.3 REQUIRED)


add_library(glad extern/src/glad.c)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern/include)


add_executable(CloudRayMarching 
    src/main.cpp
    src/Shader.cpp
    src/Noise.cpp
)


target_include_directories(CloudRayMarching
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
)


target_link_libraries(CloudRayMarching
    glfw
    glad
    "-framework OpenGL"
)

