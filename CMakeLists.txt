cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ChexQuest2Vita C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Chex Quest 2")
set(VITA_TITLEID  "CHEX00002")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-result -Wno-pointer-sign -Wno-implicit-function-declaration -Wno-sign-compare -Wno-format -Wno-parentheses -fno-strict-aliasing -DVITA -DFEATURE_SOUND=0")

# Gather all doom source files
file(GLOB DOOM_SRCS "${CMAKE_SOURCE_DIR}/build_src/*.c")

add_executable(${PROJECT_NAME}
  ${DOOM_SRCS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/build_src
)

target_link_libraries(${PROJECT_NAME}
  SceDisplay_stub
  SceCtrl_stub
  SceTouch_stub
  SceGxm_stub
  SceSysmodule_stub
  SceCommonDialog_stub
  ScePower_stub
  SceIofilemgr_stub
  SceKernelThreadMgr_stub
  SceKernelMemMgr_stub
  SceLibKernel_stub
  SceProcessmgr_stub
  SceAppMgr_stub
  SceAudio_stub
  m
  c
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
