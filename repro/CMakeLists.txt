cmake_minimum_required(VERSION 3.12)
project(minizip_reproducer C)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." minizip)
add_executable(minizip_repro finalization.c)
target_link_libraries(minizip_repro PRIVATE MINIZIP::minizip)
set_target_properties(minizip_repro PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
