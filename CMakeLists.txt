cmake_minimum_required(VERSION 3.10)

project(MapEditor)

enable_language(C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)

add_compile_definitions(
    GLM_ENABLE_EXPERIMENTAL
    IMGUI_DEFINE_MATH_OPERATORS
    )

include_directories(
    MapEditor/Libraries/include
    MapEditor/imgui
    MapEditor/imgui/backends
    MapEditor/imgui/misc/cpp
    MapEditor/imgui_gizmo
    MapEditor/imgui_node_editor
    )

file(GLOB SOURCES_C ${PROJECT_SOURCE_DIR}/MapEditor/*.c)
file(GLOB SOURCES_CXX ${PROJECT_SOURCE_DIR}/MapEditor/*.cpp)

file(GLOB SOURCES_IMGUI ${PROJECT_SOURCE_DIR}/MapEditor/imgui/*.cpp)
file(GLOB SOURCES_IMGUI_STDLIB ${PROJECT_SOURCE_DIR}/MapEditor/imgui/misc/cpp/*.cpp)
file(GLOB SOURCES_IMGUI_GIZMO ${PROJECT_SOURCE_DIR}/MapEditor/imgui_gizmo/*.cpp)
file(GLOB SOURCES_IMGUI_NODE_EDITOR ${PROJECT_SOURCE_DIR}/MapEditor/imgui_node_editor/*.cpp)

if (WIN32)
    link_directories(MapEditor/Libraries/lib)
    set(PLATFORM_SPECIFIC_LIBRARIES zstd_static glfw3)
else()
    set(PLATFORM_SPECIFIC_LIBRARIES zstd glfw)
endif()

add_executable(MapEditor
    ${SOURCES_C} ${SOURCES_CXX} ${SOURCES_IMGUI} ${SOURCES_IMGUI_STDLIB} ${SOURCES_IMGUI_GIZMO} ${SOURCES_IMGUI_NODE_EDITOR}
    ${PROJECT_SOURCE_DIR}/MapEditor/imgui/backends/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/MapEditor/imgui/backends/imgui_impl_glfw.cpp
    )

target_link_libraries(MapEditor PUBLIC ${PLATFORM_SPECIFIC_LIBRARIES} ${OPENGL_LIBRARIES})

file(COPY MapEditor/Assets DESTINATION .)
