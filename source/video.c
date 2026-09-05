#include <gba.h>

#include <stdio.h>
#include <string.h>

#include "video.h"

#include "bell_ring.h"
#include "cars.h"
#include "city_tiles.h"
#include "courier.h"
#include "crowd_clusters.h"
#include "cyclists_4dir.h"
#include "font.h"
#include "package_marker.h"
#include "palette_variants.h"
#include "pedestrians_4dir.h"
#include "route_dots.h"
#include "wordmark.h"

#define BR_BG_MAP ((volatile uint16_t *)0x0600F800)
#define BR_BG_TILES ((volatile uint8_t *)0x06000000)
#define BR_OBJ_TILES ((volatile uint8_t *)0x06010000)
#define BR_BG_PALETTE ((volatile uint16_t *)0x05000000)
#define BR_OBJ_PALETTE ((volatile uint16_t *)0x05000200)
#define BR_OAM ((volatile uint16_t *)0x07000000)

#define BR_FONT_BASE 128
#define BR_WORDMARK_BASE 176
#define BR_OBJ_COURIER 0
#define BR_OBJ_PEOPLE 64
#define BR_OBJ_CARS 96
#define BR_OBJ_CYCLISTS 128
#define BR_OBJ_CROWD 160
#define BR_OBJ_PACKAGE 192
#define BR_OBJ_BELL 200
#define BR_OBJ_ROUTE 204

static uint16_t s_map[32 * 32] __attribute__((aligned(4)));
static uint16_t s_oam[128 * 4] __attribute__((aligned(4)));
static int s_scene = -1;

static const char s_glyphs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$%:!-></ ";

static uint16_t br_entry(int tile, int palette)
{
    return (uint16_t)(tile | (palette << 12));
}

static void br_put_tile(int x, int y, int tile, int palette)
{
    if (x >= 0 && x < 30 && y >= 0 && y < 20) {
        s_map[y * 32 + x] = br_entry(tile, palette);
    }
}

static void br_fill(int tile)
{
    for (int i = 0; i < 32 * 32; ++i) s_map[i] = br_entry(tile, 0);
}

static int br_glyph(char ch)
{
    const char *found = strchr(s_glyphs, ch);
    return found == 0 ? 47 : (int)(found - s_glyphs);
}

static void br_text(int x, int y, const char *text)
{
    while (*text != '\0' && x < 30) {
        if (*text != ' ') br_put_tile(x, y, BR_FONT_BASE + br_glyph(*text), 1);
        x++;
        text++;
    }
}

static void br_center(int y, const char *text)
{
    int length = (int)strlen(text);
    br_text((30 - length) / 2, y, text);
}

static void br_number(int x, int y, int value, int width)
{
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%*d", width, value);
    br_text(x, y, buffer);
}

static void br_copy_tile(int destination, const unsigned int *source, int source_tile)
{
    const unsigned int *src = source + source_tile * 8;
    volatile unsigned int *dst =
        (volatile unsigned int *)(BR_OBJ_TILES + destination * 32);
    for (int i = 0; i < 8; ++i) dst[i] = src[i];
}

static void br_copy_frames_8(int destination, const unsigned int *source, int count)
{
    for (int frame = 0; frame < count; ++frame) {
        br_copy_tile(destination + frame, source, frame);
    }
}

static void br_copy_frames_16(int destination, const unsigned int *source,
                              int sheet_tiles_wide, int columns, int rows)
{
    int frame = 0;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            int origin = row * 2 * sheet_tiles_wide + column * 2;
            br_copy_tile(destination + frame * 4 + 0, source, origin);
            br_copy_tile(destination + frame * 4 + 1, source, origin + 1);
            br_copy_tile(destination + frame * 4 + 2, source,
                         origin + sheet_tiles_wide);
            br_copy_tile(destination + frame * 4 + 3, source,
                         origin + sheet_tiles_wide + 1);
            frame++;
        }
    }
}

