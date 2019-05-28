cmake_minimum_required(VERSION 3.0)
project(nnxcam)

set(CMAKE_CXX_STANDARD 14)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
#target_link_libraries(<your exe or lib> ${PYTHON_LIBRARIES})

file(GLOB SRC_VPRODUCER
    "src/*.cpp"
    "main.cpp")

include_directories("/usr/local/lib/python3.6/dist-packages/numpy/core/include/")

add_executable(vproducer ${SRC_VPRODUCER})
target_link_libraries(vproducer jsoncpp avcodec avformat avutil ${PYTHON_LIBRARIES} coda)
