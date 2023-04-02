cmake_minimum_required(VERSION 3.18)
find_package (Seastar REQUIRED)


add_executable (mini-redis src/main.cc)
set_property(TARGET mini-redis PROPERTY CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS  "-fcoroutines")
  
target_link_libraries (mini-redis Seastar::seastar)
