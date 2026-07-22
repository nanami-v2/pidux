cmake_minimum_required(VERSION 3.26)
project(sample LANGUAGES CXX)

find_package(pidux CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS circular_buffer)

add_executable(sample ./src/main.cpp)
target_link_libraries(sample PRIVATE pidux::pidux Boost::circular_buffer)
target_compile_features(sample PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(sample PRIVATE /W4 /WX)
endif()