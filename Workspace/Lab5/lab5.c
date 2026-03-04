
/*
 * Lab 5 - Generating sounds using PWM
 * Copyright (c) 2026, Caleb Kemere / Student
 *
 * Architecture (state machine):
 *
 *   TOP_MUSIC  (startup) ──── any button debounced pressed ────► TOP_BUTTONS
 *       │                                                             │
 *       ▼                                                             ▼
 *   Cycle through song[] array, one note per slot.           Each button plays
 *   Each slot = sound portion + inter-note gap.              a distinct tone.
 *   TIMG0 ISR fires every ~0.641ms to drive the loop.       Buzzer off when
 *                                                             all released.
 *
 * Timer: TIMG0 at LFCLK/21 ≈ 0.641ms per tick  (wakes main loop via WFI)
 * PWM:   TIMA1 at BUSCLK/4 = 8 MHz, freq = 8MHz / (LOAD+1)
 */

#include <ti/devices/msp/msp.h>
#include "hw_interface.h"

/* =========================================================================
 * NOTE FREQUENCY LOAD VALUES  (TIMA1 at 8 MHz, LOAD = 8MHz/freq - 1)
 *
 * Using notes TWO OCTAVES ABOVE the standard Simon PCB assignments so the
 * output lands near the buzzer's 4-8 kHz fundamental range, which reduces
 * nonlinear distortion from the piezo membrane.
 *
 * Standard Simon assignments (from assignment spec):
 *   Green  SW1  G4  391.995 Hz  → G6  1567.98 Hz  LOAD 5102
 *   Red    SW2  E4  329.628 Hz  → E6  1318.51 Hz  LOAD 6065
 *   Yellow SW3  C4  261.626 Hz  → C6  1046.50 Hz  LOAD 7644
 *   Blue   SW4  G3  195.998 Hz  → G5   783.99 Hz  LOAD 10203
 *
 * Song notes (Mary Had a Little Lamb uses E, D, C):
 *   E  two-octave-up = E6  1318.51 Hz  LOAD 6065
 *   D  two-octave-up = D6  1174.66 Hz  LOAD 6810
 *   C  two-octave-up = C6  1046.50 Hz  LOAD 7644
 * ========================================================================= */
#define NOTE_G6   5102u   /* G6  = 1568 Hz  (Green button SW1)               */
#define NOTE_E6   6065u   /* E6  = 1319 Hz  (Red button SW2; also song E)    */
#define NOTE_D6   6810u   /* D6  = 1175 Hz  (used in song)                   */
#define NOTE_C6   7644u   /* C6  = 1047 Hz  (Yellow button SW3; also song C) */
#define NOTE_G5  10203u   /* G5  =  784 Hz  (Blue button SW4)                */

/* Simon button tone assignments */
#define TONE_SW1  NOTE_G6   /* Green  */
#define TONE_SW2  NOTE_E6   /* Red    */
#define TONE_SW3  NOTE_C6   /* Yellow */
#define TONE_SW4  NOTE_G5   /* Blue   */

/* =========================================================================
 * MUSIC TIMING
 *
 * TIMG0 fires every 21 LFCLK cycles (LFCLK = 32768 Hz, LOAD = 20):
 *   tick period = 21 / 32768 ≈ 0.641 ms
 *
 * A note "slot" = sound portion + inter-note gap (silence).
 * The rubric requires:  whole slot = 4 × quarter slot
 *                       half  slot = 2 × quarter slot
 *
 * The gap (INTERNOTE_GAP) is the SAME for every note length, so that
 * the slot ratios are preserved:
 *   quarter slot  =  QUARTER_TOTAL ticks  =  1200 × 0.641ms ≈  769 ms
 *   half    slot  = 2×QUARTER_TOTAL ticks  = 2400 × 0.641ms ≈ 1538 ms
 *   whole   slot  = 4×QUARTER_TOTAL ticks  = 4800 × 0.641ms ≈ 3077 ms
 *
 * Sound ticks = slot ticks − INTERNOTE_GAP
 *   quarter sound = (1200 - 80) = 1120 ticks ≈ 718 ms
 *   half    sound = (2400 - 80) = 2320 ticks ≈ 1487 ms
 *   whole   sound = (4800 - 80) = 4720 ticks ≈ 3024 ms
 *
 * ── HOW TO CHANGE TEMPO ─────────────────────────────────────────────────
 *   Faster: decrease QUARTER_TOTAL (e.g., 900 → ~577 ms per quarter note)
 *   Slower: increase QUARTER_TOTAL (e.g., 1600 → ~1025 ms per quarter note)
 *   Longer gaps: increase INTERNOTE_GAP (keep < QUARTER_TOTAL)
 * ========================================================================= */
