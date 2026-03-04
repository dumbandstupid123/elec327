#include "state_machine_logic.h"
#include <ti/devices/msp/msp.h>

int GetNextState(int current_state)
{
    // Simply increment the state, and wrap back to 0 (OFF/Start) after 143
    if (current_state >= 143) {
        return 0; 
    } 
    else {
        return current_state + 1;
    }
}

int GetStateOutputGPIOA(int current_state) {
    // We define our pin maps inside the function
    static const uint32_t hourBits[] = {
        (1u<<0),  (1u<<26), (1u<<24), (1u<<22), (1u<<18), (1u<<16),
        (1u<<14), (1u<<12), (1u<<10), (1u<<8),  (1u<<6),  (1u<<28)
    };

    static const uint32_t secBits[] = {
        (1u<<27), (1u<<25), (1u<<23), (1u<<21), (1u<<17), (1u<<15), 
        (1u<<13), (1u<<11), (1u<<9),  (1u<<7),  (1u<<5),  (1u<<1)
    };

    // Calculate indices based on the state passed in
    int hourIdx = (current_state / 12);
    int secIdx  = (current_state % 12);

    // Default state: All pins HIGH (OFF for active-low)
    uint32_t output = 0xFFFFFFFF;

    // Outer Ring: Turn on all hours up to the current hour index
    for (int i = 0; i <= hourIdx; i++) {
        output &= ~(hourBits[i]);
    }

    // Inner Ring: Turn on all seconds up to the current second index
    output &= ~(secBits[secIdx]);

    return output;
}

int GetStateOutputGPIOB(int current_state) {
    return 0;
}