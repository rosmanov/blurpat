cmake_minimum_required (VERSION 2.6)
project (BlurPattern CXX C)

option(DEBUG "Enable debugging support" OFF)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(ImToolsCompiler)
include(CheckIncludeFiles)
include(CheckSymbolExists)

find_package(LibOpenCV REQUIRED)

if (DEBUG)
  set (CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DDEBUG)
else (DEBUG)
  set (CMAKE_BUILD_TYPE "Release")
endif (DEBUG)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${LIBOPENCV_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB}
${LIBOPENCV_HIGHGUI_LIB}")

list(APPEND LIBS ${LIBOPENCV_LIBS} ${Boost_LIBRARIES})

include_directories(${LIBOPENCV_INCLUDE_DIR})

if (NOT CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
endif ()

set(src src/main.cxx src/exceptions.cxx)

set(target blurpat)
add_executable(blurpat ${src})
target_link_libraries(blurpat ${LIBS})

install(TARGETS blurpat DESTINATION "bin")
# vim: et ts=2 sts=2 sw=2
