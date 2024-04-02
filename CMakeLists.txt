cmake_minimum_required(VERSION 3.28)
project(lb4)

set(CMAKE_CXX_STANDARD 17)

find_package(ZLIB)

add_library(lb4mailbox SHARED lb4mailbox.cpp)
target_link_libraries(lb4mailbox ZLIB::ZLIB)

add_executable(lb4 main.cpp)
target_link_libraries(lb4 lb4mailbox)
