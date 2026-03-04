// lab 3 - setting the clock with a button
//
// ok so heres the deal. the timer fires ~2048 times a second and every time
// it does, we wake up, check the button, update the LEDs, compute the next
// state, and go back to sleep. thats literally the whole program.
//
// the important thing is that GetNextState and GetStateOutput are PURE
// FUNCTIONS -- they only depend on what you pass in. no hidden globals.
// thats what makes this a real state machine and not just spaghetti code.
//
// timer is running at LFCLK/16 = 2048 Hz so:
//   - PWM runs at 128 Hz (you cant see it flicker, trust me i checked)
//   - button debounce threshold is ~7ms (spec says 5ms minimum, were good)
//   - long press fires after exactly 1 second

#include <ti/devices/msp/msp.h>
#include "hw_interface.h"
#include "state_machine_logic.h"

int main(void)
{
    // turn everything on
    InitializeLEDs();
    InitializeButton();
    InitializeTimerG0();

    // set up the initial state -- clock starts at 12:00, normal mode
    // have to initialize every field because theres no memset in this template
    state_t state;
    state.hour            = 0;
    state.minute          = 0;
    state.mode            = MODE_NORMAL;
    state.flash_on        = 0;       // not flashing yet
    state.flash_ticks     = 0;
    state.clock_ticks     = 0;
    state.btn_was_down    = 0;       // button starts unpressed, shockingly
    state.btn_hold_ticks  = 0;
    state.long_press_done = 0;
    state.pwm_tick        = 0;
    state.brightness      = BRIGHTNESS_DEFAULT;  // mid brightness to start

    // LOAD=16 means the timer counts down from 16 to 0 using the 32768 Hz LFCLK
    // thats 32768/16 = 2048 interrupts per second, about every 0.49ms
    SetTimerG0Delay(16);
    EnableTimerG0();

    while (1) {
        // 1. is the button being pressed right now? (1=yes, 0=no)
        uint32_t input = GetButtonState();

        // 2. figure out what the LEDs should look like and write it
        // read-modify-write so we dont accidentally clobber any non-LED pins
        int gpio_out = GPIOA->DOUT31_0;
        gpio_out &= ~led_mask;              // wipe just the LED bits
        gpio_out |= GetStateOutput(state);  // drop in the new LED values
        GPIOA->DOUT31_0 = gpio_out;

        // 3. advance the state machine (this is where all the logic lives)
        state = GetNextState(state, input);

        // 4. go to sleep until the next timer tick fires
        // the cpu will be in standby here so power consumption is low
        __WFI();
    }
}


/*
 * Copyright (c) 2026, Caleb Kemere
 * Derived from example code which is
 *
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
