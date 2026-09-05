#include <gba.h>
#include <stddef.h>

#include "sim.h"
#include "video.h"
#include "audio.h"

BrSim g_blackout_run;

const unsigned int g_br_debug_offsets[] __attribute__((used)) = {
    offsetof(BrSim, screen),
    offsetof(BrSim, shift),
    offsetof(BrSim, job_in_shift),
    offsetof(BrSim, carrying),
    offsetof(BrSim, delivered),
    offsetof(BrSim, missed),
    offsetof(BrSim, copied_percent),
    offsetof(BrSim, player_x),
    offsetof(BrSim, player_y),
    offsetof(BrSim, money),
    offsetof(BrSim, timer_frames),
    offsetof(BrSim, heat),
    offsetof(BrSim, last_route),
    BR_ROUTE_BYTES
};

int main(void)
{
    uint16_t previous = 0;

    irqInit();
    irqEnable(IRQ_VBLANK);
    br_sim_init(&g_blackout_run);
    br_video_init();
    br_audio_init();

    while (1) {
        uint16_t held;
        uint16_t pressed;
        uint8_t delivered_before = g_blackout_run.delivered;
        uint8_t missed_before = g_blackout_run.missed;

        VBlankIntrWait();
        scanKeys();
        held = keysHeld();
        pressed = held & (uint16_t)~previous;
        previous = held;
        br_sim_update(&g_blackout_run, held, pressed);
        if (g_blackout_run.bell_frames == 24) br_audio_bell();
        if (g_blackout_run.delivered != delivered_before) br_audio_delivery();
        if (g_blackout_run.missed != missed_before) br_audio_missed();
        br_video_render(&g_blackout_run);
    }
}
