#include "doomgeneric.h"
#include <psp2/ctrl.h>
#include <psp2/rtc.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>
#include <string.h>
#include <stdlib.h>

#define QUEUE_SIZE 64

static int key_queue[QUEUE_SIZE];
static int pressed_queue[QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;

static SceCtrlData old_pad;
static vita2d_texture *frame_tex;

// Struttura per mappare i tasti Vita ai tasti di Doom
struct ButtonMap {
    uint32_t vita_btn;
    unsigned char doom_key;
};

// Mappatura tasti
static const struct ButtonMap bmap[] = {
    {SCE_CTRL_UP, KEY_UPARROW},
    {SCE_CTRL_DOWN, KEY_DOWNARROW},
    {SCE_CTRL_LEFT, KEY_LEFTARROW},
    {SCE_CTRL_RIGHT, KEY_RIGHTARROW},
    {SCE_CTRL_CROSS, KEY_RCTRL},       // Spara (Zorch)
    {SCE_CTRL_SQUARE, ' '},            // Usa/Azione
    {SCE_CTRL_CIRCLE, KEY_ESCAPE},     // Menu / Indietro
    {SCE_CTRL_TRIANGLE, KEY_ENTER},    // Conferma
    {SCE_CTRL_LTRIGGER, ','},          // Strafe Sinistra
    {SCE_CTRL_RTRIGGER, '.'},          // Strafe Destra
    {SCE_CTRL_START, KEY_ESCAPE},      // Pausa / Menu
    {SCE_CTRL_SELECT, KEY_TAB}         // Mappa
};

// Aggiunge un evento tasto alla coda circolare
static void add_key(int key, int pressed) {
    int next_head = (queue_head + 1) % QUEUE_SIZE;
    if (next_head != queue_tail) { // Evita l'overflow
        key_queue[queue_head] = key;
        pressed_queue[queue_head] = pressed;
        queue_head = next_head;
    }
}

void DG_Init() {
    // Inizializza Vita2D
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));
    
    // Crea una texture delle dimensioni di Doom (320x200)
    frame_tex = vita2d_create_empty_texture(DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    
    // Inizializza il pad
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceCtrlPeekBufferPositive(0, &old_pad, 1);
}

void DG_DrawFrame() {
    // Doomgeneric genera colori nel formato 0x00RRGGBB.
    // Dobbiamo convertirli per la texture di Vita2D (tipicamente ABGR su PS Vita)
    uint32_t *tex_data = (uint32_t *)vita2d_texture_get_datap(frame_tex);
    
    for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++) {
        uint32_t c = DG_ScreenBuffer[i];
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8)  & 0xFF;
        uint8_t b = c & 0xFF;
        tex_data[i] = RGBA8(r, g, b, 255);
    }

    vita2d_start_drawing();
    vita2d_clear_screen();
    
    // Scala l'immagine originale (320x200) alla risoluzione PS Vita (960x544)
    // 960 / 320 = 3.0f  |  544 / 200 = 2.72f
    vita2d_draw_texture_scale(frame_tex, 0, 0, 3.0f, 2.72f);
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void DG_SleepMs(uint32_t ms) {
    sceKernelDelayThread(ms * 1000);
}

uint32_t DG_GetTicksMs() {
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    return (uint32_t)(tick.tick / 1000); // tick è in microsecondi
}

int DG_GetKey(int* pressed, unsigned char* key) {
    if (queue_tail == queue_head) return 0; // Coda vuota
    
    *key = key_queue[queue_tail];
    *pressed = pressed_queue[queue_tail];
    queue_tail = (queue_tail + 1) % QUEUE_SIZE;
    
    return 1;
}

void DG_SetWindowTitle(const char * title) {
    // Non necessario su console
}

int main(int argc, char **argv) {
    // Parametri hardcoded per avviare Chex Quest 2.
    // Modifica le path se preferisci cartelle diverse sulla Vita.
    char *cq_argv[] = {
        "doom", 
        "-iwad", "ux0:data/ChexQuest/chex.wad", 
        "-file", "ux0:data/ChexQuest/chex2.wad"
    };
    
    doomgeneric_Create(5, cq_argv);

    while (1) {
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(0, &pad, 1);

        // Controlla variazioni di stato per ogni tasto
        for (int i = 0; i < sizeof(bmap) / sizeof(bmap[0]); i++) {
            int old_p = (old_pad.buttons & bmap[i].vita_btn) ? 1 : 0;
            int new_p = (pad.buttons & bmap[i].vita_btn) ? 1 : 0;
            
            if (old_p != new_p) {
                add_key(bmap[i].doom_key, new_p);
            }
        }
        old_pad = pad;

        doomgeneric_Tick();
    }
    
    vita2d_fini();
    return 0;
}
