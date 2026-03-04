/*
 * state_machine.c — Lab 6 state machine: song playback + button feedback
 *
 * Copyright (c) 2026, Caleb Kemere / ELEC 327
 * All rights reserved, see LICENSE.md
 *
 * State machine has two top-level modes:
 *   TOP_MUSIC  : plays "Mary Had a Little Lamb" on buzzer + matching LED colors
 *   TOP_BUTTONS: any button press → corresponding LED lights up + button tone
 *
 * Transition: TOP_MUSIC → TOP_BUTTONS on any debounce-confirmed button press.
 * The transition is one-way (stays in button mode).
 */

#include "state_machine.h"
#include "buzzer.h"
#include "buttons.h"
#include <ti/devices/msp/msp.h>

// ---------------------------------------------------------------------------
// SPI LED packet format (APA102C, 16-bit SPI words, MSB first)
//
// Full message layout (12 × uint16_t):
//   [0..1]   : start frame (0x0000 0x0000)
//   [2..3]   : LED 0 word1, word2
//   [4..5]   : LED 1 word1, word2
//   [6..7]   : LED 2 word1, word2
//   [8..9]   : LED 3 word1, word2
//   [10..11] : end frame (0xFFFF 0xFFFF)
//
// LED word1 = ((0xE0 | brightness) << 8) | blue   (brightness 0-31)
// LED word2 = (green << 8) | red
//
// Color design rationale:
//   Note colors use pure RGB primaries mapped to musical pitch height:
//     E6 (highest) → Red   — warm/high energy
//     D6 (middle)  → Blue  — cool/neutral
//     C6 (lowest)  → Green — calm/low
//   Using pure primaries ensures the three colors are unambiguous at any
//   brightness level (no two-component mixes that can look alike).
//   Rests/inter-note gaps → all LEDs off
//
//   Button colors follow the classic Simon game palette:
//     SW1 (Green button)  → LED 0 green
//     SW2 (Red button)    → LED 1 red
//     SW3 (Yellow button) → LED 2 yellow
//     SW4 (Blue button)   → LED 3 blue
// ---------------------------------------------------------------------------

#define BRT_NOTE  8u   // brightness 0-31 for song note display
#define BRT_BTN  12u   // brightness 0-31 for button press display

// Convenience: off LED words
#define W_OFF1  0xE000u
#define W_OFF2  0x0000u
#define W_END   0xFFFFu

// --- Note LED packets (all 4 LEDs same color) ---

// E6 → RED: blue=0, green=0, red=255
static uint16_t pkt_e6[LED_MSG_LEN] = {
    0x0000, 0x0000,
    0xE800u, 0x00FFu,
    0xE800u, 0x00FFu,
    0xE800u, 0x00FFu,
    0xE800u, 0x00FFu,
    W_END, W_END
};

// D6 → BLUE: blue=255, green=0, red=0
static uint16_t pkt_d6[LED_MSG_LEN] = {
    0x0000, 0x0000,
    0xE8FFu, 0x0000u,
    0xE8FFu, 0x0000u,
    0xE8FFu, 0x0000u,
    0xE8FFu, 0x0000u,
    W_END, W_END
};

// C6 → GREEN: blue=0, green=255, red=0
// (Pure green keeps all 3 note colors as distinct RGB primaries so they
//  are unambiguous even at low brightness: E6=red, D6=blue, C6=green)
static uint16_t pkt_c6[LED_MSG_LEN] = {
    0x0000, 0x0000,
    0xE800u, 0xFF00u,
    0xE800u, 0xFF00u,
    0xE800u, 0xFF00u,
    0xE800u, 0xFF00u,
    W_END, W_END
};

// All LEDs off (inter-note silence)
static uint16_t pkt_off[LED_MSG_LEN] = {
    0x0000, 0x0000,
    W_OFF1, W_OFF2,
    W_OFF1, W_OFF2,
    W_OFF1, W_OFF2,
    W_OFF1, W_OFF2,
    W_END, W_END
};

// --- Button LED packets (one LED lit, others off) ---

