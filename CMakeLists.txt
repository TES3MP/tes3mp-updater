cmake_minimum_required(VERSION 3.10)
project(tes3mp_updater)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

if (UNIX AND NOT APPLE)
    find_package (Threads)
endif()

find_package(Boost 1.66 REQUIRED COMPONENTS
        system
        filesystem
        program_options)

find_package(OpenSSL REQUIRED)

include_directories("."
        SYSTEM
        ${Boost_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/extern/json/single_include/nlohmann
        )

link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(components)
add_subdirectory(apps/update-server)
add_subdirectory(apps/update-client)
