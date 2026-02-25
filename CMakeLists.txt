cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
        CACHE PATH "Vita toolchain file")
  else()
    message(FATAL_ERROR "Please set VITASDK environment variable")
  endif()
endif()

project(ChexQuest2Vita C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Chex Quest 2")
set(VITA_TITLEID  "CHEX00002")
set(VITA_VERSION  "01.00")

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-missing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-result")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign -Wno-implicit-function-declaration")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-compare -Wno-format -Wno-parentheses")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-int -Wno-return-type")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion -Wno-incompatible-pointer-types")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVITA")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFEATURE_SOUND=0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDOOMGENERIC")

# Gather all doom source files from build_src
file(GLOB DOOM_SRCS "${CMAKE_SOURCE_DIR}/build_src/*.c")

# Remove any duplicate main() - doomgeneric has a main in doomgeneric.c
# but we override it in doomgeneric_vita.c
# Actually doomgeneric_Create is called from our main, so check
# if there's a conflicting main
# doomgeneric.c has: int main(int argc, char **argv) only if not defined otherwise
# We handle this by defining DOOMGENERIC_VITA

add_executable(${PROJECT_NAME}
  ${DOOM_SRCS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/build_src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  VITA=1
  DOOMGENERIC=1
)

target_link_libraries(${PROJECT_NAME}
  SceDisplay_stub
  SceCtrl_stub
  SceTouch_stub
  ScePower_stub
  SceIofilemgr_stub
  SceKernelThreadMgr_stub
  SceKernelMemMgr_stub
  SceLibKernel_stub
  SceProcessmgr_stub
  SceAppMgr_stub
  SceAudio_stub
  SceSysmodule_stub
  SceCommonDialog_stub
  SceGxm_stub
  m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
