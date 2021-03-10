cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

add_library(libdfu SHARED libdfu.cpp libdfu.def libdfu_util.h libdfu_util.c)
target_link_libraries(libdfu PRIVATE libusb lib_dfuutil)
						