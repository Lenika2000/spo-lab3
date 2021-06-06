cmake_minimum_required(VERSION 3.1)
project(spo-lab3 C)
find_package(Threads REQUIRED)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})
add_executable(main main.c
        includes/util.h util.c
        includes/net.h net.c
        includes/client.h client/client.c
        includes/client_net.h client/client_net.c
        includes/server.h server/server.c
        includes/server_net.h server/server_net.c)
target_link_libraries(main Threads::Threads ${CURSES_LIBRARIES})
