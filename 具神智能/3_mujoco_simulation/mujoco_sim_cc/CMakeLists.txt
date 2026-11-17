#谨以此文件，使用AI,帮助那些开发者快速理解cmake的各个命令的意义


cmake_minimum_required(VERSION 3.24)
project(SO_ARM101_CPP VERSION 1.0.0 LANGUAGES C CXX)

#SO_ARM101_CPP → 项目名，之后可用 ${PROJECT_NAME} 引用
#VERSION 1.0.0 → 版本号，可用 ${PROJECT_VERSION} 引用，打包（CPack）时会用到
#LANGUAGES C CXX → 声明本项目用 C 和 C++。这时 CMake 会去探测编译器（找到 cl.exe 或 gcc），并把编译相关变量填好






#CMAKE_CXX_STANDARD 17	加 -std=c++17 / /std:c++17
#CMAKE_CXX_STANDARD_REQUIRED ON	编译器不支持就报错，不允许自动降级（默认是允许降到支持的最高版本）
#CMAKE_CXX_EXTENSIONS OFF	禁止编译器扩展（GCC 就不再是 gnu++17 而是 c++17），保证跨编译器的行为一致
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)





# MUJOCO_BUILD_DIR → 变量名（大写，CMake 惯例用大写表示全局/可配置变量）
# "E:/Mujoco/build" → 默认值（只在第一次配置、缓存里还不存在这个变量时使用）
# CACHE PATH → 把变量写进 CMakeCache.txt，类型为 PATH
# "MuJoCo build" → 说明文字，会显示在 cmake-gui 或 Qt Creator 的 CMake 配置页里
# CACHE 的关键特性（这是你最需要记住的一点）
# Original locally built MuJoCo paths retained.

#mujoco源码的编译路径
set(MUJOCO_BUILD_DIR "E:/Mujoco/build" CACHE PATH "MuJoCo build")
#mujoco源码路径
set(MUJOCO_SOURCE_DIR "E:/Mujoco" CACHE PATH "MuJoCo source")
#mujoco源码的include头文件路径
set(MUJOCO_INCLUDE_DIR "${MUJOCO_SOURCE_DIR}/include")
#mujoco源码的library路径
set(MUJOCO_LIBRARY "${MUJOCO_BUILD_DIR}/lib/RelWithDebInfo/mujoco.lib")
#mujoco源码的dll文件路径
set(MUJOCO_DLL "${MUJOCO_BUILD_DIR}/bin/RelWithDebInfo/mujoco.dll")


#cmake会自动遍历下面的路径，用来判断mujoco的头文件、静态库、动态库是否加载成功
foreach(_f "${MUJOCO_INCLUDE_DIR}/mujoco/mujoco.h" "${MUJOCO_LIBRARY}" "${MUJOCO_DLL}")
    if(NOT EXISTS "${_f}")
        message(FATAL_ERROR "MuJoCo file missing: ${_f}")
    endif()
endforeach()

# Keep existing Eigen dependency. No GLFW needed: Qt owns the OpenGL window.
set(EIGEN_ROOT "E:/eigen-3.4.0" CACHE PATH "Eigen root")

#cmake会根据EIGEN_ROOT循寻找名为Eigen/Core的文件，并将其写入到EIGEN_INCLUDE_DIR中
#后续就可以通过EIGEN_INCLUDE_DIR引用头文件
#为什么EIGEN的头文件引用很特殊，因为它的源码的头文件的引用不是包含在include文件夹中
#而是包含在EIGEN/CORE这个文件中
find_path(EIGEN_INCLUDE_DIR NAMES Eigen/Core PATHS "${EIGEN_ROOT}" NO_DEFAULT_PATH)
if(NOT EIGEN_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen not found at ${EIGEN_ROOT}")
endif()

#寻找QT依赖
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)


#Qt 提供的辅助命令，做三件事：
#开启 CMAKE_AUTOMOC（自动处理 Qt 的 Q_OBJECT 元对象编译器）
#开启 CMAKE_AUTOUIC（自动生成 .ui 的代码）
#设置平台相关的默认值（如 rpath）
qt_standard_project_setup()

#添加可执行文件
qt_add_executable(so_arm101_sim
    src/main.cpp
    src/Simulation.cpp
    src/Viewer.cpp
    src/Replay.cpp
    include/Simulation.h
    include/Viewer.h
    include/Replay.h
)

#整个项目的头文件搜索命令
target_include_directories(so_arm101_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${MUJOCO_INCLUDE_DIR}"
    "${EIGEN_INCLUDE_DIR}")

#整个项目链接的库
target_link_libraries(so_arm101_sim PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
    "${MUJOCO_LIBRARY}")

#如果是win32平台 引入win下的opengl32.lib
#除此之外，win下还不应该引入minimax宏，防止污染cc++中的minimax宏
#最后告诉编译器 我只要window的精简头文件
if(WIN32)
    target_link_libraries(so_arm101_sim PRIVATE opengl32)
    target_compile_definitions(so_arm101_sim PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()


#在当前项目编译之后 执行一个 command ，他会复制mujoco所有的dll文件到exe所在的目录中
#如果已经存在且是相同的，就直接跳过
add_custom_command(TARGET so_arm101_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MUJOCO_DLL}" "$<TARGET_FILE_DIR:so_arm101_sim>")



#设置scene.xml和其他资源的目录，并将其写入宏文件中
set(SO_ARM101_RESOURCE_ROOT "F:/mujoco_sim" CACHE PATH "scene.xml directory")
target_compile_definitions(so_arm101_sim PRIVATE
    SO_ARM101_RESOURCE_ROOT="${SO_ARM101_RESOURCE_ROOT}")


# 如果是MVSC平台
# /W4 → MSVC 的最高常规警告级别
# /permissive- → 禁用 MSVC 的非标准扩展，更严格地遵循 C++ 标准
# -Wall -Wextra -Wpedantic → GCC/Clang 的对应写法
if(MSVC)
    target_compile_options(so_arm101_sim PRIVATE /W4 /permissive-)
else()#如果是gcc clang 平台
    # -Wall 开启常用确定是问题的警告
    # -Wextra 一些啰嗦的警告 变量定义后未使用
    # --Wpedantic 严格遵循gcc 的c++标准 ，不允许扩展
    target_compile_options(so_arm101_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()
