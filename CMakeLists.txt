cmake_minimum_required(VERSION 2.6)
project(yolo)
add_definitions(-std=c++11)

option(CUDA_USE_STATIC_CUDA_RUNTIME OFF)
set(CMAKE_CXX_STANDARD 11)
# 编译MODE
set(CMAKE_BUILD_TYPE Release)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/workspace)

set(CUDA_GEN_CODE "-gencode=arch=compute_86,code=sm_86")

set(OpenCV_DIR   "/usr/local/cuda")
set(CUDA_DIR     "/usr/local/cuda-11.7")
set(CUDNN_DIR    "/home/Downloads/cudnn-linux-x86_64-8.3.3.40_cuda11.5-archive")
set(TENSORRT_DIR "/home/Downloads/TensorRT-8.4.1.5")

find_package(CUDA REQUIRED)
find_package(OpenCV)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_DIR}/include
    ${TENSORRT_DIR}/include
    ${CUDNN_DIR}/include
)

link_directories(
    ${TENSORRT_DIR}/lib
    ${CUDA_DIR}/lib64
    ${CUDNN_DIR}/lib
)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -O0 -Wfatal-errors -pthread -w -g")
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++11 -O0 -Xcompiler -fPIC -g -w ${CUDA_GEN_CODE}")
file(GLOB_RECURSE cpp_srcs ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE cuda_srcs ${PROJECT_SOURCE_DIR}/src/*.cu)

cuda_add_library(cucodes SHARED ${cuda_srcs})

add_executable(yolo ${cpp_srcs})

target_link_libraries(cucodes nvinfer nvparsers nvonnxparser)
target_link_libraries(cucodes cuda cublas cudart cudnn)
target_link_libraries(yolo ${OpenCV_LIBS})
target_link_libraries(yolo cucodes)

add_custom_target(
    run
    DEPENDS yolo
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/workspace
    COMMAND ./yolo
)