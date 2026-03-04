#include <ti/devices/msp/msp.h>
#include "hw_interface.h"

#define POWER_STARTUP_DELAY (32)

int led_mask; // bitmask of all the GPIO pins used for LEDs (set during init)

// hour LEDs are the outer ring, index 0 = 12 o'clock going clockwise
// each entry has the PA pin number and its IOMUX PINCM index
pin_config_t hour_leds[12] = {
    { .pin_number = 0,  .iomux = 1  },  // PA0
    { .pin_number = 26, .iomux = 59 },  // PA26
    { .pin_number = 24, .iomux = 54 },  // PA24
    { .pin_number = 22, .iomux = 47 },  // PA22
    { .pin_number = 18, .iomux = 40 },  // PA18
    { .pin_number = 16, .iomux = 38 },  // PA16
    { .pin_number = 14, .iomux = 36 },  // PA14
    { .pin_number = 12, .iomux = 34 },  // PA12
    { .pin_number = 10, .iomux = 21 },  // PA10
    { .pin_number = 8,  .iomux = 19 },  // PA8
    { .pin_number = 6,  .iomux = 11 },  // PA6
    { .pin_number = 28, .iomux = 3  },  // PA28
};

// minute LEDs are the inner ring, same deal
// index 0 = 12 o'clock, each position = 5 minutes
pin_config_t minute_leds[12] = {
    { .pin_number = 27, .iomux = 60 },  // PA27
    { .pin_number = 25, .iomux = 55 },  // PA25
    { .pin_number = 23, .iomux = 53 },  // PA23
    { .pin_number = 21, .iomux = 46 },  // PA21
    { .pin_number = 17, .iomux = 39 },  // PA17
    { .pin_number = 15, .iomux = 37 },  // PA15
    { .pin_number = 13, .iomux = 35 },  // PA13
    { .pin_number = 11, .iomux = 22 },  // PA11
    { .pin_number = 9,  .iomux = 20 },  // PA9
    { .pin_number = 7,  .iomux = 14 },  // PA7
    { .pin_number = 5,  .iomux = 10 },  // PA5
    { .pin_number = 1,  .iomux = 2  },  // PA1
};


// busy-wait delay in cpu cycles. used after powering on peripherals
// so they have time to actually turn on before we start using them.
// this is copied from TI's standard delay code -- dont touch it
void delay_cycles(uint32_t cycles)
{
    uint32_t scratch;
    __asm volatile(
#ifdef __GNUC__
        ".syntax unified\n\t"
#endif
        "SUBS %0, %[numCycles], #2; \n"
        "%=: \n\t"
        "SUBS %0, %0, #4; \n\t"
        "NOP; \n\t"
        "BHS  %=b;"
        : "=&r"(scratch)
        : [ numCycles ] "r"(cycles));
}


// sets up the pushbutton on PB8 as a GPIO input with a pull-up resistor
//
// the button connects PB8 to GND, so:
//   pressed   = pin reads 0 (pulled low)
//   released  = pin reads 1 (pulled high by the internal resistor)
//
// PB8 lives at IOMUX PINCM25 on the MSPM0G350x
// confirmed from the device header: IOMUX_PINCM25_PF_GPIOB_DIO08
void InitializeButton() {
    // power on GPIOB -- same dance as GPIOA in InitializeLEDs
    GPIOB->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W |
                            GPIO_RSTCTL_RESETSTKYCLR_CLR |
                            GPIO_RSTCTL_RESETASSERT_ASSERT);
    GPIOB->GPRCM.PWREN  = (GPIO_PWREN_KEY_UNLOCK_W |
                            GPIO_PWREN_ENABLE_ENABLE);
    delay_cycles(POWER_STARTUP_DELAY);

    // configure PB8 pin:
    //   IOMUX_PINCM25 = enum value 24 (0-indexed), maps to PB8
    //   function 0x1  = GPIO mode
    //   PC            = tell the IOMUX the pin is actually connected to something
    //   INENA         = enable the input buffer so DIN31_0 actually reads the pin
    //   PIPU          = pull the pin up internally (button pulls it down when pressed)
    const uint32_t input_cfg = IOMUX_PINCM_PC_CONNECTED
                              | IOMUX_PINCM_INENA_ENABLE
                              | IOMUX_PINCM_PIPU_ENABLE
                              | (uint32_t)0x00000001U;  // GPIO function select

    IOMUX->SECCFG.PINCM[IOMUX_PINCM25] = input_cfg;

    // PB8 stays as input by default since we never set DOESET31_0 for it
}

