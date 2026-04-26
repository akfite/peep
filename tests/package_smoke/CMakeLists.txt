cmake_minimum_required(VERSION 3.14)
project(peep_package_smoke LANGUAGES CXX)

find_package(peep CONFIG REQUIRED)

add_executable(package_smoke package_smoke.cpp)
target_link_libraries(package_smoke PRIVATE peep::peep)
