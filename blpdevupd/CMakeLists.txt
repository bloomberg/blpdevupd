cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

				  
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_MULTITHREADED ON)

set(Boost_INCLUDE_DIR $ENV{BPCDEV_PATH}/boost/1_68_0/include/boost-1_68)
set(Boost_LIBRARY_DIR $ENV{BPCDEV_PATH}/boost/1_68_0/lib/$ENV{VS_BUILDTOOLS_VERSION}/32)
find_package(Boost 1.68.0 REQUIRED COMPONENTS thread filesystem system program_options REQUIRED)

if(NOT Boost_FOUND)
message("Can not find Boost librray" ERROR_FATAL)
endif()

add_library(dfutransport STATIC dfutransport.cpp dfutransport.h)
target_include_directories(dfutransport PUBLIC ./)

add_library(dfusvc_command STATIC dfusvc_command.cpp dfusvc_command.h dfusvc_command_factory.h dfusvc_command_factory.cpp)
target_include_directories(dfusvc_command PUBLIC ./)

add_library(dfusvc_server STATIC dfusvc_server.cpp dfusvc_server.h)
target_include_directories(dfusvc_server PUBLIC ./)
target_link_libraries(dfusvc_server dfusvc_command dfutransport ${Boost_LIBRARIES})

include_directories(${Boost_INCLUDE_DIRS})

add_executable(blpdevupd dfusvc.cpp)
target_link_libraries(blpdevupd dfusvc_server ${Boost_LIBRARIES})
add_custom_command(TARGET blpdevupd POST_BUILD         
    COMMAND ${CMAKE_COMMAND} -E copy_if_different      
        "$ENV{BPCDEV_PATH}/libusb/1.0.23/dll/$ENV{VS_BUILDTOOLS_VERSION}/x86/Release/libusb-1.0.dll"  
        $<TARGET_FILE_DIR:blpdevupd>)                  
