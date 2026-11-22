cmake_minimum_required(VERSION 3.16)

project(onnx_pro VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ONNX_RUN_TIME_ROOT "E:/onnx_run_time/onnxruntime_gpu_1201")
set(OPENCV_DIR "E:/opencv/build")
set(CUDA_ROOT_DIR "E:/cuda12")

enable_language(CUDA CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
find_package(OpenCV REQUIRED)


qt_standard_project_setup(REQUIRES 6.5)


qt_add_executable(apponnx_pro
    main.cpp
    relux.h
    relux.cc
    onnx_register.cc
)


qt_add_qml_module(apponnx_pro
    URI onnx_pro
    VERSION 1.0
    QML_FILES
        Main.qml
)


###############自定义算子库dll##################
add_library(relux SHARED
    relux.cc
    onnx_register.cc
)


target_include_directories(relux
    PRIVATE
    ${ONNX_RUN_TIME_ROOT}/include

)

target_link_directories(relux
    PRIVATE
    ${ONNX_RUN_TIME_ROOT}/lib

)

target_link_libraries(relux PRIVATE onnxruntime)
##############################################






# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(apponnx_pro PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.apponnx_pro
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(apponnx_pro
  PRIVATE  ${ONNX_RUN_TIME_ROOT}/include
  ${OPENCV_ROOT}/build/include
  ${CUDA_ROOT_DIR}/include

)

target_link_directories(apponnx_pro PRIVATE

    ${ONNX_RUN_TIME_ROOT}/lib
    ${CUDA_ROOT_DIR}/lib/x64
)

target_link_libraries(apponnx_pro
    PRIVATE Qt6::Quick
    onnxruntime
    ${OpenCV_LIBS}
    cudart
)



include(GNUInstallDirs)
install(TARGETS apponnx_pro
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


if(WIN32)

    add_custom_command(TARGET apponnx_pro POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNX_RUN_TIME_ROOT}/lib/onnxruntime.dll
        $<TARGET_FILE_DIR:apponnx_pro>

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNX_RUN_TIME_ROOT}/lib/onnxruntime_providers_cuda.dll
        $<TARGET_FILE_DIR:apponnx_pro>

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNX_RUN_TIME_ROOT}/lib/onnxruntime_providers_shared.dll
        $<TARGET_FILE_DIR:apponnx_pro>

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNX_RUN_TIME_ROOT}/lib/onnxruntime_providers_tensorrt.dll
        $<TARGET_FILE_DIR:apponnx_pro>


    )

endif()

if(WIN32)

    add_custom_command(TARGET apponnx_pro POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:E:/opencv/build/x64/vc15/bin/opencv_world460d.dll>
        $<$<CONFIG:Release>:E:/opencv/build/x64/vc15/bin/opencv_world460.dll>
        $<TARGET_FILE_DIR:apponnx_pro>
    )

endif()

