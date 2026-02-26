cmake_minimum_required(VERSION 3.21)

# Force vitasdk toolchain
if(NOT DEFINED ENV{VITASDK})
    if(EXISTS /usr/local/vitasdk)
        set(ENV{VITASDK} /usr/local/vitasdk)
    elseif(EXISTS $ENV{HOME}/vitasdk)
        set(ENV{VITASDK} $ENV{HOME}/vitasdk)
    endif()
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
            CACHE PATH "Vita toolchain file")
    endif()
endif()

project(ChexQuest2Vita C)

# Verifica che siamo in cross-compilation Vita
if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "VITASDK env var is not set. Please install vitasdk.")
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

include(FetchContent)
FetchContent_Declare(
    doomgeneric
    GIT_REPOSITORY https://github.com/Doom-Generic/Doom-Generic.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(doomgeneric)

set(DOOMGENERIC_DIR ${doomgeneric_SOURCE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -ffast-math")
add_definitions(-DDOOMGENERIC -D__VITA__)

file(GLOB DOOM_SRCS "${DOOMGENERIC_DIR}/doom/*.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/i_main.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/doomgeneric_sdl.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/doomgeneric_win.c")

add_executable(chexquest2_vita
    doomgeneric_vita.c
    ${DOOM_SRCS}
)

target_include_directories(chexquest2_vita PRIVATE
    ${DOOMGENERIC_DIR}/doom
)

target_link_libraries(chexquest2_vita
    vita2d
    SceDisplay_stub
    SceGxm_stub
    SceCtrl_stub
    SceRtc_stub
    SceSysmodule_stub
    SceCommonDialog_stub
    ScePgf_stub
    png
    jpeg
    z
    m
    c
)

vita_create_self(chexquest2.self chexquest2_vita)

vita_create_vpk(ChexQuest2.vpk "CHEX00002" chexquest2.self
    NAME "Chex Quest 2"
)
