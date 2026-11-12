cmake_minimum_required(VERSION 3.24)
project(SO_ARM101_CPP VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Original locally built MuJoCo paths retained.
set(MUJOCO_BUILD_DIR "E:/Mujoco/build" CACHE PATH "MuJoCo build")
set(MUJOCO_SOURCE_DIR "E:/Mujoco" CACHE PATH "MuJoCo source")
set(MUJOCO_INCLUDE_DIR "${MUJOCO_SOURCE_DIR}/include")
set(MUJOCO_LIBRARY "${MUJOCO_BUILD_DIR}/lib/RelWithDebInfo/mujoco.lib")
set(MUJOCO_DLL "${MUJOCO_BUILD_DIR}/bin/RelWithDebInfo/mujoco.dll")
foreach(_f "${MUJOCO_INCLUDE_DIR}/mujoco/mujoco.h" "${MUJOCO_LIBRARY}" "${MUJOCO_DLL}")
    if(NOT EXISTS "${_f}")
        message(FATAL_ERROR "MuJoCo file missing: ${_f}")
    endif()
endforeach()

# Keep existing Eigen dependency. No GLFW needed: Qt owns the OpenGL window.
set(EIGEN_ROOT "E:/eigen-3.4.0" CACHE PATH "Eigen root")
find_path(EIGEN_INCLUDE_DIR NAMES Eigen/Core PATHS "${EIGEN_ROOT}" NO_DEFAULT_PATH)
if(NOT EIGEN_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen not found at ${EIGEN_ROOT}")
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
qt_standard_project_setup()

qt_add_executable(so_arm101_sim
    src/main.cpp
    src/Simulation.cpp
    src/Viewer.cpp
    src/Replay.cpp
    include/Simulation.h
    include/Viewer.h
    include/Replay.h
)
target_include_directories(so_arm101_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${MUJOCO_INCLUDE_DIR}"
    "${EIGEN_INCLUDE_DIR}")
target_link_libraries(so_arm101_sim PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
    "${MUJOCO_LIBRARY}")
if(WIN32)
    target_link_libraries(so_arm101_sim PRIVATE opengl32)
    target_compile_definitions(so_arm101_sim PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
add_custom_command(TARGET so_arm101_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MUJOCO_DLL}" "$<TARGET_FILE_DIR:so_arm101_sim>")
set(SO_ARM101_RESOURCE_ROOT "F:/mujoco_sim" CACHE PATH "scene.xml directory")
target_compile_definitions(so_arm101_sim PRIVATE
    SO_ARM101_RESOURCE_ROOT="${SO_ARM101_RESOURCE_ROOT}")
if(MSVC)
    target_compile_options(so_arm101_sim PRIVATE /W4 /permissive-)
else()
    target_compile_options(so_arm101_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()
