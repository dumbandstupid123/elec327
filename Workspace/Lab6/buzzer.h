#ifndef buzzer_include
#define buzzer_include

#include <stdbool.h>
#include <stdint.h>

// Note periods for TIMA1 (8 MHz clock): frequency = 8000000 / (LOAD + 1)
#define NOTE_G6   5102u   // G6  ~ 1568 Hz  (Green button SW1)
#define NOTE_E6   6065u   // E6  ~ 1319 Hz  (Red button SW2; song note)
#define NOTE_D6   6810u   // D6  ~ 1175 Hz  (song note)
#define NOTE_C6   7644u   // C6  ~ 1047 Hz  (Yellow button SW3; song note)
#define NOTE_G5  10203u   // G5  ~  784 Hz  (Blue button SW4)

// Simon button tone assignments
#define TONE_SW1  NOTE_G6
#define TONE_SW2  NOTE_E6
#define TONE_SW3  NOTE_C6
#define TONE_SW4  NOTE_G5

void InitializeBuzzer(void);

void SetBuzzerPeriod(uint16_t period);
void EnableBuzzer(void);
void DisableBuzzer(void);

#endif // buzzer_include