#define QUARTER_TOTAL   1200u   /* ticks per quarter-note slot (sound + gap) */
#define INTERNOTE_GAP     80u   /* ticks of silence between notes            */

#define QUARTER_SOUND   (QUARTER_TOTAL - INTERNOTE_GAP)           /* 1120 */
#define HALF_SOUND      (2u * QUARTER_TOTAL - INTERNOTE_GAP)      /* 2320 */
#define WHOLE_SOUND     (4u * QUARTER_TOTAL - INTERNOTE_GAP)      /* 4720 */

/* =========================================================================
 * SONG DEFINITION  ── "Mary Had a Little Lamb" ──
 *
 * Each entry: { NOTE_xx, duration }
 *   NOTE_xx : one of the NOTE_* defines above (sets the pitch)
 *   duration: 1 = quarter note, 2 = half note, 4 = whole note
 *
 * ── HOW TO CHANGE THE SONG ──────────────────────────────────────────────
 *   1. Edit the entries in song[] below.
 *   2. Add/remove entries freely – SONG_LENGTH updates automatically.
 *   3. Use NOTE_G6/E6/D6/C6/G5 for pitches, or add new NOTE_* defines.
 *   4. Adjust QUARTER_TOTAL above to change the overall tempo.
 * ========================================================================= */
typedef struct {
    uint16_t period;  /* TIMA1 LOAD value (pitch) */
    uint8_t  dur;     /* 1=quarter, 2=half, 4=whole */
} Note;

static const Note song[] = {
    /* "Mary had a"       */ {NOTE_E6, 1}, {NOTE_D6, 1}, {NOTE_C6, 1}, {NOTE_D6, 1},
    /* "lit-tle"          */ {NOTE_E6, 1}, {NOTE_E6, 1},
    /* "lamb,"            */ {NOTE_E6, 2},
    /* "lit-tle"          */ {NOTE_D6, 1}, {NOTE_D6, 1},
    /* "lamb,"            */ {NOTE_D6, 2},
    /* "lit-tle"          */ {NOTE_E6, 1}, {NOTE_E6, 1},
    /* "lamb."            */ {NOTE_E6, 2},

    /* "Mary had a"       */ {NOTE_E6, 1}, {NOTE_D6, 1}, {NOTE_C6, 1}, {NOTE_D6, 1},
    /* "lit-tle lamb whose" */ {NOTE_E6, 1}, {NOTE_E6, 1}, {NOTE_E6, 1}, {NOTE_C6, 1},
    /* "fleece was"       */ {NOTE_D6, 1}, {NOTE_D6, 1}, {NOTE_E6, 1}, {NOTE_D6, 1},
    /* "white as snow."   */ {NOTE_C6, 4},
};

#define SONG_LENGTH  ((uint8_t)(sizeof(song) / sizeof(Note)))

/* =========================================================================
 * DEBOUNCE
 * Button must read consistently for DEBOUNCE_THRESH ticks to be confirmed.
 * 10 ticks × 0.641ms ≈ 6.4ms  (spec requires ≥ 5ms)
 * ========================================================================= */
#define DEBOUNCE_THRESH  10u

/* =========================================================================
 * STATE MACHINE TYPES
 * ========================================================================= */
#define TOP_MUSIC    0u   /* startup: plays the song on loop        */
#define TOP_BUTTONS  1u   /* button-tone mode (one-way transition)  */

typedef struct {
    uint8_t  top;        /* TOP_MUSIC or TOP_BUTTONS                */

    /* Music playback (active only in TOP_MUSIC) */
    uint8_t  note_idx;   /* current note index into song[]          */
    uint16_t tick;       /* ticks elapsed in current note slot      */
    uint8_t  in_gap;     /* 1 = inter-note silence, 0 = tone on     */

    /* Button debounce (active in both states) */
    uint8_t  db[4];      /* per-button counters, saturate 0..THRESH */
    uint8_t  pressed[4]; /* debounced state: 1=pressed, 0=released  */
} state_t;

/* =========================================================================
 * HELPER FUNCTIONS
 * ========================================================================= */

/* Read raw (not debounced) button states.
 * raw[i] = 1 if button i is pressed (buttons are active-low). */
