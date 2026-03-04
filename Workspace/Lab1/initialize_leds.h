#ifndef initialize_leds_include
#define initialize_leds_include

// Define LED pin mask for all the LEDs
#define ALL_LEDS_MASK ( \
    (1u<<0) | (1u<<1) | (1u<<5) | (1u<<7) | (1u<<8) | (1u<<9) | \
    (1u<<10) | (1u<<11) | (1u<<12) | (1u<<13) | (1u<<14) | (1u<<15) | \
    (1u<<16) | (1u<<17) | (1u<<18) | (1u<<21) | (1u<<22) | (1u<<23) | \
    (1u<<24) | (1u<<25) | (1u<<26) | (1u<<27) | (1u<<28) \
)

// Function prototypes
void InitializeGPIO(void);

#endif /* initialize_leds_include */