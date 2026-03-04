// state machine logic for lab 3
//
// two functions: GetNextState and GetStateOutput
// both are pure -- they take the current state (and input for GetNextState)
// and return something new without touching any globals. this is what makes
// it a "real" state machine and not just a pile of if statements.
//
// the state has a bunch of stuff packed in:
//   - the actual time (hour, minute)
//   - what mode were in (normal, hour-set, minute-set, brightness-set)
//   - button tracking (was it down, how long, did a long press already fire)
//   - flash counter (blink the indicator LED at 1Hz in setting modes)
//   - clock counter (tick up to 1 second then move the minute hand)
//   - PWM counter (cycles fast to do brightness without hardware PWM)

#include "state_machine_logic.h"
#include "hw_interface.h"
#include <ti/devices/msp/msp.h>


state_t GetNextState(state_t s, uint32_t input)
{
    uint8_t btn_down    = (uint8_t)(input & 1U);  // 1 if button is pressed right now
    uint8_t short_press = 0;  // will be set to 1 if we detect a valid short press
    uint8_t long_press  = 0;  // will be set to 1 if we detect a long press

    // ---- figure out what the button is doing ----
    //
    // the basic idea: count how long the button has been held.
    // if it's held for >= 1 second, thats a long press (fires once immediately).
    // if it gets released before that but after >=7ms, thats a short press (fires on release).
    // if released faster than 7ms its probably just bounce, we ignore it.

    if (btn_down) {
        if (!s.btn_was_down) {
            // button just went from not-pressed to pressed
            // reset the counter so we start timing this press fresh
            s.btn_hold_ticks  = 0;
            s.long_press_done = 0;
        }

        // count up how long we've been holding (cap at max so it doesnt wrap around)
        if (s.btn_hold_ticks < 0xFFFFU) {
            s.btn_hold_ticks++;
        }

        // has it been held for 1 full second? fire the long press exactly once.
        // long_press_done makes sure we dont keep firing every tick after that
        if (!s.long_press_done && s.btn_hold_ticks >= LONG_PRESS_TICKS) {
            long_press        = 1;
            s.long_press_done = 1;  // dont fire again until next press
        }

    } else {
        // button just got released (or was already up)
        if (s.btn_was_down && !s.long_press_done
                && s.btn_hold_ticks >= DEBOUNCE_MIN_TICKS) {
            // it was pressed, no long press happened, and it lasted long enough
            // to not be bounce -- thats a legit short press!
            // action fires on release intentionally: this way if you long-press
            // to change modes, the release doesnt ALSO trigger a short press action
            short_press = 1;
        }
        // reset button state for next time
        s.btn_hold_ticks  = 0;
        s.long_press_done = 0;
    }
    s.btn_was_down = btn_down;  // remember for next tick


    // ---- mode transitions on long press ----
    //
    // long press cycles forward through modes:
    //   Normal -> Hour-Set -> Minute-Set -> Brightness-Set -> Normal -> ...
    // entering a setting mode also freezes the clock (clock_ticks stops incrementing)

    if (long_press) {
        switch (s.mode) {
            case MODE_NORMAL:
                s.mode        = MODE_HOUR_SET;
                s.flash_on    = 1;      // start with the indicator visible
                s.flash_ticks = 0;
                s.clock_ticks = 0;      // freeze -- clock shouldnt tick while setting
                break;

            case MODE_HOUR_SET:
                s.mode        = MODE_MINUTE_SET;
                s.flash_on    = 1;
                s.flash_ticks = 0;
                break;

            case MODE_MINUTE_SET:
                s.mode        = MODE_BRIGHTNESS_SET;
                s.flash_on    = 1;
                s.flash_ticks = 0;
                break;

            case MODE_BRIGHTNESS_SET:
                // done setting everything, go back to normal
                s.mode        = MODE_NORMAL;
                s.flash_on    = 0;
                s.flash_ticks = 0;
                s.clock_ticks = 0;      // restart counting from zero with the new time
                break;

            default:
                // somehow ended up in an unknown mode, just go back to normal
                s.mode = MODE_NORMAL;
                break;
        }
    }


    // ---- short press actions ----
    //
    // each mode has a different thing that advances:
    //   hour-set:       hour goes up by 1 (wraps 11->0, thats 12->1 on the clock face)
    //   minute-set:     minute goes up by 1 (wraps 11->0, thats 55min->0)
    //   brightness-set: brightness level goes up by 1 (wraps back to 0 at max)
    //   normal mode:    short press does nothing

    if (short_press) {
        switch (s.mode) {
            case MODE_HOUR_SET:
                s.hour = (uint8_t)((s.hour + 1U) % 12U);
                break;
            case MODE_MINUTE_SET:
                s.minute = (uint8_t)((s.minute + 1U) % 12U);
                break;
            case MODE_BRIGHTNESS_SET:
                s.brightness = (uint8_t)((s.brightness + 1U) % BRIGHTNESS_LEVELS);
                break;
            default:
                break;  // normal mode, do nothing
        }
    }


    // ---- blink the indicator in setting modes ----
    //
    // flash_ticks counts up, when it hits FLASH_HALF_PERIOD (500ms) we flip flash_on
    // so the LED blinks at 1 Hz (500ms on, 500ms off)
    // only runs when were in a setting mode

    if (s.mode != MODE_NORMAL) {
        s.flash_ticks++;
        if (s.flash_ticks >= FLASH_HALF_PERIOD) {
            s.flash_ticks = 0;
            s.flash_on    = !s.flash_on;  // toggle
        }
    }


    // ---- advance the clock in normal mode ----
    //
    // clock_ticks counts up, when it hits TICKS_PER_SECOND (1 second) we move
    // the minute hand forward one LED position. when minute wraps past 11, the
    // hour hand also moves forward.
    //
    // NOTE: right now this advances every 1 second which is fast. change
    // CLOCK_ADVANCE_TICKS to (5*60*TICKS_PER_SECOND) for real 5-minute steps.
    // the fast mode is just way easier to test with.

    if (s.mode == MODE_NORMAL) {
        s.clock_ticks++;
        if (s.clock_ticks >= CLOCK_ADVANCE_TICKS) {
            s.clock_ticks = 0;
            s.minute = (uint8_t)((s.minute + 1U) % 12U);
            if (s.minute == 0U) {
                // minute wrapped back to 0, so an hour just passed
                s.hour = (uint8_t)((s.hour + 1U) % 12U);
            }
        }
    }


    // ---- PWM counter ----
    // just cycles 0,1,2,...,15,0,1,2,... every tick
    // used in GetStateOutput to decide whether the LED is in its "on" phase
    s.pwm_tick = (uint8_t)((s.pwm_tick + 1U) % PWM_PERIOD_TICKS);

    return s;
}