// reads the button and returns 1 if it's being pressed, 0 if not
// inverts the raw pin value because the hardware is active-low
inline uint32_t GetButtonState() {
    return !((GPIOB->DIN31_0 >> 8U) & 1U);
}


// sets up all 24 LEDs (12 hour + 12 minute) on GPIOA
// LEDs are active-low: writing 0 turns them ON, writing 1 turns them OFF
// led_mask ends up with a 1 in every bit position that has an LED
void InitializeLEDs() {
    // reset and power on GPIOA
    GPIOA->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);
    GPIOA->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    delay_cycles(POWER_STARTUP_DELAY);

    led_mask = 0;
    for (int i = 0; i < 12; i++) {
        // hour LEDs
        int iomux_entry = hour_leds[i].iomux - 1;  // PINCM is 1-indexed, array is 0-indexed
        IOMUX->SECCFG.PINCM[iomux_entry] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
        GPIOA->DOUTSET31_0 = (0x1 << hour_leds[i].pin_number);  // default high = LED off
        GPIOA->DOESET31_0 = (0x1 << hour_leds[i].pin_number);   // make it an output
        led_mask |= (0x1 << hour_leds[i].pin_number);

        // minute LEDs (same thing)
        iomux_entry = minute_leds[i].iomux - 1;
        IOMUX->SECCFG.PINCM[iomux_entry] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
        GPIOA->DOUTSET31_0 = (0x1 << minute_leds[i].pin_number);
        GPIOA->DOESET31_0 = (0x1 << minute_leds[i].pin_number);
        led_mask |= (0x1 << minute_leds[i].pin_number);
    }
}


// sets up the timer (TIMG0) to count down using the 32768 Hz LFCLK
// also configures the chip to sleep in STANDBY mode between interrupts
// so the battery doesnt die instantly
void InitializeTimerG0() {
    // step 1: reset and power on the timer module
    TIMG0->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);
    TIMG0->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    delay_cycles(16);

    // step 2: use LFCLK (32768 Hz low-power clock) instead of the fast bus clock
    // this is what lets us stay in standby -- LFCLK keeps running when the main clock stops
    TIMG0->CLKSEL = GPTIMER_CLKSEL_LFCLK_SEL_ENABLE;

    // step 3: make the timer repeat forever (not one-shot)
    TIMG0->COUNTERREGS.CTRCTL = GPTIMER_CTRCTL_REPEAT_REPEAT_1;

    // step 4: fire an interrupt when the counter hits zero
    TIMG0->CPU_INT.IMASK |= GPTIMER_CPU_INT_IMASK_Z_SET;

    // step 5: enable the clock going into the timer
    TIMG0->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;

    // step 6: configure sleep to use STANDBY mode
    // in standby, MCLK stops but LFCLK (and therefore TIMG0) keeps running
    // this means the timer will still wake us up even in deep sleep
    SYSCTL->SOCLOCK.PMODECFG = SYSCTL_PMODECFG_DSLEEP_STANDBY;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    SYSCTL->SOCLOCK.MCLKCFG |= SYSCTL_MCLKCFG_STOPCLKSTBY_ENABLE;
}

// sets the timer period. call this before EnableTimerG0.
// LOAD=16 gives 32768/16 = 2048 Hz
inline void SetTimerG0Delay(uint16_t delay) {
    TIMG0->COUNTERREGS.LOAD = delay;
}

// actually starts the timer and enables its interrupt in the NVIC
inline void EnableTimerG0() {
    TIMG0->COUNTERREGS.CTRCTL |= (GPTIMER_CTRCTL_EN_ENABLED);
    NVIC_EnableIRQ(TIMG0_INT_IRQn);
}


// the timer interrupt handler -- this is what wakes the cpu from __WFI()
// we dont actually need to do anything here, just clearing the interrupt
// is enough. all the real work happens back in main after we wake up.
void TIMG0_IRQHandler(void)
{
    switch (TIMG0->CPU_INT.IIDX) {
        case GPTIMER_CPU_INT_IIDX_STAT_Z:
            break;  // nothing to do, waking up is the whole point
        default:
            break;
    }
}
