#include <assert.h>
#include <stdio.h>

#include "sim.h"

static void press(BrSim *sim, uint16_t key)
{
    br_sim_update(sim, key, key);
    br_sim_update(sim, 0, 0);
}

static void deliver_current_job(BrSim *sim)
{
    const BrJob *job = br_sim_current_job(sim);
    assert(job != 0);
    sim->player_x = (uint16_t)(job->source_x * 8);
    sim->player_y = (uint16_t)(job->source_y * 8);
    br_sim_update(sim, 0, 0);
    assert(sim->carrying == 1);

    for (int i = 0; i < 12; ++i) br_sim_update(sim, BR_KEY_RIGHT, 0);
    job = br_sim_current_job(sim);
    sim->player_x = (uint16_t)(job->destination_x * 8);
    sim->player_y = (uint16_t)(job->destination_y * 8);
    br_sim_update(sim, 0, 0);
}

static void test_story_reaches_ending(void)
{
    BrSim sim;
    br_sim_init(&sim);
    assert(sim.screen == BR_TITLE);
    press(&sim, BR_KEY_A);
    assert(sim.screen == BR_BRIEF);

    for (int shift = 0; shift < BR_SHIFT_COUNT; ++shift) {
        press(&sim, BR_KEY_A);
        assert(sim.screen == BR_PLAY);
        deliver_current_job(&sim);
        assert(sim.screen == BR_PLAY);
        deliver_current_job(&sim);
        assert(sim.screen == BR_MEMORY);
        assert(sim.delivered == (shift + 1) * BR_JOBS_PER_SHIFT);
        press(&sim, BR_KEY_A);
        assert(sim.screen == (shift + 1 == BR_SHIFT_COUNT ? BR_ENDING : BR_BRIEF));
    }
    assert(sim.money > 0);
    assert(sim.copied_percent > 0);
}

static void test_route_heat_and_bell(void)
{
    BrSim sim;
    br_sim_init(&sim);
    br_sim_start_story(&sim);
    press(&sim, BR_KEY_A);
    br_sim_update(&sim, 0, 0);
    assert(sim.carrying == 1);
    for (int i = 0; i < 18; ++i) br_sim_update(&sim, BR_KEY_RIGHT, 0);
    {
        const BrJob *job = br_sim_current_job(&sim);
        sim.player_x = (uint16_t)(job->destination_x * 8);
        sim.player_y = (uint16_t)(job->destination_y * 8);
    }
    br_sim_update(&sim, 0, 0);
    assert(sim.delivered == 1);
    assert(sim.copied_percent >= 29);
    press(&sim, BR_KEY_A);
    assert(sim.bell_frames > 0);
}

static void test_timeout_and_free_ride(void)
{
    BrSim sim;
    br_sim_init(&sim);
    br_sim_start_story(&sim);
    press(&sim, BR_KEY_A);
    sim.timer_frames = 1;
    br_sim_update(&sim, 0, 0);
    assert(sim.missed == 1);
    assert(sim.job_in_shift == 1);

    br_sim_start_free_ride(&sim);
    press(&sim, BR_KEY_A);
    assert(sim.screen == BR_PLAY);
    assert(sim.free_ride == 1);
    assert(sim.timer_frames > 0);
}

int main(void)
{
    test_story_reaches_ending();
    test_route_heat_and_bell();
    test_timeout_and_free_ride();
    puts("simulation tests: PASS");
    return 0;
}
