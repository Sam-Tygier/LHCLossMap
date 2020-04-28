
set(MERLIN_BUILD_DIR ${MERLIN_ROOT_DIR})
set(MERLIN_INCLUDE-DIR ${MERLIN_ROOT_DIR}/../Merlin++)



message(STATUS "MERLIN_ROOT_DIR ${MERLIN_ROOT_DIR}")
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project (LHCLossMaps)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -pedantic")


find_library(MERLIN NAMES MERLIN libmerlin++.so PATHS ${MERLIN_BUILD_DIR})
include_directories(${MERLIN_INCLUDE-DIR})
message(STATUS "MERLIN ${MERLIN}")

add_library(settings STATIC settings.cpp)
add_library(accelerator_sim STATIC accelerator_sim.cpp)

target_link_libraries(accelerator_sim ${MERLIN} settings)

add_executable(loss_map loss_map.cpp )
target_link_libraries(loss_map accelerator_sim)

add_executable(optics_check optics_check.cpp )
target_link_libraries(optics_check accelerator_sim)

add_executable(tracks tracks.cpp TrackingOutputASCII.cpp)
target_link_libraries(tracks accelerator_sim)

add_executable(poincare poincare.cpp)
target_link_libraries(poincare accelerator_sim)
