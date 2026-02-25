/*
 * doomgeneric_vita.c - PS Vita platform implementation for doomgeneric
 *
 * This implements the required doomgeneric interface functions:
 *   DG_Init, DG_DrawFrame, DG_SleepMs, DG_GetTicksMs, DG_GetKey, DG_SetWindowTitle
 *
 * Rendering: direct framebuffer via SceDisplay
 * Input: SceCtrl (gamepad) + SceTouch (front touch for virtual buttons)
 * Audio: disabled (stub)
 *
 * The WAD file (chex2.wad) must be placed at ux0:data/chexquest2/chex2.wad
 */

#include "doomgeneric.h"
#include "doomkeys.h"
#include "m_argv.h"

#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/apputil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// PS Vita screen
#define VITA_SCREEN_W 960
#define VITA_SCREEN_H 544
#define VITA_FB_ALIGN 256

// doomgeneric renders at DOOMGENERIC_RESX x DOOMGENERIC_RESY
// We'll scale to fill the Vita screen

static uint32_t *vita_fb = NULL;
static SceUID fb_memuid = -1;
static uint64_t start_time_us = 0;

// Key queue
#define KEY_QUEUE_SIZE 64
static struct {
    unsigned short key;
    int pressed;
} key_queue[KEY_QUEUE_SIZE];
static int key_queue_head = 0;
static int key_queue_tail = 0;

// Previous button state for edge detection
static uint32_t prev_buttons = 0;

// Allocate CDRAM for framebuffer
static void *vita_gpu_alloc(SceKernelMemBlockType type, unsigned int size, SceUID *uid) {
    void *mem = NULL;
    size = (size + 0xFFFFF) & ~0xFFFFF; // Align to 1MB
    *uid = sceKernelAllocMemBlock("fb", type, size, NULL);
    if (*uid < 0)
        return NULL;
    sceKernelGetMemBlockBase(*uid, &mem);
    return mem;
}

static void push_key(unsigned short key, int pressed) {
    int next = (key_queue_head + 1) % KEY_QUEUE_SIZE;
    if (next == key_queue_tail)
        return; // queue full
    key_queue[key_queue_head].key = key;
    key_queue[key_queue_head].pressed = pressed;
    key_queue_head = next;
}

static void check_button(uint32_t buttons, uint32_t prev, uint32_t mask, unsigned short doom_key) {
    if ((buttons & mask) && !(prev & mask)) {
        push_key(doom_key, 1);
    } else if (!(buttons & mask) && (prev & mask)) {
        push_key(doom_key, 0);
    }
}

void DG_Init(void) {
    // Set CPU/GPU clock to max
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    // Initialize controller
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    // Initialize touch
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    // Allocate framebuffer in CDRAM
    int fb_size = VITA_SCREEN_W * VITA_SCREEN_H * 4;
    vita_fb = (uint32_t *)vita_gpu_alloc(
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
        fb_size,
        &fb_memuid
    );

    if (!vita_fb) {
        // Fallback: try main memory
        vita_fb = (uint32_t *)malloc(fb_size);
    }

    memset(vita_fb, 0, fb_size);

    // Set framebuffer
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size = sizeof(fb);
    fb.base = vita_fb;
    fb.pitch = VITA_SCREEN_W;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = VITA_SCREEN_W;
    fb.height = VITA_SCREEN_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);

    // Record start time
    start_time_us = sceKernelGetProcessTimeWide();
}