static void br_load_assets(void)
{
    memcpy((void *)BR_BG_TILES, city_tilesTiles, city_tilesTilesLen);
    memcpy((void *)(BR_BG_TILES + BR_FONT_BASE * 32), fontTiles, fontTilesLen);
    memcpy((void *)(BR_BG_TILES + BR_WORDMARK_BASE * 32), wordmarkTiles,
           wordmarkTilesLen);

    br_copy_frames_16(BR_OBJ_COURIER, courierTiles, 8, 4, 4);
    br_copy_frames_8(BR_OBJ_PEOPLE, pedestrians_4dirTiles, 32);
    br_copy_frames_16(BR_OBJ_CARS, carsTiles, 8, 4, 2);
    br_copy_frames_16(BR_OBJ_CYCLISTS, cyclists_4dirTiles, 8, 4, 2);
    br_copy_frames_16(BR_OBJ_CROWD, crowd_clustersTiles, 8, 4, 2);
    br_copy_frames_16(BR_OBJ_PACKAGE, package_markerTiles, 4, 2, 1);
    br_copy_frames_8(BR_OBJ_BELL, bell_ringTiles, 4);
    br_copy_frames_8(BR_OBJ_ROUTE, route_dotsTiles, 4);
}

static void br_apply_scene(int scene)
{
    if (scene == s_scene) return;
    s_scene = scene;
    for (int i = 0; i < 16; ++i) {
        BR_BG_PALETTE[i] =
            blackout_run_scene_palettes[scene][BLACKOUT_RUN_PALETTE_CITY][i];
        BR_BG_PALETTE[16 + i] =
            blackout_run_scene_palettes[scene][BLACKOUT_RUN_PALETTE_HUD][i];
        BR_OBJ_PALETTE[i] =
            blackout_run_scene_palettes[scene][BLACKOUT_RUN_PALETTE_COURIER][i];
        BR_OBJ_PALETTE[16 + i] =
            blackout_run_scene_palettes[scene][BLACKOUT_RUN_PALETTE_PEOPLE][i];
        BR_OBJ_PALETTE[32 + i] =
            blackout_run_scene_palettes[scene][BLACKOUT_RUN_PALETTE_TRAFFIC][i];
        BR_OBJ_PALETTE[48 + i] =
            blackout_run_scene_palettes[scene][BLACKOUT_RUN_PALETTE_HUD][i];
    }
}

static int br_scene_for(const BrSim *sim)
{
    if (sim->screen == BR_TITLE || sim->screen == BR_ENDING) {
        return BLACKOUT_RUN_SCENE_NIGHT;
    }
    if (sim->shift == 0) return BLACKOUT_RUN_SCENE_DAY;
    if (sim->shift == 1) return BLACKOUT_RUN_SCENE_DUSK;
    return BLACKOUT_RUN_SCENE_BLACKOUT;
}

static void br_hide_oam(void)
{
    for (int i = 0; i < 128; ++i) {
        s_oam[i * 4] = 160;
        s_oam[i * 4 + 1] = 0;
        s_oam[i * 4 + 2] = 0;
        s_oam[i * 4 + 3] = 0;
    }
}

static void br_sprite(int index, int x, int y, int tile, int palette, int size16)
{
    if (index < 0 || index >= 128) return;
    s_oam[index * 4] = (uint16_t)(y & 0xFF);
    s_oam[index * 4 + 1] = (uint16_t)((x & 0x1FF) | (size16 ? (1 << 14) : 0));
    s_oam[index * 4 + 2] = (uint16_t)(tile | (palette << 12));
}

static int br_is_road_tile(int x, int y)
{
    return (x >= 4 && x <= 6) || (x >= 13 && x <= 15) ||
           (x >= 22 && x <= 24) || (y >= 5 && y <= 7) ||
           (y >= 11 && y <= 13) || (y >= 16 && y <= 18);
}

