cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(chexquest2-vita C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Chex Quest 2")
set(VITA_TITLEID  "CHEX00002")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -ffast-math")

# Definizioni necessarie per doomgeneric
add_definitions(-DDOOMGENERIC_RESX=320)
add_definitions(-DDOOMGENERIC_RESY=200)
add_definitions(-DFEATURE_SOUND)

# Source files doomgeneric
set(DOOMGENERIC_DIR ${CMAKE_SOURCE_DIR}/doomgeneric)

file(GLOB DOOM_SOURCES
    "${DOOMGENERIC_DIR}/*.c"
)

# Rimuovi altri backend
list(FILTER DOOM_SOURCES EXCLUDE REGEX ".*doomgeneric_sdl\\.c$")
list(FILTER DOOM_SOURCES EXCLUDE REGEX ".*doomgeneric_win\\.c$")
list(
