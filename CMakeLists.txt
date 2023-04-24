cmake_minimum_required(VERSION 3.25)
project(Lab_6_client)

set(CMAKE_CXX_STANDARD 17)

add_executable(Lab_6_client main.cpp sha-1.h)
target_link_libraries(Lab_6_client Ws2_32.lib)