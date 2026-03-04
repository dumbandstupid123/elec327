/*
 * Copyright (c) 2026, Caleb Kemere
 * All rights reserved, see LICENSE.md
 *
 * Lab 6 — SPI RGB LED + buzzer control via state machine
 *
 * On power-up the board plays "Mary Had a Little Lamb" on the buzzer
 * while the four SPI RGB LEDs cycle through three distinct color patterns
 * (one per note: red=E6, blue=D6, yellow=C6) in sync with the music.
 * Inter-note silences leave all LEDs dark.
 *
 * Pressing any button stops the song and enters button-feedback mode:
 *   • The LED nearest the pressed button lights its Simon color
 *     (SW1=green, SW2=red, SW3=yellow, SW4=blue) and plays the
 *     corresponding Simon tone.
 *   • Releasing all buttons turns all LEDs off and silences the buzzer.
 *
 * Two ISRs are active (TIMG0 and SPI0).  Only TIMG0 ticks advance the
 * state machine; SPI0 interrupts are used solely to clock out the LED
 * message and are otherwise ignored by the main loop.
 */

#include <ti/devices/msp/msp.h>
#include "delay.h"
#include "buttons.h"
#include "timing.h"
#include "buzzer.h"
#include "leds.h"
#include "state_machine.h"

int main(void)
{
    InitializeButtonGPIO();
    InitializeBuzzer();       // starts buzzer at 2 kHz
    InitializeLEDInterface();
    InitializeTimerG0();

    // Brief startup beep so we know the hardware is alive
    delay_cycles(1600000);    // ~0.1 s at 32 MHz
    DisableBuzzer();

    // Build initial state and prime first note (enables buzzer + sets LED ptr)
    state_t state = InitStateMachine();

    SetTimerG0Delay(20);      // ~0.641 ms per tick (21 counts at 32768 Hz)
    EnableTimerG0();

    while (1) {
        if (timer_wakeup) {
            timer_wakeup = false;

            // Advance state machine: updates buzzer and current_led_packet
            state = TickStateMachine(state);

            // Push current LED color pattern out over SPI.
            // Spin if the previous SPI message is still in flight (rare:
            // SPI takes ~96 µs, timer period is ~641 µs).
            while (!SendSPIMessage(current_led_packet, LED_MSG_LEN)) {}
        }

        // Clear any SPI wakeup flag — we handle SPI entirely in its ISR
        spi_wakeup = false;

        __WFI(); // sleep until the next interrupt (timer or SPI)
    }
}
