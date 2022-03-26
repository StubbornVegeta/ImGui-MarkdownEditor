cmake_minimum_required(VERSION 3.5) # CMake最小版本
project(ImGuiMarkdown)              # 生成可执行文件的名字

message("${CMAKE_SOURCE_DIR}")
set(PROJECT_DIR ${CMAKE_SOURCE_DIR})
set(SOURCE_DIR ${PROJECT_DIR}/src)
set(INCLUDE_DIR ${PROJECT_DIR}/include)
set(BACKENDS_DIR ${PROJECT_DIR}/backends)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

#find_package(glfw3 REQUIRED)

set(SOURCES
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/LoadImage.cpp
    ${SOURCE_DIR}/imgui_markdown.cpp
    ${SOURCE_DIR}/imgui.cpp
    ${SOURCE_DIR}/imgui_draw.cpp
    ${SOURCE_DIR}/imgui_tables.cpp
    ${SOURCE_DIR}/imgui_widgets.cpp
    ${BACKENDS_DIR}/imgui_impl_glfw.cpp
    ${BACKENDS_DIR}/imgui_impl_opengl3.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${INCLUDE_DIR}
    ${BACKENDS_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    GL
    glfw)
