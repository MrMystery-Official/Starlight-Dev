# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE ARCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(CMAKE_OSX_ARCHITECTURES "${ARCH}" CACHE STRING "" FORCE)
    endif()
    message(STATUS "macOS target architecture: ${CMAKE_OSX_ARCHITECTURES}")
endif()

if(APPLE)
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64|aarch64")
        set(ASTCENC_TARGET astcenc-neon-static)
    elseif()
        set(ASTCENC_TARGET astcenc-sse2-static)
    endif()
elseif(WIN32)
    set(ASTCENC_TARGET astcenc-native-static)
else()
    set(ASTCENC_TARGET astcenc-native-static)
endif()

if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    # Common MSVC flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

    # Debug
    #set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi")
    #set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")

    # Release WITH PDBs
    #set(CMAKE_CXX_FLAGS_RELEASE "/MD /Zi")
    #set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /Zi")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Zi")

    message(STATUS "Set MSVC specific compiler flags (Release generates PDBs)")
endif()

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")

set(TOOL_FORCE_PATHS ON)
set(TOOL_MARIO_WONDER_AINB OFF)

set(TOOL_REMOVE_PRIVATE_FEATURES OFF)

if (WIN32)
    add_compile_definitions(WIN32)
endif()

include(ExternalProject)

function(CloneRepositoryOnly repositoryURL branchName targetDir)
    find_package(Git REQUIRED)
    
    if(NOT EXISTS "${targetDir}")
        message(STATUS "Cloning ${repositoryURL} into ${targetDir}")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} clone -b ${branchName} ${repositoryURL} ${targetDir}
            RESULT_VARIABLE GIT_CLONE_RESULT
        )
        if(NOT GIT_CLONE_RESULT EQUAL "0")
            message(FATAL_ERROR "Failed to clone repository")
        endif()
    else()
        message(STATUS "Repository already exists at ${targetDir}")
    endif()
endfunction()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/glfw)
    CloneRepositoryOnly(
        "https://github.com/glfw/glfw.git"
        "master"
        "${CMAKE_SOURCE_DIR}/libs/glfw")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/imgui)
    CloneRepositoryOnly(
        "https://github.com/ocornut/imgui.git"
        "v1.91.5-docking"
        "${CMAKE_SOURCE_DIR}/libs/imgui")

     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.h DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.h DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3_loader.h DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/misc/cpp/imgui_stdlib.h DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
     file(COPY ${CMAKE_SOURCE_DIR}/libs/imgui/misc/cpp/imgui_stdlib.cpp DESTINATION ${CMAKE_SOURCE_DIR}/libs/imgui)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/imgui-node-editor)
    CloneRepositoryOnly(
        "https://github.com/thedmd/imgui-node-editor.git"
        "develop"
        "${CMAKE_SOURCE_DIR}/libs/imgui-node-editor")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/glm)
    CloneRepositoryOnly(
        "https://github.com/g-truc/glm.git"
        "master"
        "${CMAKE_SOURCE_DIR}/libs/glm")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/stb)
    CloneRepositoryOnly(
        "https://github.com/nothings/stb.git"
        "master"
        "${CMAKE_SOURCE_DIR}/libs/stb/stb")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/zstd)
    CloneRepositoryOnly(
        "https://github.com/facebook/zstd.git"
        "dev"
        "${CMAKE_SOURCE_DIR}/libs/zstd")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/recastnavigation)
    CloneRepositoryOnly(
        "https://github.com/recastnavigation/recastnavigation.git"
        "main"
        "${CMAKE_SOURCE_DIR}/libs/recastnavigation")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/bvh)
    CloneRepositoryOnly(
        "https://github.com/madmann91/bvh.git"
        "v1"
        "${CMAKE_SOURCE_DIR}/libs/bvh")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/openmesh)
    CloneRepositoryOnly(
        "https://github.com/Lawrencemm/openmesh.git"
        "master"
        "${CMAKE_SOURCE_DIR}/libs/openmesh")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/meshoptimizer)
    CloneRepositoryOnly(
        "https://github.com/zeux/meshoptimizer.git"
        "master"
        "${CMAKE_SOURCE_DIR}/libs/meshoptimizer")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/astcenc)
    CloneRepositoryOnly(
        "https://github.com/ARM-software/astc-encoder.git"
        "main"
        "${CMAKE_SOURCE_DIR}/libs/astcenc")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/json)
    CloneRepositoryOnly(
        "https://github.com/nlohmann/json.git"
        "develop"
        "${CMAKE_SOURCE_DIR}/libs/json")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/bullet3)
    CloneRepositoryOnly(
        "https://github.com/bulletphysics/bullet3.git"
        "master"
        "${CMAKE_SOURCE_DIR}/libs/bullet3")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/libgit2)
    CloneRepositoryOnly(
        "https://github.com/libgit2/libgit2.git"
        "main"
        "${CMAKE_SOURCE_DIR}/libs/libgit2")
