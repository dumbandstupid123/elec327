
#ifndef state_machine_logic_include
#define state_machine_logic_include

#include <stdint.h>

// the 4 modes. we cycle through them with long presses.
// normal is the only one where the clock actually moves.
#define MODE_NORMAL          0   // just showing the time like a normal clock
#define MODE_HOUR_SET        1   // hour LED blinks, short press = hour++
#define MODE_MINUTE_SET      2   // minute LED blinks, short press = minute++
#define MODE_BRIGHTNESS_SET  3   // both blink, short press = brightness++ (extra credit)

// timing stuff -- timer fires at LFCLK/16 = 32768/16 = 2048 Hz
// so 1 tick is about 0.488 ms. keep this in mind for all the thresholds below.
#define TICKS_PER_SECOND     2048   // 2048 ticks * 0.488ms = 1 second
#define LONG_PRESS_TICKS     2048   // hold for 1 full second to change modes
#define DEBOUNCE_MIN_TICKS     15   // ~7.3ms, spec says >=5ms so were fine
#define FLASH_HALF_PERIOD    1024   // 1024 ticks = 500ms, so LED blinks at 1Hz

// how often to move the minute hand in normal mode
// right now its every 1 second (TICKS_PER_SECOND) which is fast but good for demos
// if you want real 5-minute intervals change this to: (5 * 60 * TICKS_PER_SECOND)
#define CLOCK_ADVANCE_TICKS  TICKS_PER_SECOND

// PWM for brightness control
// period is 16 ticks, at 2048Hz thats 2048/16 = 128 Hz PWM
// 128 Hz is fast enough that your eyes cant see the flicker (unlike the
// previous version which was 6Hz and looked like a disco... not great)
// brightness goes from 0 to 14, and on_ticks = brightness+1
//   level 0  = 1/16 duty  ~6%  (dim but visible in the dark, hopefully)
//   level 14 = 15/16 duty ~94% (basically max, close enough)
#define PWM_PERIOD_TICKS       16
#define BRIGHTNESS_LEVELS      15   // 15 levels, 0 through 14
#define BRIGHTNESS_DEFAULT      7   // start in the middle so it doesnt blind you on boot

// the whole state of the machine in one struct
// GetNextState and GetStateOutput are pure functions of this -- no hidden globals
typedef struct {
    // the actual clock time (0-11 for both, each minute position = 5 min)
    uint8_t  hour;
    uint8_t  minute;

    // which mode are we in right now
    uint8_t  mode;

    // flashing stuff for the setting modes
    uint8_t  flash_on;       // 1 = indicator is currently visible, 0 = hidden
    uint16_t flash_ticks;    // counts up to FLASH_HALF_PERIOD then toggles flash_on

    // how long has it been since the clock last moved (only used in normal mode)
    uint16_t clock_ticks;

    // button tracking -- need all 3 of these to detect short vs long press
    uint8_t  btn_was_down;      // what was the button doing last tick
    uint16_t btn_hold_ticks;    // how long has it been held this press
    uint8_t  long_press_done;   // did we already fire the long press? (prevents double-action)

    // PWM sub-state, cycles 0..PWM_PERIOD_TICKS-1 every tick
    uint8_t  pwm_tick;

    // brightness level 0-14, LED is on when pwm_tick < (brightness+1)
    uint8_t  brightness;
} state_t;

// computes the next state given current state + button input
// input: 1 = button pressed, 0 = button released
state_t GetNextState(state_t current_state, uint32_t input);

// computes what the LEDs should look like for the current state
// returns a GPIOA DOUT bitmask (active-low: 0 = LED on, 1 = LED off)
int GetStateOutput(state_t current_state);

#endif /* state_machine_logic_include */
