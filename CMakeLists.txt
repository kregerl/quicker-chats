cmake_minimum_required(VERSION 3.20)
project(quicker_chats C)

set(CMAKE_C_STANDARD 99)

add_executable(quicker_chats src/main.c src/chatter.c src/chatter.h)
target_link_libraries(quicker_chats -lX11 -lXext -lXi)
