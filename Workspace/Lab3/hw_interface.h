// hw_interface.h -- everything that touches actual hardware lives here
//
// the idea is to keep all the register-poking in hw_interface.c/.h so that
// state_machine_logic.c never has to care about GPIO or IOMUX or any of that.
// state machine just calls GetButtonState() and trusts it returns 1 or 0.
// much easier to debug when the hardware layer and logic layer dont mix.

#ifndef hw_interface_include
#define hw_interface_include

#include <stdint.h>

// ----- init functions -----
// call all three of these at startup before you do anything else
// order doesnt matter much but timer last makes sense so it doesnt fire before youre ready

void InitializeLEDs();      // powers up GPIOA, sets all 24 LED pins as outputs (all off to start)
void InitializeButton();    // powers up GPIOB, sets PB8 as input with pull-up for the button
void InitializeTimerG0();   // sets TIMG0 to use LFCLK and configures STANDBY sleep mode

// SetTimerG0Delay: set how many LFCLK cycles between interrupts
//   LOAD=16 → 32768/16 = 2048 Hz (one tick every ~0.49ms)
//   call this BEFORE EnableTimerG0 or it wont take effect
void SetTimerG0Delay(uint16_t delay);

// EnableTimerG0: actually starts the countdown and turns on the NVIC interrupt
//   after this the cpu will get an interrupt every LOAD cycles and wake from __WFI()
void EnableTimerG0();

// GetButtonState: returns 1 if the button is pressed, 0 if released
//   handles the active-low inversion internally so callers dont have to think about it
uint32_t GetButtonState();


// ----- LED pin mapping -----
// each LED is described by its GPIO pin number (PAx) and its IOMUX PINCM index
// these are two separate numbering schemes because of course they are

typedef struct {
    int pin_number;  // PAx → x (used for bit-shifting into DOUT/DOESET registers)
    int iomux;       // 1-indexed PINCM entry (we subtract 1 when indexing into SECCFG.PINCM[])
} pin_config_t;

// the actual pin tables -- defined in hw_interface.c
// hour_leds[0]   = 12 o'clock on the outer ring, going clockwise
// minute_leds[0] = 12 o'clock on the inner ring, going clockwise
// each step = 1 hour / 5 minutes on the clock face
extern pin_config_t hour_leds[12];
extern pin_config_t minute_leds[12];

// bitmask of all GPIO pins used for LEDs
// built during InitializeLEDs() so we know which bits to preserve when writing GPIOA
// (we dont want to accidentally toggle non-LED pins)
extern int led_mask;

#endif /* hw_interface_include */