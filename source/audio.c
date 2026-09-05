#include <gba.h>

#include "audio.h"

static void br_square(unsigned short pitch, unsigned short volume)
{
    REG_SOUND1CNT_L = 0x0024;
    REG_SOUND1CNT_H = (unsigned short)((2 << 6) | (2 << 8) | (1 << 11) |
                                      (volume << 12));
    REG_SOUND1CNT_X = (unsigned short)(0x8000 | (pitch & 0x07FF));
}

void br_audio_init(void)
{
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = SND1_R_ENABLE | SND1_L_ENABLE |
                     SND4_R_ENABLE | SND4_L_ENABLE | 0x0077;
    REG_SOUNDCNT_H = 0x0002;
}

void br_audio_bell(void)
{
    br_square(1816, 13);
}

void br_audio_delivery(void)
{
    br_square(1940, 15);
}

void br_audio_missed(void)
{
    REG_SOUND4CNT_L = (unsigned short)((3 << 8) | (8 << 12));
    REG_SOUND4CNT_H = 0xC05D;
}
