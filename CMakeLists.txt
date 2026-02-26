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
    message(FATAL_ERROR
        "VITASDK non trovato. Installa VitaSDK e imposta la variabile d'ambiente VITASDK.")
endif()

# Verifica che il compilatore esista davvero
if(NOT EXISTS "$ENV{VITASDK}/bin/arm-vita-eabi-gcc")
    message(FATAL_ERROR
        "arm-vita-eabi-gcc non trovato in $ENV{VITASDK}/bin/\n"
        "Il toolchain VitaSDK non e' installato completamente.\n"
        "Esegui: git clone https://github.com/vitasdk/vdpm && cd vdpm && ./bootstrap-vitasdk.sh")
endif()

# Usa vita.cmake come toolchain file (PRIMA di project())
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.cmake"
        CACHE PATH "Vita toolchain file")
endif()

project(ChexQuest2Vita C)

# --- Fetch doomgeneric ---
include(FetchContent)
FetchContent_Declare(
    doomgeneric
    GIT_REPOSITORY https://github.com/Doom-Generic/Doom-Generic.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(doomgeneric)
set(DOOMGENERIC_DIR ${doomgeneric_SOURCE_DIR})

# --- Flags ---
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -ffast-math")
add_definitions(-DDOOMGENERIC -D__VITA__)

# --- Sorgenti Doom ---
file(GLOB DOOM_SRCS "${DOOMGENERIC_DIR}/doom/*.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/i_main.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/doomgeneric_sdl.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/doomgeneric_win.c")

# --- Target ---
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

# --- Genera .self e .vpk ---
vita_create_self(chexquest2.self chexquest2_vita)

vita_create_vpk(ChexQuest2.vpk "CHEX00002" chexquest2.self
    NAME "Chex Quest 2"
)