static void br_city_base(void)
{
    br_fill(80);
    for (int y = 2; y < 20; ++y) {
        for (int x = 0; x < 30; ++x) {
            int tile;
            if (br_is_road_tile(x, y)) {
                tile = ((x == 5 || x == 14 || x == 23) ? 17 : 0);
                if (y == 6 || y == 12 || y == 17) tile = 19;
                if (((x >= 4 && x <= 6) || (x >= 13 && x <= 15) ||
                     (x >= 22 && x <= 24)) &&
                    ((y >= 5 && y <= 7) || (y >= 11 && y <= 13) ||
                     (y >= 16 && y <= 18))) tile = 2;
            } else if (y < 5) {
                tile = 64 + ((x + y) & 3);
            } else if (y < 11) {
                tile = 40 + ((x + y) & 3);
            } else if (y < 16) {
                tile = 72 + ((x + y) & 3);
            } else {
                tile = 32 + ((x + y) & 3);
            }
            br_put_tile(x, y, tile, 0);
        }
    }
    for (int x = 0; x < 30; ++x) {
        br_put_tile(x, 0, 80, 0);
        br_put_tile(x, 1, 13, 0);
    }
}

static void br_render_title(const BrSim *sim)
{
    br_city_base();
    for (int y = 3; y <= 17; ++y) {
        for (int x = 2; x <= 27; ++x) br_put_tile(x, y, 120, 0);
        br_put_tile(2, y, 121, 0);
        br_put_tile(27, y, 121, 0);
    }
    for (int x = 2; x <= 27; ++x) {
        br_put_tile(x, 3, 121, 0);
        br_put_tile(x, 17, 121, 0);
    }
    for (int i = 0; i < 12; ++i) br_put_tile(9 + i, 5, BR_WORDMARK_BASE + i, 1);
    br_center(7, "THE CITY COPIES YOU");
    br_center(10, sim->menu_index == 0 ? "> STORY SHIFT" : "  STORY SHIFT");
    br_center(12, sim->menu_index == 1 ? "> FREE RIDE" : "  FREE RIDE");
    br_center(14, sim->menu_index == 2 ? "> HOW TO PLAY" : "  HOW TO PLAY");
    br_center(18, "A OR START");
}

static void br_render_help(void)
{
    br_fill(120);
    br_center(2, "DISPATCH HANDBOOK");
    br_text(3, 5, "D PAD  RIDE AND STEER");
    br_text(3, 7, "A      RING THE BELL");
    br_text(3, 9, "B      BRAKE HARD");
    br_text(3, 11, "START  PAUSE");
    br_text(3, 13, "SELECT ROUTE MEMORY");
    br_center(16, "CROWDS COPY FAST ROUTES");
    br_center(18, "A TO RETURN");
}

static void br_render_brief(const BrSim *sim)
{
    static const char *titles[BR_SHIFT_COUNT] = {
        "SHIFT 1  MORNING LINES",
        "SHIFT 2  RUSH HOUR",
        "SHIFT 3  BLACKOUT RUN"
    };
    static const char *notes[BR_SHIFT_COUNT][3] = {
        { "TEACH THE CITY A ROUTE", "TWO PARCELS BEFORE CLOSE", "WATCH WHAT FOLLOWS" },
        { "YESTERDAYS LINE IS BUSY", "BELL SPLITS THE CROWD", "FIND ANOTHER WAY" },
        { "SIGNALS ARE DOWN", "THE CITY KNOWS YOUR MOVES", "GET POWER HOME" }
    };

    br_fill(120);
    br_center(2, sim->free_ride ? "FREE RIDE" : titles[sim->shift]);
    br_center(6, notes[sim->shift][0]);
    br_center(8, notes[sim->shift][1]);
    br_center(10, notes[sim->shift][2]);
    br_center(14, "D PAD RIDE  A BELL  B BRAKE");
    br_center(17, "PRESS A TO ROLL");
}

