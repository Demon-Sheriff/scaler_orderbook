cmake_minimum_required(VERSION 3.17)
project(SPPE_MarketData LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)
find_package(fmt REQUIRED)

include_directories(common)

add_executable(publisher publisher/main.cpp)
target_link_libraries(publisher PRIVATE Boost::boost fmt::fmt rt pthread)

add_executable(consumer_shm consumer_shm/main.cpp)
target_link_libraries(consumer_shm PRIVATE fmt::fmt rt pthread)

add_executable(consumer_tcp consumer_tcp/main.cpp)
target_link_libraries(consumer_tcp PRIVATE Boost::boost fmt::fmt pthread)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
