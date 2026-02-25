// doomgeneric_vita.c - Chex Quest 2 for PS Vita
// Implementazione completa di doomgeneric per PS Vita

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/audioout.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/power.h>

#include <vita2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "doomgeneric.h"
#include "doomkeys.h"
#include "i_sound.h"
#include "m_argv.h"
#include "d_main.h"

// Configurazione display
#define VITA_SCREEN_W 960
#define VITA_SCREEN_H 544
#define DOOM_SCREEN_W DOOMGENERIC_RESX
#define DOOM_SCREEN_H DOOMGENERIC_RESY

// Configurazione audio
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_SAMPLES 1024

// Buffer dei tasti
#define MAX_KEY_QUEUE 32

// Struttura per la coda dei tasti
typedef struct {
    unsigned char key;
    int pressed;
} key_event_t;

static key_event_t key_queue[MAX_KEY_QUEUE];
static int key_queue_read = 0;
static int key_queue_write = 0;

// Stato precedente dei controlli
static SceCtrlData prev_ctrl;

// Texture per il framebuffer
static vita2d_texture *doom_texture = NULL;

// Variabili audio
static int audio_port = -1;
static SceUID audio_thread_id = -1;
static volatile bool audio_running = false;

// Tick iniziale
static uint64_t start_time = 0;

// Mappatura controlli Vita -> DOOM
typedef struct {
    uint32_t vita_button;
    unsigned char doom_key;
} button_mapping_t;

static const button_mapping_t button_map[] = {
    { SCE_CTRL_UP,       KEY_UPARROW },
    { SCE_CTRL_DOWN,     KEY_DOWNARROW },
    { SCE_CTRL_LEFT,     KEY_LEFTARROW },
    { SCE_CTRL_RIGHT,    KEY_RIGHTARROW },
    { SCE_CTRL_CROSS,    KEY_USE },
    { SCE_CTRL_CIRCLE,   KEY_FIRE },
    { SCE_CTRL_SQUARE,   KEY_STRAFE_L },
    { SCE_CTRL_TRIANGLE, KEY_STRAFE_R },
    { SCE_CTRL_LTRIGGER, KEY_RSHIFT },  // Run
    { SCE_CTRL_RTRIGGER, KEY_FIRE },
    { SCE_CTRL_START,    KEY_ESCAPE },
    { SCE_CTRL_SELECT,   KEY_TAB },     // Mappa
    { 0, 0 }
};

// Mappatura armi numeriche
static const unsigned char weapon_keys[] = {
    '1', '2', '3', '4', '5', '6', '7'
};
static int current_weapon = 0;

// ============================================================================
// FUNZIONI DI UTILITÀ
// ============================================================================

static void add_key_event(unsigned char key, int pressed) {
    key_queue[key_queue_write].key = key;
    key_queue[key_queue_write].pressed = pressed;
    key_queue_write = (key_queue_write + 1) % MAX_KEY_QUEUE;
}

static int get_key_event(unsigned char *key, int *pressed) {
    if (key_queue_read == key_queue_write)
        return 0;
    
    *key = key_queue[key_queue_read].key;
    *pressed = key_queue[key_queue_read].pressed;
    key_queue_read = (key_queue_read + 1) % MAX_KEY_QUEUE;
    return 1;
}

// ============================================================================
// IMPLEMENTAZIONE DOOMGENERIC
// ============================================================================

void DG_Init(void) {
    // Inizializza sistema di potenza (CPU a piena velocità)
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    
    // Inizializza vita2d
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));
    
    // Crea texture per DOOM
    doom_texture = vita2d_create_empty_texture_format(
        DOOM_SCREEN_W, 
        DOOM_SCREEN_H,
        SCE_GXM_TEXTURE_FORMAT_A8B8G8R8
    );
    
    // Inizializza controlli
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    memset(&prev_ctrl, 0, sizeof(prev_ctrl));
    
    // Inizializza touch (opzionale per menu)
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    
    // Salva tempo di avvio
    start_time = sceKernelGetProcessTimeWide();
    
    printf("Chex Quest 2 Vita - Inizializzato!\n");
}

void DG_DrawFrame(void) {
    if (!doom_texture || !DG_ScreenBuffer)
        return;
    
    // Copia framebuffer DOOM nella texture
    uint32_t *tex_data = (uint32_t *)vita2d_texture_get_datap(doom_texture);
    uint32_t *src = (uint32_t *)DG_ScreenBuffer;
    
    int stride = vita2d_texture_get_stride(doom_texture) / 4;
    
    for (int y = 0; y < DOOM_SCREEN_H; y++) {
        for (int x = 0; x < DOOM_SCREEN_W; x++) {
            // Converti da BGRA a RGBA se necessario
            uint32_t pixel = src[y * DOOM_SCREEN_W + x];
            tex_data[y * stride + x] = pixel;
        }
    }
    
    // Calcola scaling per mantenere aspect ratio
    float scale_x = (float)VITA_SCREEN_W / DOOM_SCREEN_W;
    float scale_y = (float)VITA_SCREEN_H / DOOM_SCREEN_H;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    int draw_w = (int)(DOOM_SCREEN_W * scale);
    int draw_h = (int)(DOOM_SCREEN_H * scale);
    int draw_x = (VITA_SCREEN_W - draw_w) / 2;
    int draw_y = (VITA_SCREEN_H - draw_h) / 2;
    
    // Rendering
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_texture_scale(
        doom_texture,
        draw_x, draw_y,
        scale, scale
    );
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
    vita2d_wait_rendering_done();
}

