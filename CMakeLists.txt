project(mhs5200)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 14)


set(TARGET_EXE mhs5200)
file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(${TARGET_EXE} ${SRC_FILES})
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
    target_link_libraries(${TARGET_EXE} stdc++fs)
endif()
