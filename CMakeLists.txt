#----------------------------------------------------------------------
#
#  Single example osmium-contrib CMakeLists.txt
#
#----------------------------------------------------------------------
project(minjur)

cmake_minimum_required(VERSION 2.8.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Osmium REQUIRED COMPONENTS io)
include_directories(${OSMIUM_INCLUDE_DIRS})

include(common)

#----------------------------------------------------------------------

set(PROG minjur)
set(SOURCES minjur.cpp)
include_directories(include)
add_executable(${PROG} ${SOURCES})
target_link_libraries(${PROG} ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})


#----------------------------------------------------------------------