// SW1 → LED 0 GREEN: blue=0, green=255, red=0
static uint16_t pkt_sw1[LED_MSG_LEN] = {
    0x0000, 0x0000,
    0xEC00u, 0xFF00u,   // LED 0: green
    W_OFF1,  W_OFF2,
    W_OFF1,  W_OFF2,
    W_OFF1,  W_OFF2,
    W_END, W_END
};

// SW2 → LED 1 RED: blue=0, green=0, red=255
static uint16_t pkt_sw2[LED_MSG_LEN] = {
    0x0000, 0x0000,
    W_OFF1,  W_OFF2,
    0xEC00u, 0x00FFu,   // LED 1: red
    W_OFF1,  W_OFF2,
    W_OFF1,  W_OFF2,
    W_END, W_END
};

// SW3 → LED 2 YELLOW: blue=0, green=200, red=255
static uint16_t pkt_sw3[LED_MSG_LEN] = {
    0x0000, 0x0000,
    W_OFF1,  W_OFF2,
    W_OFF1,  W_OFF2,
    0xEC00u, 0xC8FFu,   // LED 2: yellow
    W_OFF1,  W_OFF2,
    W_END, W_END
};

// SW4 → LED 3 BLUE: blue=255, green=0, red=0
static uint16_t pkt_sw4[LED_MSG_LEN] = {
    0x0000, 0x0000,
    W_OFF1,  W_OFF2,
    W_OFF1,  W_OFF2,
    W_OFF1,  W_OFF2,
    0xECFFu, 0x0000u,   // LED 3: blue
    W_END, W_END
};

// Public pointer used by main loop to know which packet to transmit
uint16_t *current_led_packet = pkt_off;

// ---------------------------------------------------------------------------
// Song definition: "Mary Had a Little Lamb"
// ---------------------------------------------------------------------------

// Timing constants (TIMG0 at LFCLK 32768 Hz, LOAD=20 → ~0.641 ms/tick)
#define QUARTER_TOTAL   800u   // ticks per quarter-note slot (~513 ms)
#define INTERNOTE_GAP    50u   // ticks of silence between every note
#define QUARTER_SOUND   (QUARTER_TOTAL - INTERNOTE_GAP)            // 750 ticks
#define HALF_SOUND      (2u * QUARTER_TOTAL - INTERNOTE_GAP)       // 1550 ticks
#define WHOLE_SOUND     (4u * QUARTER_TOTAL - INTERNOTE_GAP)       // 3150 ticks

#define DEBOUNCE_THRESH  10u   // ticks (~6.4 ms) to confirm a button press

typedef struct {
    uint16_t period;  // TIMA1 LOAD value (sets pitch)
    uint8_t  dur;     // 1=quarter, 2=half, 4=whole
} Note;

static const Note song[] = {
    // "Mary had a"
    {NOTE_E6, 1}, {NOTE_D6, 1}, {NOTE_C6, 1}, {NOTE_D6, 1},
    // "lit-tle lamb,"
    {NOTE_E6, 1}, {NOTE_E6, 1}, {NOTE_E6, 2},
    // "lit-tle lamb,"
    {NOTE_D6, 1}, {NOTE_D6, 1}, {NOTE_D6, 2},
    // "lit-tle lamb."
    {NOTE_E6, 1}, {NOTE_E6, 1}, {NOTE_E6, 2},
    // "Mary had a lit-tle lamb whose"
    {NOTE_E6, 1}, {NOTE_D6, 1}, {NOTE_C6, 1}, {NOTE_D6, 1},
    {NOTE_E6, 1}, {NOTE_E6, 1}, {NOTE_E6, 1}, {NOTE_C6, 1},
    // "fleece was white as"
    {NOTE_D6, 1}, {NOTE_D6, 1}, {NOTE_E6, 1}, {NOTE_D6, 1},
    // "snow."
    {NOTE_C6, 4},
};

#define SONG_LENGTH  ((uint8_t)(sizeof(song) / sizeof(Note)))

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static uint16_t *NoteLEDPacket(uint16_t period) {
    if (period == NOTE_E6) return pkt_e6;
    if (period == NOTE_D6) return pkt_d6;
    return pkt_c6;  // NOTE_C6 (and any other pitch defaults to yellow)
}

