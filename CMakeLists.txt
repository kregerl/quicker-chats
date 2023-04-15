cmake_minimum_required(VERSION 3.20)
project(rl_quick_chatter C)

set(CMAKE_C_STANDARD 99)

add_executable(rl_quick_chatter main.c chatter.c chatter.h)
target_link_libraries(rl_quick_chatter -lX11 -lXext -lXi)
