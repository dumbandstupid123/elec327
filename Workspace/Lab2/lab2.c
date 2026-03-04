#include <ti/devices/msp/msp.h>
#include "delay.h"
#include "initialize_leds.h"
#include "state_machine_logic.h"

int main(void)
{
    InitializeGPIO();

    /* Timer module and Sleep Mode Initialization */
    TIMG0->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W |
                           GPIO_RSTCTL_RESETSTKYCLR_CLR |
                           GPIO_RSTCTL_RESETASSERT_ASSERT);
    TIMG0->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    delay_cycles(16);

    TIMG0->CLKSEL = GPTIMER_CLKSEL_LFCLK_SEL_ENABLE;
    TIMG0->COUNTERREGS.CTRCTL = GPTIMER_CTRCTL_REPEAT_REPEAT_1;
    TIMG0->CPU_INT.IMASK |= GPTIMER_CPU_INT_IMASK_Z_SET;
    TIMG0->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;

    SYSCTL->SOCLOCK.PMODECFG = SYSCTL_PMODECFG_DSLEEP_STANDBY;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    SYSCTL->SOCLOCK.MCLKCFG |= SYSCTL_MCLKCFG_STOPCLKSTBY_ENABLE;

    int state_unused = 0; // State is stored internally

    /* ----------------- PWM Timer Configuration ----------------- */
    // LFCLK = 32,768 Hz
    // PWM_TICKS_PER_SECOND = 800 (from state_machine_logic.c)
    // LOAD = 32,768 / 800 ≈ 41
    TIMG0->COUNTERREGS.LOAD = 41;
    TIMG0->COUNTERREGS.CTRCTL |= (GPTIMER_CTRCTL_EN_ENABLED);
    NVIC_EnableIRQ(TIMG0_INT_IRQn);

    while (1) {
        int output = GetStateOutputGPIOA(state_unused);
        GPIOA->DOUT31_0 = output;

        GetNextState(state_unused);  // increments time and PWM tick
        __WFI(); // Sleep until timer interrupt
    }
}

// Timer interrupt (already wakes processor)
void TIMG0_IRQHandler(void)
{
    switch (TIMG0->CPU_INT.IIDX) {
        case GPTIMER_CPU_INT_IIDX_STAT_Z:
            break;
        default:
            break;
    }
}