static uint16_t NoteSoundTicks(uint8_t dur) {
    if (dur == 1) return QUARTER_SOUND;
    if (dur == 2) return HALF_SOUND;
    return WHOLE_SOUND;
}

static uint16_t NoteTotalTicks(uint8_t dur) {
    return (uint16_t)(dur * QUARTER_TOTAL);
}

static void UpdateDebounce(state_t *s) {
    uint32_t raw = GPIOA->DIN31_0;
    // Buttons are active-low; invert so pressed = 1
    uint8_t r[4];
    r[0] = ((raw & SW1) == 0) ? 1u : 0u;
    r[1] = ((raw & SW2) == 0) ? 1u : 0u;
    r[2] = ((raw & SW3) == 0) ? 1u : 0u;
    r[3] = ((raw & SW4) == 0) ? 1u : 0u;

    for (int i = 0; i < 4; i++) {
        if (r[i]) {
            if (s->db[i] < DEBOUNCE_THRESH) s->db[i]++;
        } else {
            if (s->db[i] > 0) s->db[i]--;
        }
        s->pressed[i] = (s->db[i] >= DEBOUNCE_THRESH) ? 1u : 0u;
    }
}

static uint8_t AnyPressed(const state_t *s) {
    return s->pressed[0] | s->pressed[1] | s->pressed[2] | s->pressed[3];
}

static void AdvanceMusicTick(state_t *s) {
    s->tick++;

    uint16_t sound_ticks = NoteSoundTicks(song[s->note_idx].dur);
    uint16_t total_ticks = NoteTotalTicks(song[s->note_idx].dur);

    // Transition from tone to inter-note silence
    if (!s->in_gap && s->tick >= sound_ticks) {
        s->in_gap = 1;
        DisableBuzzer();
        current_led_packet = pkt_off;
    }

    // Advance to the next note when the full slot has elapsed
    if (s->tick >= total_ticks) {
        s->note_idx = (uint8_t)((s->note_idx + 1u) % SONG_LENGTH);
        s->tick   = 0;
        s->in_gap = 0;
        SetBuzzerPeriod(song[s->note_idx].period);
        EnableBuzzer();
        current_led_packet = NoteLEDPacket(song[s->note_idx].period);
    }
}

static void SetButtonOutput(const state_t *s) {
    if (s->pressed[0]) {
        SetBuzzerPeriod(TONE_SW1); EnableBuzzer();
        current_led_packet = pkt_sw1;
    } else if (s->pressed[1]) {
        SetBuzzerPeriod(TONE_SW2); EnableBuzzer();
        current_led_packet = pkt_sw2;
    } else if (s->pressed[2]) {
        SetBuzzerPeriod(TONE_SW3); EnableBuzzer();
        current_led_packet = pkt_sw3;
    } else if (s->pressed[3]) {
        SetBuzzerPeriod(TONE_SW4); EnableBuzzer();
        current_led_packet = pkt_sw4;
    } else {
        DisableBuzzer();
        current_led_packet = pkt_off;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

state_t InitStateMachine(void) {
    state_t s;
    s.top      = TOP_MUSIC;
    s.note_idx = 0;
    s.tick     = 0;
    s.in_gap   = 0;
    for (int i = 0; i < 4; i++) {
        s.db[i]      = 0;
        s.pressed[i] = 0;
    }
    // Prime the first note so buzzer and LEDs are ready before the first tick
    SetBuzzerPeriod(song[0].period);
    EnableBuzzer();
    current_led_packet = NoteLEDPacket(song[0].period);
    return s;
}

state_t TickStateMachine(state_t s) {
    UpdateDebounce(&s);

    if (s.top == TOP_MUSIC) {
        if (AnyPressed(&s)) {
            // One-way transition to button feedback mode
            s.top   = TOP_BUTTONS;
            s.tick  = 0;
            s.in_gap = 0;
        } else {
            AdvanceMusicTick(&s);
        }
    }

    if (s.top == TOP_BUTTONS) {
        SetButtonOutput(&s);
    }

    return s;
}
