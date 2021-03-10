cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(libdfu VERSION 0.1
               DESCRIPTION "DFU-Util project packaged as share library"
               LANGUAGES CXX C)
				  
add_library(libusb STATIC IMPORTED)
set_target_properties(libusb PROPERTIES
  IMPORTED_LOCATION "$ENV{BPCDEV_PATH}/libusb/1.0.23/dll/$ENV{VS_BUILDTOOLS_VERSION}/x86/Release/libusb-1.0.lib"
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{BPCDEV_PATH}/libusb/1.0.23/include/libusb.h"
)

target_include_directories(libusb INTERFACE $ENV{BPCDEV_PATH}/libusb/1.0.23/include)

add_subdirectory(lib_dfuutil)
add_subdirectory(dfudll)
add_subdirectory(blpdevupd)
						