static void br_render_hud(const BrSim *sim)
{
    const BrJob *job = br_sim_current_job(sim);
    int seconds = sim->free_ride ? 99 : sim->timer_frames / 60;

    br_text(0, 0, "TIME");
    br_number(4, 0, seconds, 2);
    br_text(8, 0, "JOB");
    br_number(11, 0, sim->job_in_shift + 1, 1);
    br_text(14, 0, "PAY$");
    br_number(18, 0, sim->money, 3);
    br_text(23, 0, sim->bell_frames ? "RING!" : "A BELL");
    if (job != 0) {
        br_text(0, 1, sim->carrying ? "TO " : "PICKUP ");
        br_text(sim->carrying ? 3 : 7, 1,
                sim->carrying ? job->destination_name : job->source_name);
    }
}

static int br_render_heat_sprites(const BrSim *sim, int sprite)
{
    int shown = 0;
    int phase = (sim->frame >> 4) & 1;
    for (int y = 2; y < 20 && shown < 18; ++y) {
        for (int x = 0; x < 30 && shown < 18; ++x) {
            uint8_t heat = sim->heat[y * BR_MAP_W + x];
            if (heat == 0 || ((x + y) & 1) != 0) continue;
            if (heat >= 4) {
                int frame = ((x + y) & 3) + phase * 4;
                br_sprite(sprite++, x * 8 - 4, y * 8 - 4,
                          BR_OBJ_CROWD + frame * 4, 1, 1);
            } else {
                int frame = ((x + y) & 3) * 8 + phase;
                br_sprite(sprite++, x * 8, y * 8,
                          BR_OBJ_PEOPLE + frame, 1, 0);
            }
            shown++;
        }
    }
    return sprite;
}

static void br_render_play(const BrSim *sim)
{
    const BrJob *job = br_sim_current_job(sim);
    int sprite = 0;
    int action = sim->bell_frames ? 2 : (sim->brake_frames ? 3 : ((sim->frame >> 3) & 1));
    int courier_frame = sim->facing * 4 + action;
    int target_x;
    int target_y;

    br_city_base();
    br_render_hud(sim);

    if (job != 0) {
        target_x = (sim->carrying ? job->destination_x : job->source_x) * 8;
        target_y = (sim->carrying ? job->destination_y : job->source_y) * 8;
        br_sprite(sprite++, target_x - 8, target_y - 8,
                  BR_OBJ_PACKAGE + ((sim->frame >> 4) & 1) * 4, 3, 1);
    }

    sprite = br_render_heat_sprites(sim, sprite);

    for (int i = 0; i < 4; ++i) {
        int horizontal = i & 1;
        int frame = horizontal ? 2 : 0;
        int x = horizontal ? (sim->frame * (i + 1) / 2 + i * 47) % 256 - 16
                           : (i ? 176 : 32);
        int y = horizontal ? (i == 1 ? 88 : 128)
                           : (sim->frame * (i + 1) / 3 + i * 61) % 176 - 16;
        br_sprite(sprite++, x, y, BR_OBJ_CARS + frame * 4, 2, 1);
    }

    if (sim->delivered > 0) {
        int x = (sim->frame + 72) % 248 - 8;
        br_sprite(sprite++, x, 120,
                  BR_OBJ_CYCLISTS + (2 + ((sim->frame >> 4) & 1) * 4) * 4,
                  0, 1);
    }

    br_sprite(sprite++, sim->player_x - 8, sim->player_y - 8,
              BR_OBJ_COURIER + courier_frame * 4, 0, 1);
    if (sim->bell_frames > 0) {
        br_sprite(sprite++, sim->player_x + 5, sim->player_y - 4,
                  BR_OBJ_BELL + ((24 - sim->bell_frames) >> 3), 3, 0);
    }
}

