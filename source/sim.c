#include "sim.h"

#include <string.h>

#define BR_SIGNATURE 0x4252554Eu
#define BR_JOB_SECONDS 38

const BrJob br_jobs[BR_SHIFT_COUNT * BR_JOBS_PER_SHIFT] = {
    { 5, 6, 23, 12, "BAKERY", "STATION" },
    { 14, 17, 5, 12, "MARKET", "CLINIC" },
    { 23, 6, 14, 12, "PRINTWORKS", "MEMORIAL" },
    { 5, 17, 23, 17, "DEPOT", "NIGHT CAFE" },
    { 14, 6, 5, 17, "SIGNAL BOX", "SHELTER" },
    { 23, 12, 14, 17, "HOSPITAL", "POWER YARD" }
};

static int br_abs(int value)
{
    return value < 0 ? -value : value;
}

static int br_near(int x, int y, int target_x, int target_y)
{
    return br_abs(x - target_x) <= 11 && br_abs(y - target_y) <= 11;
}

static int br_route_get(const uint8_t *bits, int index)
{
    return (bits[index >> 3] >> (index & 7)) & 1;
}

static void br_route_set(uint8_t *bits, int index)
{
    bits[index >> 3] |= (uint8_t)(1u << (index & 7));
}

static void br_begin_job(BrSim *sim)
{
    const BrJob *job = br_sim_current_job(sim);

    sim->carrying = 0;
    sim->timer_frames = BR_JOB_SECONDS * 60;
    sim->route_points = 0;
    memset(sim->route, 0, sizeof(sim->route));
    if (job != 0) {
        sim->player_x = (uint16_t)(job->source_x * 8);
        sim->player_y = (uint16_t)(job->source_y * 8);
    }
}

static void br_finish_shift(BrSim *sim)
{
    sim->memory_return = 0;
    sim->screen = BR_MEMORY;
}

static void br_advance_job(BrSim *sim)
{
    sim->job_in_shift++;
    if (sim->job_in_shift >= BR_JOBS_PER_SHIFT) {
        if (sim->free_ride) {
            sim->shift = (uint8_t)((sim->shift + 1) % BR_SHIFT_COUNT);
            sim->job_in_shift = 0;
            br_begin_job(sim);
        } else {
            br_finish_shift(sim);
        }
    } else {
        br_begin_job(sim);
    }
}

static void br_copy_route(BrSim *sim)
{
    int used = 0;

    memcpy(sim->last_route, sim->route, sizeof(sim->last_route));
    for (int i = 0; i < BR_HEAT_CELLS; ++i) {
        if (br_route_get(sim->route, i)) {
            int next = sim->heat[i] + 1 + (sim->shift > 0);
            sim->heat[i] = (uint8_t)(next > 7 ? 7 : next);
            used++;
        }
    }
    sim->copied_percent = (uint8_t)(18 + sim->delivered * 11);
    if (sim->copied_percent > 92) {
        sim->copied_percent = 92;
    }
    if (used == 0) {
        sim->copied_percent = 0;
    }
}

static void br_deliver(BrSim *sim)
{
    int bonus = sim->timer_frames / 60;

    sim->delivered++;
    sim->money = (int16_t)(sim->money + 18 + bonus);
    br_copy_route(sim);
    br_advance_job(sim);
}

void br_sim_init(BrSim *sim)
{
    memset(sim, 0, sizeof(*sim));
    sim->signature = BR_SIGNATURE;
    sim->screen = BR_TITLE;
    sim->player_x = 40;
    sim->player_y = 48;
}

void br_sim_start_story(BrSim *sim)
{
    uint8_t menu = sim->menu_index;
    br_sim_init(sim);
    sim->menu_index = menu;
    sim->screen = BR_BRIEF;
}

void br_sim_start_free_ride(BrSim *sim)
{
    br_sim_init(sim);
    sim->free_ride = 1;
    sim->screen = BR_BRIEF;
}

const BrJob *br_sim_current_job(const BrSim *sim)
{
    int index = sim->shift * BR_JOBS_PER_SHIFT + sim->job_in_shift;

    if (index < 0 || index >= BR_SHIFT_COUNT * BR_JOBS_PER_SHIFT) {
        return 0;
    }
    return &br_jobs[index];
}

int br_sim_is_road(int x, int y)
{
    int near_vertical = br_abs(x - 40) <= 13 || br_abs(x - 112) <= 13 ||
                        br_abs(x - 184) <= 13;
    int near_horizontal = br_abs(y - 48) <= 11 || br_abs(y - 96) <= 11 ||
                          br_abs(y - 136) <= 11;

    return y >= 16 && (near_vertical || near_horizontal);
}

uint8_t br_sim_heat_at(const BrSim *sim, int x, int y)
{
    int tile_x = x >> 3;
    int tile_y = y >> 3;

    if (tile_x < 0 || tile_x >= BR_MAP_W || tile_y < 0 || tile_y >= BR_MAP_H) {
        return 0;
    }
    return sim->heat[tile_y * BR_MAP_W + tile_x];
}

