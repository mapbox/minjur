#----------------------------------------------------------------------
#
#  Single example osmium-contrib CMakeLists.txt
#
#----------------------------------------------------------------------
project(minjur)

cmake_minimum_required(VERSION 2.8.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Osmium REQUIRED COMPONENTS io)
include_directories(${OSMIUM_INCLUDE_DIRS})

include(common)

#----------------------------------------------------------------------

include_directories(include)

add_executable(minjur minjur.cpp)
target_link_libraries(minjur ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})

add_executable(minjur-generate-tilelist minjur-generate-tilelist.cpp)
target_link_libraries(minjur-generate-tilelist ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})

#----------------------------------------------------------------------