void DG_SleepMs(uint32_t ms) {
    sceKernelDelayThread(ms * 1000);
}

uint32_t DG_GetTicksMs(void) {
    uint64_t now = sceKernelGetProcessTimeWide();
    return (uint32_t)((now - start_time) / 1000);
}

int DG_GetKey(int *pressed, unsigned char *key) {
    // Leggi stato controlli
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    
    // Controlla cambiamenti nei pulsanti
    for (int i = 0; button_map[i].vita_button != 0; i++) {
        uint32_t btn = button_map[i].vita_button;
        
        // Pulsante appena premuto
        if ((ctrl.buttons & btn) && !(prev_ctrl.buttons & btn)) {
            add_key_event(button_map[i].doom_key, 1);
        }
        // Pulsante appena rilasciato
        else if (!(ctrl.buttons & btn) && (prev_ctrl.buttons & btn)) {
            add_key_event(button_map[i].doom_key, 0);
        }
    }
    
    // Analog stick sinistro per movimento
    int lx = ctrl.lx - 128;
    int ly = ctrl.ly - 128;
    static int prev_lx = 0, prev_ly = 0;
    
    // Deadzone
    if (abs(lx) < 30) lx = 0;
    if (abs(ly) < 30) ly = 0;
    
    // Movimento avanti/indietro
    if (ly < -50 && prev_ly >= -50)
        add_key_event(KEY_UPARROW, 1);
    else if (ly >= -50 && prev_ly < -50)
        add_key_event(KEY_UPARROW, 0);
    
    if (ly > 50 && prev_ly <= 50)
        add_key_event(KEY_DOWNARROW, 1);
    else if (ly <= 50 && prev_ly > 50)
        add_key_event(KEY_DOWNARROW, 0);
    
    // Rotazione
    if (lx < -50 && prev_lx >= -50)
        add_key_event(KEY_LEFTARROW, 1);
    else if (lx >= -50 && prev_lx < -50)
        add_key_event(KEY_LEFTARROW, 0);
    
    if (lx > 50 && prev_lx <= 50)
        add_key_event(KEY_RIGHTARROW, 1);
    else if (lx <= 50 && prev_lx > 50)
        add_key_event(KEY_RIGHTARROW, 0);
    
    prev_lx = lx;
    prev_ly = ly;
    
    // Analog stick destro per strafe/look
    int rx = ctrl.rx - 128;
    static int prev_rx = 0;
    
    if (abs(rx) < 30) rx = 0;
    
    if (rx < -50 && prev_rx >= -50)
        add_key_event(KEY_STRAFE_L, 1);
    else if (rx >= -50 && prev_rx < -50)
        add_key_event(KEY_STRAFE_L, 0);
    
    if (rx > 50 && prev_rx <= 50)
        add_key_event(KEY_STRAFE_R, 1);
    else if (rx <= 50 && prev_rx > 50)
        add_key_event(KEY_STRAFE_R, 0);
    
    prev_rx = rx;
    
    // Cambio arma con touch screen
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    static int prev_touch = 0;
    
    if (touch.reportNum > 0 && prev_touch == 0) {
        // Touch nella parte superiore dello schermo per cambiare arma
        if (touch.report[0].y < 200) {
            current_weapon = (current_weapon + 1) % 7;
            add_key_event(weapon_keys[current_weapon], 1);
            add_key_event(weapon_keys[current_weapon], 0);
        }
    }
    prev_touch = touch.reportNum;
    
    // Salva stato precedente
    prev_ctrl = ctrl;
    
    // Restituisci evento dalla coda
    return get_key_event(key, pressed);
}

void DG_SetWindowTitle(const char *title) {
    // Non applicabile su Vita
    printf("Titolo: %s\n", title);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    printf("=================================\n");
    printf("  CHEX QUEST 2 - PS VITA PORT\n");
    printf("  Powered by doomgeneric\n");
    printf("=================================\n");
    
    // Argomenti per caricare Chex Quest 2
    // Il WAD deve essere in ux0:data/chexquest2/
    static char *doom_argv[] = {
        "chexquest2",
        "-iwad", "ux0:data/chexquest2/chex2.wad",
        NULL
    };
    int doom_argc = 3;
    
    // Controlla se il WAD esiste
    SceIoStat stat;
    if (sceIoGetstat("ux0:data/chexquest2/chex2.wad", &stat) < 0) {
        printf("ERRORE: WAD non trovato!\n");
        printf("Copia chex2.wad in:\n");
        printf("ux0:data/chexquest2/chex2.wad\n");
        printf("\nPremi qualsiasi tasto per uscire...\n");
        
        SceCtrlData ctrl;
        while (1) {
            sceCtrlPeekBufferPositive(0, &ctrl, 1);
            if (ctrl.buttons)
                break;
            sceKernelDelayThread(100000);
        }
        
        sceKernelExitProcess(0);
        return 1;
    }
    
    printf("WAD trovato! Avvio...\n");
    
    // Avvia DOOM/Chex Quest
    doomgeneric_Create(doom_argc, doom_argv);
    
    // Loop principale
    while (1) {
        doomgeneric_Tick();
    }
    
    // Cleanup (non raggiunto normalmente)
    if (doom_texture) {
        vita2d_free_texture(doom_texture);
    }
    vita2d_fini();
    sceKernelExitProcess(0);
    
    return 0;
}
