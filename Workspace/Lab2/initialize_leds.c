#include "initialize_leds.h"
#include "delay.h"
#include <ti/devices/msp/msp.h>

#define POWER_STARTUP_DELAY (16)   // Small delay to allow GPIO power/reset to stabilize

void InitializeGPIO() {
    // Reset GPIOA module using the required unlock key and assert reset
    // KEY unlocks protected register, RESETSTKYCLR clears old reset flags, RESETASSERT resets hardware
    GPIOA->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

    // Enable power to GPIOA module (required before using GPIO pins)
    GPIOA->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    delay_cycles(16);   // short hardware settling delay

    // Reset GPIOB module
    GPIOB->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

    // Enable power to GPIOB module
    GPIOB->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    delay_cycles(16);

    delay_cycles(POWER_STARTUP_DELAY); // allow clocks and GPIO logic to fully stabilize

    /* Code to initialize specific GPIO PIN */
    
    // OUTER PINS (hour LEDs)

    // PA0 configuration
    // IOMUX PINCM selects GPIO function and connects pad to GPIO peripheral
    // 0x00000001 selects GPIO function mode
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM1)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);        // no bits set high initially
    GPIOA->DOESET31_0 = (0x00000001);         // bit 0 = PA0 output enable

    // PA26
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM59)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x04000000);         // bit 26 = 1 → enable PA26 as output
    
    // PA24
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM54)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x01000000);         // bit 24 mask
    
    // PA22
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM47)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00400000);         // bit 22 mask
    
    // PA18
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM40)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00040000);         // bit 18 mask
    
    // PA16
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM38)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00010000);         // bit 16 mask
    
    // PA14
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM36)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00004000);         // bit 14 mask
    
    // PA12
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM34)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00001000);         // bit 12 mask
    
    // PA10
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM21)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000400);         // bit 10 mask
    
    // PA8
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM19)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000100);         // bit 8 mask
    
    // PA6
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM11)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000040);         // bit 6 mask
    
    // PA28
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM3)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x10000000);         // bit 28 mask
    
    
    // INNER PINS (minute LEDs)

    // Each value is a bitmask enabling the corresponding GPIO pin as output

    // PA27
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM60)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x08000000);         // bit 27 mask

    // PA25
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM55)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x02000000);         // bit 25 mask
    
    // PA23
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM53)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00800000);         // bit 23 mask
    
    // PA21
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM46)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00200000);         // bit 21 mask
    
    // PA17
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM39)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00020000);         // bit 17 mask
    
    // PA15
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM37)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00008000);         // bit 15 mask
    
    // PA13
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM35)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00002000);         // bit 13 mask
    
    // PA11
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM22)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000800);         // bit 11 mask
    
    // PA9
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM20)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000200);         // bit 9 mask
    
    // PA7
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM14)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x0000080);          // bit 7 mask
    
    // PA5
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM10)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000020);         // bit 5 mask
    
    // PA1
    IOMUX->SECCFG.PINCM[(IOMUX_PINCM2)] = (IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001));
    GPIOA->DOUTSET31_0 = (0x00000000);
    GPIOA->DOESET31_0 = (0x00000002);         // bit 1 mask
}