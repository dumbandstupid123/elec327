#ifndef state_machine_include
#define state_machine_include

#include <stdbool.h>
#include <stdint.h>

#define TOP_MUSIC    0
#define TOP_BUTTONS  1

// SPI message length: 2 start words + (2 words × 4 LEDs) + 2 end words
#define LED_MSG_LEN  12

typedef struct {
    uint8_t  top;        // TOP_MUSIC or TOP_BUTTONS
    uint8_t  note_idx;   // current note index in song[]
    uint16_t tick;       // ticks elapsed in current note slot
    uint8_t  in_gap;     // 1 = inter-note silence, 0 = tone on
    uint8_t  db[4];      // per-button debounce counters (saturates at threshold)
    uint8_t  pressed[4]; // debounced press state: 1=pressed, 0=released
} state_t;

// Pointer to the LED SPI packet that should be sent this tick.
// Updated by TickStateMachine(); read by the main loop.
extern uint16_t *current_led_packet;

state_t InitStateMachine(void);
state_t TickStateMachine(state_t s);

#endif // state_machine_include