static void ReadRawButtons(uint8_t raw[4])
{
    uint32_t din = GPIOA->DIN31_0;
    raw[0] = ((din & SW1) == 0u) ? 1u : 0u;
    raw[1] = ((din & SW2) == 0u) ? 1u : 0u;
    raw[2] = ((din & SW3) == 0u) ? 1u : 0u;
    raw[3] = ((din & SW4) == 0u) ? 1u : 0u;
}

/* Update debounce counters from raw readings.
 * Returns 1 if at least one button is debounce-confirmed pressed. */
static uint8_t UpdateDebounce(state_t *s, const uint8_t raw[4])
{
    uint8_t any = 0u;
    for (int i = 0; i < 4; i++) {
        if (raw[i]) {
            if (s->db[i] < DEBOUNCE_THRESH) { s->db[i]++; }
        } else {
            if (s->db[i] > 0u)              { s->db[i]--; }
        }
        s->pressed[i] = (s->db[i] >= DEBOUNCE_THRESH) ? 1u : 0u;
        if (s->pressed[i]) { any = 1u; }
    }
    return any;
}

/* Sound-playing ticks for a note of the given duration code. */
static uint16_t SoundTicks(uint8_t dur)
{
    if (dur == 4u) { return (uint16_t)WHOLE_SOUND;   }
    if (dur == 2u) { return (uint16_t)HALF_SOUND;    }
    return (uint16_t)QUARTER_SOUND;
}

/* Total slot ticks (sound + gap) for a note of the given duration code. */
static uint16_t TotalTicks(uint8_t dur)
{
    return (uint16_t)((uint16_t)dur * (uint16_t)QUARTER_TOTAL);
}

/* Advance music playback by one timer tick. */
static state_t AdvanceMusicTick(state_t s)
{
    uint16_t sound_end = SoundTicks(song[s.note_idx].dur);
    uint16_t total_end = TotalTicks(song[s.note_idx].dur);

    s.tick++;

    /* Crossed into the inter-note gap? */
    if (!s.in_gap && s.tick >= sound_end) {
        s.in_gap = 1u;
    }

    /* Finished the entire slot? Advance to next note (wraps to start). */
    if (s.tick >= total_end) {
        s.note_idx = (uint8_t)((s.note_idx + 1u) % SONG_LENGTH);
        s.tick     = 0u;
        s.in_gap   = 0u;
    }

    return s;
}

/* Drive the buzzer based on the current state.
 * In MUSIC mode:   enables PWM during the sound portion, disables during gap.
 * In BUTTONS mode: plays the tone of the lowest-indexed pressed button,
 *                  or disables PWM if no button is pressed. */
static void SetPWMOutput(const state_t *s)
{
    static const uint16_t btn_tones[4] = {TONE_SW1, TONE_SW2, TONE_SW3, TONE_SW4};

    if (s->top == TOP_MUSIC) {
        if (!s->in_gap) {
            SetTimerA1Period(song[s->note_idx].period);
            EnableTimerA1PWM();
        } else {
            DisableTimerA1PWM();
        }
    } else {
        /* TOP_BUTTONS: first pressed button wins */
        uint8_t played = 0u;
        for (int i = 0; i < 4; i++) {
            if (s->pressed[i]) {
                SetTimerA1Period(btn_tones[i]);
                EnableTimerA1PWM();
                played = 1u;
                break;
            }
        }
        if (!played) {
            DisableTimerA1PWM();
        }
    }
}

/* =========================================================================
 * MAIN
 * ========================================================================= */
int main(void)
{
    InitializeGPIO();
    InitializeTimerG0();
    InitializeTimerA1_PWM();   /* starts buzzer at 2 kHz */

    /* Brief startup beep (~0.1 s) so we know the buzzer works */
    delay_cycles(1600000);
    DisableTimerA1PWM();

    /* TIMG0: fires every 21 LFCLK cycles ≈ 0.641 ms per tick */
    SetTimerG0Delay(20);
    EnableTimerG0();

    /* Zero-initialise → TOP_MUSIC, note 0, tick 0, no buttons pressed */
    state_t state = {0};

    while (1) {
        uint8_t raw[4];
        ReadRawButtons(raw);
        uint8_t any_pressed = UpdateDebounce(&state, raw);

        if (state.top == TOP_MUSIC) {
            if (any_pressed) {
                /* Button pressed: stop song, enter button-tone mode */
                state.top = TOP_BUTTONS;
            } else {
                state = AdvanceMusicTick(state);
            }
        }
        /* In TOP_BUTTONS mode UpdateDebounce already updated pressed[],
         * so SetPWMOutput below handles everything. */

        SetPWMOutput(&state);

        __WFI();   /* sleep until next TIMG0 tick */
    }
}
