cmake_minimum_required(VERSION 3.16)
project(progetto_so)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/*.c)
list(REMOVE_ITEM SOURCES ${PROJECT_SOURCE_DIR}/server.c ${PROJECT_SOURCE_DIR}/client.c)

add_definitions(-Wall -std=gnu99)

add_executable(server ${SOURCES} ${PROJECT_SOURCE_DIR}/server.c)
add_executable(client ${SOURCES} ${PROJECT_SOURCE_DIR}/client.c)
