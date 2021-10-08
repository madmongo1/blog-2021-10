cmake_minimum_required(VERSION 3.20)

project(blog-oct-2021)

find_package(asio REQUIRED)
add_executable(scratch scratch.cpp)
target_link_libraries(scratch PUBLIC asio::asio)
set_property(TARGET scratch PROPERTY CXX_VERSION 20)
