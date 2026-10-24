cmake_minimum_required(VERSION 3.10.0)
project(physics_simulation VERSION 0.1.0 )
cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
add_executable(physics_simulation src/main.cpp 
glad/include/glad/glad.h 
glad/src/glad.c)

target_link_libraries(physics_simulation glfw OpenGL::GL glm::glm)