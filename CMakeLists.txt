#----------------------------------------------------------------------
#
#  Single example osmium-contrib CMakeLists.txt
#
#----------------------------------------------------------------------
project(minjur)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 2.8.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Boost REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(Osmium REQUIRED COMPONENTS io)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})

include(common)

#----------------------------------------------------------------------

include_directories(include)

add_executable(minjur minjur.cpp json_feature.cpp json_handler.cpp)
target_link_libraries(minjur ${OSMIUM_LIBRARIES})

add_executable(minjur-generate-tilelist minjur-generate-tilelist.cpp)
target_link_libraries(minjur-generate-tilelist ${OSMIUM_LIBRARIES})

add_executable(minjur-mp minjur-mp.cpp json_feature.cpp json_handler.cpp)
target_link_libraries(minjur-mp ${OSMIUM_LIBRARIES})


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS --enable=all)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    add_custom_target(cppcheck ${CPPCHECK} --std=c++11 ${CPPCHECK_OPTIONS} ${CMAKE_SOURCE_DIR}/*pp)
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available")
endif()


#-----------------------------------------------------------------------------
#
#  Optional "iwyu" target to check headers
#  http://include-what-you-use.org/
#
#-----------------------------------------------------------------------------
find_program(IWYU_TOOL iwyu_tool.py)

if(IWYU_TOOL)
    message(STATUS "Looking for iwyu_tool.py - found")
    add_custom_target(iwyu ${IWYU_TOOL} -p ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Looking for iwyu_tool.py - not found")
    message(STATUS "  Make target 'iwyu' will not be available")
endif()


