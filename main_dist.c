/* 
 * File:   main.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mclib/project.h"


void main(void) {
    SYSTEM_Initialize(); // initialize the device

    // When using interrupts, you need to set the Global and Peripheral
    // Interrupt Enable bits Use the following macros to:
    INTERRUPT_GlobalInterruptEnable();      // Enable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();   // Disable the Global Interrupts
    //INTERRUPT_PeripheralInterruptEnable();// Enable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable(); // Disable the Peripheral Interrupts
    
    while(1) {
        // TODO Implement main loop
    }
}
