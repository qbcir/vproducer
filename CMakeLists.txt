cmake_minimum_required(VERSION 3.0)
project(nnxcam)

set(CMAKE_CXX_STANDARD 14)

include_directories("src")

file(GLOB SRC_VPRODUCER
    "src/common/*.cpp"
    "src/vproducer/*.cpp")
add_executable(vproducer ${SRC_VPRODUCER})
target_link_libraries(vproducer jsoncpp avcodec avformat avutil coda)