static void br_render_pause(void)
{
    br_fill(120);
    br_center(5, "SHIFT PAUSED");
    br_center(9, "START OR B  KEEP RIDING");
    br_center(12, "SELECT  END TO TITLE");
}

static int br_route_bit(const uint8_t *bits, int index)
{
    return (bits[index >> 3] >> (index & 7)) & 1;
}

static void br_render_memory(const BrSim *sim)
{
    br_fill(120);
    br_text(1, 1, "ROUTE MEMORY");
    br_text(20, 1, "COPIED");
    br_number(27, 1, sim->copied_percent, 2);
    br_text(29, 1, "%");
    for (int my = 0; my < 9; ++my) {
        for (int mx = 0; mx < 15; ++mx) {
            int heat = 0;
            int route = 0;
            for (int oy = 0; oy < 2; ++oy) {
                for (int ox = 0; ox < 2; ++ox) {
                    int index = (my * 2 + oy + 2) * BR_MAP_W + mx * 2 + ox;
                    if (index >= BR_HEAT_CELLS) continue;
                    if (sim->heat[index] > heat) heat = sim->heat[index];
                    route |= br_route_bit(sim->last_route, index);
                }
            }
            br_put_tile(7 + mx, 4 + my,
                        route ? 121 : (heat >= 4 ? 127 : (heat ? 42 : 120)), 0);
        }
    }
    br_center(14, "YOUR LINE BECAME THEIR LINE");
    br_text(4, 16, "DELIVERED");
    br_number(14, 16, sim->delivered, 2);
    br_text(18, 16, "MISSED");
    br_number(25, 16, sim->missed, 2);
    br_center(18, sim->memory_return ? "SELECT TO RIDE" : "A FOR NEXT SHIFT");
}

static void br_render_ending(const BrSim *sim)
{
    br_fill(120);
    for (int i = 0; i < 12; ++i) br_put_tile(9 + i, 3, BR_WORDMARK_BASE + i, 1);
    br_center(6, sim->delivered >= 5 ? "POWER HOME BEFORE DAWN" : "DAWN FOUND A NEW ROUTE");
    br_center(9, "THE CITY REMEMBERS");
    br_center(11, "EVERY SHORTCUT YOU TAUGHT IT");
    br_text(7, 14, "JOBS");
    br_number(12, 14, sim->delivered, 2);
    br_text(16, 14, "PAY$");
    br_number(20, 14, sim->money, 3);
    br_center(18, "A TO RETURN TO DISPATCH");
}

void br_video_init(void)
{
    REG_DISPCNT = 0x0080;
    memset((void *)BR_BG_TILES, 0, 0x10000);
    memset((void *)BR_OBJ_TILES, 0, 0x8000);
    memset((void *)BR_BG_MAP, 0, 0x800);
    memset((void *)BR_OAM, 0, 0x400);
    br_load_assets();
    br_fill(80);
    br_hide_oam();
    dmaCopy(s_map, (void *)BR_BG_MAP, sizeof(s_map));
    dmaCopy(s_oam, (void *)BR_OAM, sizeof(s_oam));
    REG_BG0CNT = 2 | (31 << 8);
    REG_DISPCNT = MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP;
}

void br_video_render(const BrSim *sim)
{
    br_apply_scene(br_scene_for(sim));
    br_hide_oam();
    switch (sim->screen) {
    case BR_TITLE: br_render_title(sim); break;
    case BR_HELP: br_render_help(); break;
    case BR_BRIEF: br_render_brief(sim); break;
    case BR_PLAY: br_render_play(sim); break;
    case BR_PAUSE: br_render_pause(); break;
    case BR_MEMORY: br_render_memory(sim); break;
    case BR_ENDING: br_render_ending(sim); break;
    default: br_render_title(sim); break;
    }
    dmaCopy(s_map, (void *)BR_BG_MAP, sizeof(s_map));
    dmaCopy(s_oam, (void *)BR_OAM, sizeof(s_oam));
}
