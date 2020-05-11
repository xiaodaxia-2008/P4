cmake_minimum_required(VERSION 3.1)
project(P4)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# set(Eigen3_DIR ${CMAKE_SOURCE_DIR}/dependencies/osqp/build)
set(osqp_DIR $ENV{HOME}/Rvbust/Install/osqp/lib/cmake/osqp)

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/Rvbust/Install/P4)
add_subdirectory(src)
add_subdirectory(examples)
install(FILES P4Config.cmake DESTINATION cmake)