static void br_update_player(BrSim *sim, uint16_t held)
{
    int dx = 0;
    int dy = 0;
    int speed = (held & BR_KEY_B) ? 1 : 2;
    int next_x;
    int next_y;
    uint8_t heat;

    if (held & BR_KEY_LEFT) dx--;
    if (held & BR_KEY_RIGHT) dx++;
    if (held & BR_KEY_UP) dy--;
    if (held & BR_KEY_DOWN) dy++;
    if (dx == 0 && dy == 0) return;
    if (br_abs(dx) >= br_abs(dy) && dx < 0) sim->facing = 1;
    else if (br_abs(dx) >= br_abs(dy) && dx > 0) sim->facing = 2;
    else if (dy < 0) sim->facing = 3;
    else sim->facing = 0;

    next_x = sim->player_x + dx * speed;
    next_y = sim->player_y + dy * speed;
    if (next_x < 8) next_x = 8;
    if (next_x > 231) next_x = 231;
    if (next_y < 24) next_y = 24;
    if (next_y > 151) next_y = 151;

    if (!br_sim_is_road(next_x, next_y) && speed > 1) {
        next_x = sim->player_x + dx;
        next_y = sim->player_y + dy;
    }

    heat = br_sim_heat_at(sim, next_x, next_y);
    if (heat >= 2 && sim->bell_frames == 0 && ((sim->frame + heat) % 3) == 0) {
        sim->congestion_events++;
        return;
    }

    sim->player_x = (uint16_t)next_x;
    sim->player_y = (uint16_t)next_y;
    if (sim->carrying) {
        int index = (next_y >> 3) * BR_MAP_W + (next_x >> 3);
        if (!br_route_get(sim->route, index)) {
            br_route_set(sim->route, index);
            if (sim->route_points < 255) sim->route_points++;
        }
    }
}

static void br_update_job(BrSim *sim)
{
    const BrJob *job = br_sim_current_job(sim);

    if (job == 0) return;
    if (!sim->carrying && br_near(sim->player_x, sim->player_y,
                                  job->source_x * 8, job->source_y * 8)) {
        sim->carrying = 1;
        memset(sim->route, 0, sizeof(sim->route));
    } else if (sim->carrying && br_near(sim->player_x, sim->player_y,
                                        job->destination_x * 8,
                                        job->destination_y * 8)) {
        br_deliver(sim);
    }
}

void br_sim_update(BrSim *sim, uint16_t held, uint16_t pressed)
{
    sim->frame++;
    if (sim->bell_frames > 0) sim->bell_frames--;
    if (sim->brake_frames > 0) sim->brake_frames--;

    switch (sim->screen) {
    case BR_TITLE:
        if ((pressed & BR_KEY_UP) && sim->menu_index > 0) sim->menu_index--;
        if ((pressed & BR_KEY_DOWN) && sim->menu_index < 2) sim->menu_index++;
        if (pressed & (BR_KEY_A | BR_KEY_START)) {
            if (sim->menu_index == 0) br_sim_start_story(sim);
            else if (sim->menu_index == 1) br_sim_start_free_ride(sim);
            else sim->screen = BR_HELP;
        }
        break;
    case BR_HELP:
        if (pressed & (BR_KEY_A | BR_KEY_B | BR_KEY_START)) sim->screen = BR_TITLE;
        break;
    case BR_BRIEF:
        if (pressed & (BR_KEY_A | BR_KEY_START)) {
            br_begin_job(sim);
            sim->screen = BR_PLAY;
        }
        break;
    case BR_PLAY:
        if (pressed & BR_KEY_START) {
            sim->screen = BR_PAUSE;
            break;
        }
        if (pressed & BR_KEY_SELECT) {
            sim->memory_return = 1;
            sim->screen = BR_MEMORY;
            break;
        }
        if (pressed & BR_KEY_A) sim->bell_frames = 24;
        if (held & BR_KEY_B) sim->brake_frames = 2;
        br_update_player(sim, held);
        br_update_job(sim);
        if (sim->screen == BR_PLAY && !sim->free_ride) {
            if (sim->timer_frames > 0) sim->timer_frames--;
            if (sim->timer_frames == 0) {
                sim->missed++;
                sim->money = (int16_t)(sim->money - 5);
                br_advance_job(sim);
            }
        }
        break;
    case BR_PAUSE:
        if (pressed & (BR_KEY_START | BR_KEY_B)) sim->screen = BR_PLAY;
        if (pressed & BR_KEY_SELECT) {
            br_sim_init(sim);
        }
        break;
    case BR_MEMORY:
        if (pressed & (BR_KEY_A | BR_KEY_START | BR_KEY_SELECT)) {
            if (sim->memory_return) {
                sim->memory_return = 0;
                sim->screen = BR_PLAY;
            } else if (sim->shift + 1 >= BR_SHIFT_COUNT) {
                sim->screen = BR_ENDING;
            } else {
                sim->shift++;
                sim->job_in_shift = 0;
                sim->screen = BR_BRIEF;
            }
        }
        break;
    case BR_ENDING:
        if (pressed & (BR_KEY_A | BR_KEY_START)) br_sim_init(sim);
        break;
    default:
        br_sim_init(sim);
        break;
    }
}
