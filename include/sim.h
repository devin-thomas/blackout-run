#ifndef BLACKOUT_RUN_SIM_H
#define BLACKOUT_RUN_SIM_H

#include <stdint.h>

#define BR_MAP_W 30
#define BR_MAP_H 20
#define BR_HEAT_CELLS (BR_MAP_W * BR_MAP_H)
#define BR_ROUTE_BYTES ((BR_HEAT_CELLS + 7) / 8)
#define BR_JOBS_PER_SHIFT 2
#define BR_SHIFT_COUNT 3

enum BrScreen {
    BR_TITLE,
    BR_BRIEF,
    BR_PLAY,
    BR_PAUSE,
    BR_MEMORY,
    BR_ENDING,
    BR_HELP
};

enum BrInput {
    BR_KEY_A = 1u << 0,
    BR_KEY_B = 1u << 1,
    BR_KEY_SELECT = 1u << 2,
    BR_KEY_START = 1u << 3,
    BR_KEY_RIGHT = 1u << 4,
    BR_KEY_LEFT = 1u << 5,
    BR_KEY_UP = 1u << 6,
    BR_KEY_DOWN = 1u << 7
};

typedef struct {
    uint8_t source_x;
    uint8_t source_y;
    uint8_t destination_x;
    uint8_t destination_y;
    const char *source_name;
    const char *destination_name;
} BrJob;

typedef struct {
    uint32_t signature;
    uint8_t screen;
    uint8_t shift;
    uint8_t job_in_shift;
    uint8_t carrying;
    uint8_t delivered;
    uint8_t missed;
    uint8_t free_ride;
    uint8_t menu_index;
    uint8_t memory_return;
    uint8_t facing;
    uint8_t bell_frames;
    uint8_t brake_frames;
    uint8_t route_points;
    uint8_t copied_percent;
    uint16_t player_x;
    uint16_t player_y;
    int16_t money;
    uint16_t timer_frames;
    uint16_t frame;
    uint16_t congestion_events;
    uint8_t heat[BR_HEAT_CELLS];
    uint8_t route[BR_ROUTE_BYTES];
    uint8_t last_route[BR_ROUTE_BYTES];
} BrSim;

extern const BrJob br_jobs[BR_SHIFT_COUNT * BR_JOBS_PER_SHIFT];

void br_sim_init(BrSim *sim);
void br_sim_start_story(BrSim *sim);
void br_sim_start_free_ride(BrSim *sim);
void br_sim_update(BrSim *sim, uint16_t held, uint16_t pressed);
int br_sim_is_road(int x, int y);
uint8_t br_sim_heat_at(const BrSim *sim, int x, int y);
const BrJob *br_sim_current_job(const BrSim *sim);

#endif
