cmake_minimum_required(VERSION 3.21)

# --- Rileva VitaSDK ---
if(NOT DEFINED ENV{VITASDK})
    if(EXISTS /usr/local/vitasdk)
        set(ENV{VITASDK} /usr/local/vitasdk)
    elseif(EXISTS $ENV{HOME}/vitasdk)
        set(ENV{VITASDK} $ENV{HOME}/vitasdk)
    endif()
endif()

if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "VITASDK non trovato.")
endif()

# Toolchain file PRIMA di project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.cmake"
        CACHE PATH "Vita toolchain" FORCE)
endif()

project(ChexQuest2Vita C)

# --- Fetch doomgeneric (URL CORRETTO) ---
include(FetchContent)
FetchContent_Declare(
    doomgeneric
    GIT_REPOSITORY https://github.com/ozkl/doomgeneric.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doomgeneric)
set(DOOMGENERIC_DIR ${doomgeneric_SOURCE_DIR}/doomgeneric)

# --- Flags ---
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -ffast-math")
add_definitions(-DDOOMGENERIC -D__VITA__)

# --- Sorgenti Doom ---
file(GLOB DOOM_SRCS "${DOOMGENERIC_DIR}/*.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/i_main.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doomgeneric_sdl.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doomgeneric_win.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doomgeneric_xlib.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doomgeneric_ascii.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doomgeneric_null.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doomgeneric_soso.c")

# --- Target ---
add_executable(chexquest2_vita
    doomgeneric_vita.c
    ${DOOM_SRCS}
)

target_include_directories(chexquest2_vita PRIVATE
    ${DOOMGENERIC_DIR}
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

# --- Genera .self e .vpk ---
vita_create_self(chexquest2.self chexquest2_vita)

vita_create_vpk(ChexQuest2.vpk "CHEX00002" chexquest2.self
    NAME "Chex Quest 2"
)