endif()

set(IS_DEBUG_BUILD CMAKE_BUILD_TYPE STREQUAL "Debug")

project ("Starlight")

add_compile_definitions(_USE_MATH_DEFINES)

find_package( OpenGL REQUIRED )

include_directories( ${OPENGL_INCLUDE_DIRS} )
add_subdirectory( libs/glfw )
add_subdirectory( libs/openmesh )

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static Bullet libs" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "Disable PyBullet" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "Disable extras and demos" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Disable unit tests" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "Disable CPU demos" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "Disable Bullet 2 demos" FORCE)
set(USE_DOUBLE_PRECISION OFF CACHE BOOL "Use single precision" FORCE)
add_subdirectory( libs/bullet3 )

set(ASTCENC_SHAREDLIB ON CACHE BOOL "Enable astcenc builds with core library shared objects")
set(ASTCENC_CLI OFF CACHE BOOL "Enable build of astcenc command line tools")
add_subdirectory( libs/astcenc )

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libgit2 static" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(STATIC_CRT OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/libgit2)

include_directories( libs/glm )
include_directories( libs/stb )
include_directories( libs/bvh/include )
include_directories( libs/openmesh/src )
include_directories( libs/bullet3/src )
include_directories( libs/libgit2/include )
include_directories( libs/astcenc/Source )
include_directories( libs/json/single_include )

add_library(imgui
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    # Add GLFW backend files
    libs/imgui/imgui_impl_glfw.cpp
    libs/imgui/imgui_impl_opengl3.cpp
    # Optional
    libs/imgui/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/libs/imgui
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends
)
target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
)

add_library(imgui-node-editor
    libs/imgui-node-editor/crude_json.cpp
    libs/imgui-node-editor/crude_json.h
    libs/imgui-node-editor/imgui_bezier_math.h
    libs/imgui-node-editor/imgui_bezier_math.inl
    libs/imgui-node-editor/imgui_canvas.cpp
    libs/imgui-node-editor/imgui_canvas.h
    libs/imgui-node-editor/imgui_extra_math.h
    libs/imgui-node-editor/imgui_extra_math.inl
    libs/imgui-node-editor/imgui_node_editor.h
    libs/imgui-node-editor/imgui_node_editor.cpp
    libs/imgui-node-editor/imgui_node_editor_api.cpp
    libs/imgui-node-editor/imgui_node_editor_internal.h
    libs/imgui-node-editor/imgui_node_editor_internal.inl
)
target_include_directories(imgui-node-editor PUBLIC
    ${CMAKE_SOURCE_DIR}/libs/imgui-node-editor
)
target_link_libraries(imgui-node-editor PUBLIC
    imgui
)

