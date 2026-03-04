#include "initialize_leds.h"
#include "delay.h"
#include <ti/devices/msp/msp.h>

#define POWER_STARTUP_DELAY (16)

#define ALL_LEDS_MASK \
    ((1u<<0) | (1u<<1) | (1u<<5) | (1u<<6) | (1u<<7) | (1u<<8) | (1u<<9) | \
     (1u<<10) | (1u<<11) | (1u<<12) | (1u<<13) | (1u<<14) | (1u<<15) | \
     (1u<<16) | (1u<<17) | (1u<<18) | (1u<<21) | (1u<<22) | (1u<<23) | \
     (1u<<24) | (1u<<25) | (1u<<26) | (1u<<27) | (1u<<28))

void InitializeGPIO(void) {
    // 1. Reset GPIO port (the one(s) for pins that you will use)
    GPIOA->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);
    
    // 2. Enable power on LED GPIO port
    GPIOA->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    
    delay_cycles(POWER_STARTUP_DELAY); // delay to enable GPIO to turn on and reset

    /* Code to initialize specific GPIO PIN */
    // PA0 is red led gpio
    uint32_t gpio_cfg = (IOMUX_PINCM_PC_CONNECTED | 0x00000001);
    
    GPIOA->DOUTSET31_0 = ALL_LEDS_MASK;
    GPIOA->DOESET31_0 = ALL_LEDS_MASK;

    //Outer Ring LEDs (Hours) 
    IOMUX->SECCFG.PINCM[IOMUX_PINCM1] = gpio_cfg; // PA0 (12)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM59] = gpio_cfg; // PA26 (1)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM54] = gpio_cfg; // PA24 (2)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM47] = gpio_cfg; // PA22 (3)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM40] = gpio_cfg; // PA18 (4)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM38] = gpio_cfg; // PA16 (5)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM36] = gpio_cfg; // PA14 (6)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM34] = gpio_cfg; // PA12 (7)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM21] = gpio_cfg; // PA10 (8)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM19] = gpio_cfg; // PA8 (9)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM11] = gpio_cfg; // PA6 (10)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM3] = gpio_cfg; // PA28 (11)

    // Inner Ring LEDs (Seconds) 
    IOMUX->SECCFG.PINCM[IOMUX_PINCM60] = gpio_cfg; // PA27 (12)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM55] = gpio_cfg; // PA25 (1)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM53] = gpio_cfg; // PA23 (2)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM46] = gpio_cfg; // PA21 (3)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM39] = gpio_cfg; // PA17 (4)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM37] = gpio_cfg; // PA15 (5)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM35] = gpio_cfg; // PA13 (6)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM22] = gpio_cfg; // PA11 (7)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM20] = gpio_cfg; // PA9 (8)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM14] = gpio_cfg; // PA7 (9)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM10] = gpio_cfg; // PA5 (10)
    IOMUX->SECCFG.PINCM[IOMUX_PINCM2] = gpio_cfg; // PA1 (11)
}   
