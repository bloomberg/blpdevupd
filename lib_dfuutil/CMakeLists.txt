cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

add_library(lib_dfuutil STATIC)
target_include_directories(lib_dfuutil PUBLIC ./)

target_sources(lib_dfuutil
	PRIVATE portable.h config.h dfu.c dfu.h dfu_file.c dfu_load.c dfu_util.c dfuse.c dfuse_mem.c quirks.c quirks.h
	PUBLIC dfu.h dfu_file.h dfu_load.h dfu_util.h dfuse.h dfuse_mem.h)

target_link_libraries(lib_dfuutil PRIVATE libusb)
						