cmake_minimum_required(VERSION 3.10)
project(ChexQuest2Vita)

# Build command:
# export VITASDK=/usr/local/vitasdk
# mkdir build && cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.cmake
# make

include(FetchContent)

FetchContent_Declare(
    doomgeneric
    GIT_REPOSITORY https://github.com/Doom-Generic/Doom-Generic.git
    GIT_TAG master
)

FetchContent_MakeAvailable(doomgeneric)

set(DOOMGENERIC_DIR ${doomgeneric_SOURCE_DIR})

# Opzioni di ottimizzazione
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -ffast-math")
add_definitions(-DDOOMGENERIC -D__VITA__)

# Raccoglie tutti i sorgenti originali di doomgeneric
file(GLOB DOOM_SRCS "${DOOMGENERIC_DIR}/doom/*.c")

# Rimuove eventuali file main() di altre piattaforme dentro doomgeneric se presenti
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/i_main.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/doomgeneric_sdl.c")
list(REMOVE_ITEM DOOM_SRCS "${DOOMGENERIC_DIR}/doom/doomgeneric_win.c")

# Aggiunge il nostro sorgente PS Vita e i file di Doom
add_executable(chexquest2_vita doomgeneric_vita.c ${DOOM_SRCS})

target_include_directories(chexquest2_vita PRIVATE ${DOOMGENERIC_DIR}/doom)

# Linka le librerie necessarie (Vita2D e le API di base della Vita)
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

# Genera l'eseguibile e il VPK
vita_create_self(chexquest2.self chexquest2_vita)
vita_create_vpk(ChexQuest2.vpk "CHEX00002" chexquest2.self
    NAME "Chex Quest 2"
)