// GetStateOutput -- given the current state, what should the LEDs look like?
//
// returns a bitmask for GPIOA->DOUT31_0
// LEDs are active-low so: bit=0 means LED ON, bit=1 means LED OFF
// the led_mask global has 1s for every LED pin, so we start with all off
// and then clear individual bits to turn specific LEDs on

int GetStateOutput(state_t current_state)
{
    int output = led_mask;  // start with every LED off (all bits = 1)

    // figure out if hour and minute should currently be visible
    // in normal mode both are always on
    // in setting modes the one being set blinks, the other stays steady
    uint8_t show_hour   = 1;
    uint8_t show_minute = 1;

    switch (current_state.mode) {
        case MODE_NORMAL:
            // both on, no drama
            break;

        case MODE_HOUR_SET:
            // hour blinks to show thats whats being changed
            // minute stays on as a reference so you know what time it is
            show_hour = current_state.flash_on;
            break;

        case MODE_MINUTE_SET:
            // minute blinks, hour stays steady
            show_minute = current_state.flash_on;
            break;

        case MODE_BRIGHTNESS_SET:
            // both blink together so you can see them at whatever brightness you just set
            show_hour   = current_state.flash_on;
            show_minute = current_state.flash_on;
            break;

        default:
            break;
    }

    // apply brightness via PWM
    // the LED is only turned on when pwm_tick is in the "on" window
    // window size = brightness+1 out of PWM_PERIOD_TICKS (16)
    // so brightness=0 gives 1/16 duty (~6%), brightness=14 gives 15/16 (~94%)
    if (current_state.pwm_tick < (uint8_t)(current_state.brightness + 1U)) {
        if (show_hour) {
            output &= ~(1 << hour_leds[current_state.hour].pin_number);
        }
        if (show_minute) {
            output &= ~(1 << minute_leds[current_state.minute].pin_number);
        }
    }

    return output;
}
