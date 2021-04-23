cmake_minimum_required(VERSION 3.18)

project(cpplua)

include_directories(include)

add_definitions(
    -D_FORTIFY_SOURCE=2 
    -O2
    -g
    -Wall
    -Wconversion -Wsign-conversion
    -Werror
    -Wextra
    -Wformat-security
    -Wl,-z,noexecstack
    -Wl,-z,relro,-z,now
    -Wstack-protector
    -fPIE 
    -fstack-clash-protection
    -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4
    -pedantic
    -pie 
)

include(FindLua)

set(CMAKE_CXX_STANDARD 17)

add_library(cpplua STATIC
    src/cpplua.cpp
)

target_include_directories(cpplua PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(cpplua ${LUA_LIBRARIES})

add_executable(test src/main.cpp)

target_link_libraries(test cpplua)