file(GLOB_RECURSE ZSTD_SOURCES "${CMAKE_SOURCE_DIR}/libs/zstd/lib/*.c")
list(FILTER ZSTD_SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/libs/zstd/lib/legacy/.*")
add_library(zstd
    ${ZSTD_SOURCES}
)
target_include_directories(zstd PUBLIC
    ${CMAKE_SOURCE_DIR}/libs/zstd/lib
    ${CMAKE_SOURCE_DIR}/libs/zstd/lib/dictBuilder
    ${CMAKE_SOURCE_DIR}/libs/zstd/lib/deprecated
    ${CMAKE_SOURCE_DIR}/libs/zstd/lib/decompress
    ${CMAKE_SOURCE_DIR}/libs/zstd/lib/compress
    ${CMAKE_SOURCE_DIR}/libs/zstd/lib/common
)

add_library(recastnavigation
    libs/recastnavigation/Recast/Source/Recast.cpp
    libs/recastnavigation/Recast/Source/RecastAlloc.cpp
    libs/recastnavigation/Recast/Source/RecastArea.cpp
    libs/recastnavigation/Recast/Source/RecastAssert.cpp
    libs/recastnavigation/Recast/Source/RecastContour.cpp
    libs/recastnavigation/Recast/Source/RecastFilter.cpp
    libs/recastnavigation/Recast/Source/RecastLayers.cpp
    libs/recastnavigation/Recast/Source/RecastMesh.cpp
    libs/recastnavigation/Recast/Source/RecastMeshDetail.cpp
    libs/recastnavigation/Recast/Source/RecastRasterization.cpp
    libs/recastnavigation/Recast/Source/RecastRegion.cpp
    
    libs/recastnavigation/Detour/Source/DetourAlloc.cpp
    libs/recastnavigation/Detour/Source/DetourAssert.cpp
    libs/recastnavigation/Detour/Source/DetourCommon.cpp
    libs/recastnavigation/Detour/Source/DetourNavMesh.cpp
    libs/recastnavigation/Detour/Source/DetourNavMeshBuilder.cpp
    libs/recastnavigation/Detour/Source/DetourNavMeshQuery.cpp
    libs/recastnavigation/Detour/Source/DetourNode.cpp
)
target_include_directories(recastnavigation PUBLIC
    ${CMAKE_SOURCE_DIR}/libs/recastnavigation/Recast/Include
    ${CMAKE_SOURCE_DIR}/libs/recastnavigation/Detour/Include
)

add_library(meshoptimizer
    libs/meshoptimizer/src/allocator.cpp
    libs/meshoptimizer/src/clusterizer.cpp
    libs/meshoptimizer/src/indexcodec.cpp
    libs/meshoptimizer/src/indexgenerator.cpp
    libs/meshoptimizer/src/overdrawoptimizer.cpp
    libs/meshoptimizer/src/quantization.cpp
    libs/meshoptimizer/src/simplifier.cpp
    libs/meshoptimizer/src/spatialorder.cpp
    libs/meshoptimizer/src/stripifier.cpp
    libs/meshoptimizer/src/vcacheoptimizer.cpp
    libs/meshoptimizer/src/vertexcodec.cpp
    libs/meshoptimizer/src/vertexfilter.cpp
    libs/meshoptimizer/src/vfetchoptimizer.cpp
)
target_include_directories(meshoptimizer PUBLIC
    ${CMAKE_SOURCE_DIR}/libs/meshoptimizer/src
)

include_directories( src )

# Include sub-projects.
link_libraries(imgui)
link_libraries(imgui-node-editor)
link_libraries(zstd)
link_libraries(recastnavigation)
link_libraries(OpenMeshCore OpenMeshTools)
link_libraries(BulletDynamics BulletCollision LinearMath)
link_libraries(libgit2package)
link_libraries(meshoptimizer)
message(STATUS "ASTCENC_TARGET: ${ASTCENC_TARGET}")
link_libraries(${ASTCENC_TARGET})

if(WIN32)
    add_compile_definitions(BACKWARD_SYSTEM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(BACKWARD_SYSTEM_DARWIN)
elseif(UNIX)
    add_compile_definitions(BACKWARD_SYSTEM_LINUX)
endif()

if (${IS_DEBUG_BUILD})
    add_compile_definitions(__DEBUG__)
endif ()

if(TOOL_FORCE_PATHS)
    add_compile_definitions(TOOL_FORCE_PATHS)
endif()

if(TOOL_MARIO_WONDER_AINB)
    add_compile_definitions(TOOL_MARIO_WONDER_AINB)
endif()

if(TOOL_REMOVE_PRIVATE_FEATURES)
    add_compile_definitions(TOOL_REMOVE_PRIVATE_FEATURES)
endif()

add_subdirectory ("src")