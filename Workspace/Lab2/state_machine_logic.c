#include "state_machine_logic.h"
#include <ti/devices/msp/msp.h>

/* ================= LED bitmasks ================= */

// Outer LEDs – hours
static const uint32_t hour_leds[12] = {
    0x00000001, // PA0
    0x04000000, // PA26
    0x01000000, // PA24
    0x00400000, // PA22
    0x00040000, // PA18
    0x00010000, // PA16
    0x00004000, // PA14
    0x00001000, // PA12
    0x00000400, // PA10
    0x00000100, // PA8
    0x00000040, // PA6
    0x10000000  // PA28
};

// Inner LEDs – minutes
static const uint32_t minute_leds[12] = {
    0x08000000, // PA27
    0x02000000, // PA25
    0x00800000, // PA23
    0x00200000, // PA21
    0x00020000, // PA17
    0x00008000, // PA15
    0x00002000, // PA13
    0x00000800, // PA11
    0x00000200, // PA9
    0x00000080, // PA7
    0x00000020, // PA5
    0x00000002  // PA1
};

/* ================= State ================= */
typedef struct {
    uint8_t hour;
    uint8_t minute;
} ClockState;

static ClockState state = {0, 0};

/* ================= PWM ================= */
#define PWM_PERIOD_TICKS 4   // total ticks per PWM cycle
#define PWM_ON_TICKS     1   // ticks LED is ON (25% duty cycle)
static uint8_t pwm_tick = 0;
static uint16_t subsecond_counter = 0;  // counts timer ticks for 1 second

#define PWM_TICKS_PER_SECOND 800  // number of timer interrupts per second

/* ================= State Machine ================= */
int GetNextState(int unused)
{
    (void)unused;

    // Increment PWM tick
    pwm_tick = (pwm_tick + 1) % PWM_PERIOD_TICKS;

    // Count time for clock advancement
    subsecond_counter++;
    if (subsecond_counter >= PWM_TICKS_PER_SECOND) {
        subsecond_counter = 0;

        // Advance minutes and hours
        state.minute++;
        if (state.minute >= 12) {
            state.minute = 0;
            state.hour = (state.hour + 1) % 12;
        }
    }

    return 0; // state is stored internally
}

/* ================= Output Logic ================= */
int GetStateOutputGPIOA(int unused)
{
    (void)unused;

    // All LEDs off initially (active-low)
    uint32_t output =
          0x00000001 | 0x04000000 | 0x01000000 | 0x00400000 |
          0x00040000 | 0x00010000 | 0x00004000 | 0x00001000 |
          0x00000400 | 0x00000100 | 0x00000040 | 0x10000000 |
          0x08000000 | 0x02000000 | 0x00800000 | 0x00200000 |
          0x00020000 | 0x00008000 | 0x00002000 | 0x00000800 |
          0x00000200 | 0x00000080 | 0x00000020 | 0x00000002;

    // Apply PWM: only turn ON LEDs during ON phase
    if (pwm_tick < PWM_ON_TICKS) {
        output &= ~hour_leds[state.hour];
        output &= ~minute_leds[state.minute];
    }

    return output;
}

int GetStateOutputGPIOB(int unused)
{
    (void)unused;
    return 0;
}