void DG_DrawFrame(void) {
    if (!vita_fb || !DG_ScreenBuffer)
        return;

    // Scale DOOMGENERIC_RESX x DOOMGENERIC_RESY -> VITA_SCREEN_W x VITA_SCREEN_H
    // Using nearest-neighbor scaling for speed

    int src_w = DOOMGENERIC_RESX;
    int src_h = DOOMGENERIC_RESY;
    int dst_w = VITA_SCREEN_W;
    int dst_h = VITA_SCREEN_H;

    // Precompute X mapping table
    static int x_map[960];
    static int x_map_computed = 0;
    if (!x_map_computed) {
        for (int dx = 0; dx < dst_w; dx++) {
            x_map[dx] = (dx * src_w) / dst_w;
        }
        x_map_computed = 1;
    }

    for (int dy = 0; dy < dst_h; dy++) {
        int sy = (dy * src_h) / dst_h;
        uint32_t *src_row = &DG_ScreenBuffer[sy * src_w];
        uint32_t *dst_row = &vita_fb[dy * dst_w];

        for (int dx = 0; dx < dst_w; dx++) {
            uint32_t pixel = src_row[x_map[dx]];
            // doomgeneric outputs XRGB8888, Vita wants ABGR8888
            uint32_t r = (pixel >> 16) & 0xFF;
            uint32_t g = (pixel >> 8) & 0xFF;
            uint32_t b = pixel & 0xFF;
            dst_row[dx] = 0xFF000000 | (b << 16) | (g << 8) | r;
        }
    }

    // Update display
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size = sizeof(fb);
    fb.base = vita_fb;
    fb.pitch = VITA_SCREEN_W;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = VITA_SCREEN_W;
    fb.height = VITA_SCREEN_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);

    // Process input
    SceCtrlData ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    sceCtrlPeekBufferPositive(0, &ctrl, 1);

    uint32_t buttons = ctrl.buttons;

    // D-pad / analog -> movement
    // Check analog stick for movement
    int lx = ctrl.lx - 128;
    int ly = ctrl.ly - 128;
    int rx = ctrl.rx - 128;
    int ry = ctrl.ry - 128;

    // Left analog -> movement (forward/back/strafe)
    static int prev_up = 0, prev_down = 0, prev_left = 0, prev_right = 0;

    int cur_up = (ly < -40) ? 1 : 0;
    int cur_down = (ly > 40) ? 1 : 0;
    int cur_left = (lx < -40) ? 1 : 0;
    int cur_right = (lx > 40) ? 1 : 0;

    if (cur_up && !prev_up) push_key(KEY_UPARROW, 1);
    if (!cur_up && prev_up) push_key(KEY_UPARROW, 0);
    if (cur_down && !prev_down) push_key(KEY_DOWNARROW, 1);
    if (!cur_down && prev_down) push_key(KEY_DOWNARROW, 0);
    if (cur_left && !prev_left) push_key(KEY_STRAFELEFT, 1);
    if (!cur_left && prev_left) push_key(KEY_STRAFELEFT, 0);
    if (cur_right && !prev_right) push_key(KEY_STRAFERIGHT, 1);
    if (!cur_right && prev_right) push_key(KEY_STRAFERIGHT, 0);

    prev_up = cur_up;
    prev_down = cur_down;
    prev_left = cur_left;
    prev_right = cur_right;

    // Right analog -> turning
    static int prev_turn_left = 0, prev_turn_right = 0;
    int cur_turn_left = (rx < -40) ? 1 : 0;
    int cur_turn_right = (rx > 40) ? 1 : 0;

    if (cur_turn_left && !prev_turn_left) push_key(KEY_LEFTARROW, 1);
    if (!cur_turn_left && prev_turn_left) push_key(KEY_LEFTARROW, 0);
    if (cur_turn_right && !prev_turn_right) push_key(KEY_RIGHTARROW, 1);
    if (!cur_turn_right && prev_turn_right) push_key(KEY_RIGHTARROW, 0);

    prev_turn_left = cur_turn_left;
    prev_turn_right = cur_turn_right;

    // Button mappings
    check_button(buttons, prev_buttons, SCE_CTRL_UP,       KEY_UPARROW);
    check_button(buttons, prev_buttons, SCE_CTRL_DOWN,     KEY_DOWNARROW);
    check_button(buttons, prev_buttons, SCE_CTRL_LEFT,     KEY_LEFTARROW);
    check_button(buttons, prev_buttons, SCE_CTRL_RIGHT,    KEY_RIGHTARROW);

    check_button(buttons, prev_buttons, SCE_CTRL_CROSS,    KEY_USE);         // X = Use/Open
    check_button(buttons, prev_buttons, SCE_CTRL_CIRCLE,   KEY_ESCAPE);      // O = Menu/Back
    check_button(buttons, prev_buttons, SCE_CTRL_SQUARE,   KEY_RSHIFT);      // Square = Run
    check_button(buttons, prev_buttons, SCE_CTRL_TRIANGLE, KEY_TAB);         // Triangle = Map

    check_button(buttons, prev_buttons, SCE_CTRL_RTRIGGER, KEY_FIRE);        // R = Fire
    check_button(buttons, prev_buttons, SCE_CTRL_LTRIGGER, KEY_STRAFE_L);    // L = Strafe modifier

    check_button(buttons, prev_buttons, SCE_CTRL_START,    KEY_ENTER);       // Start = Enter
    check_button(buttons, prev_buttons, SCE_CTRL_SELECT,   KEY_ESCAPE);      // Select = Escape

    prev_buttons = buttons;
}

void DG_SleepMs(uint32_t ms) {
    sceKernelDelayThread(ms * 1000);
}

uint32_t DG_GetTicksMs(void) {
    uint64_t now = sceKernelGetProcessTimeWide();
    return (uint32_t)((now - start_time_us) / 1000);
}

int DG_GetKey(int *pressed, unsigned char *doom_key) {
    if (key_queue_tail == key_queue_head)
        return 0;

    *pressed = key_queue[key_queue_tail].pressed;
    *doom_key = (unsigned char)key_queue[key_queue_tail].key;
    key_queue_tail = (key_queue_tail + 1) % KEY_QUEUE_SIZE;
    return 1;
}

void DG_SetWindowTitle(const char *title) {
    (void)title;
    // No-op on Vita
}

// Override main - set up arguments to point to the WAD file
int main(int argc, char **argv) {
    // Create data directory if needed
    sceIoMkdir("ux0:data/chexquest2", 0777);

    // Set up doom arguments
    // doomgeneric expects: program_name -iwad <path>
    static char *vita_argv[] = {
        "ChexQuest2Vita",
        "-iwad",
        "ux0:data/chexquest2/chex2.wad",
        NULL
    };
    int vita_argc = 3;

    // Check if WAD exists; if not, also try chex.wad and other common names
    SceIoStat stat;
    if (sceIoGetstat("ux0:data/chexquest2/chex2.wad", &stat) < 0) {
        // Try alternate names
        if (sceIoGetstat("ux0:data/chexquest2/chex.wad", &stat) >= 0) {
            vita_argv[2] = "ux0:data/chexquest2/chex.wad";
        } else if (sceIoGetstat("ux0:data/chexquest2/doom.wad", &stat) >= 0) {
            vita_argv[2] = "ux0:data/chexquest2/doom.wad";
        } else if (sceIoGetstat("ux0:data/chexquest2/CHEX2.WAD", &stat) >= 0) {
            vita_argv[2] = "ux0:data/chexquest2/CHEX2.WAD";
        } else if (sceIoGetstat("ux0:data/chexquest2/CHEX.WAD", &stat) >= 0) {
            vita_argv[2] = "ux0:data/chexquest2/CHEX.WAD";
        }
        // If none found, doomgeneric will show its own error
    }

    // Call doomgeneric main
    doomgeneric_Create(vita_argc, vita_argv);

    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}
