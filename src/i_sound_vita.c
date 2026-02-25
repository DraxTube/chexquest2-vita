/*
 * i_sound_vita.c - Stub sound implementation for PS Vita
 *
 * This replaces i_sound.c from doomgeneric with no-op stubs.
 * Sound is not supported in this port (to keep things simple and working).
 */

#include "doomtype.h"
#include "i_sound.h"
#include "w_wad.h"

#include <string.h>

// --- Music stubs ---

static boolean mus_initialized = false;

boolean I_InitMusic(void) {
    mus_initialized = true;
    return true;
}

void I_ShutdownMusic(void) {
    mus_initialized = false;
}

void I_SetMusicVolume(int volume) {
    (void)volume;
}

void I_PauseSong(void) {
}

void I_ResumeSong(void) {
}

void *I_RegisterSong(void *data, int len) {
    (void)data;
    (void)len;
    return (void *)1; // Return non-NULL to indicate "success"
}

void I_UnRegisterSong(void *handle) {
    (void)handle;
}

void I_PlaySong(void *handle, boolean looping) {
    (void)handle;
    (void)looping;
}

void I_StopSong(void) {
}

boolean I_MusicIsPlaying(void) {
    return false;
}

// --- Sound effect stubs ---

boolean I_InitSound(void) {
    return true;
}

void I_ShutdownSound(void) {
}

int I_GetSfxLumpNum(sfxinfo_t *sfx) {
    char namebuf[16];

    if (!sfx || !sfx->name)
        return -1;

    memset(namebuf, 0, sizeof(namebuf));
    snprintf(namebuf, sizeof(namebuf), "ds%s", sfx->name);

    int lump = W_CheckNumForName(namebuf);
    return lump;
}

void I_UpdateSound(void) {
}

void I_UpdateSoundParams(int channel, int vol, int sep) {
    (void)channel;
    (void)vol;
    (void)sep;
}

int I_StartSound(sfxinfo_t *sfx, int channel, int vol, int sep) {
    (void)sfx;
    (void)channel;
    (void)vol;
    (void)sep;
    return channel;
}

void I_StopSound(int channel) {
    (void)channel;
}

boolean I_SoundIsPlaying(int channel) {
    (void)channel;
    return false;
}
