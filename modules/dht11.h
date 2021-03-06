/* 
 * File:   dht11.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * DHT11 (Humidity and Temperature) sensor implementation.
 * 
 * Needs DHT11 pin to be defined on the microcontroller.
 */
#ifndef DHT11_H
#define	DHT11_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"

#ifdef DHT11_PORT

#include <stdbool.h>
#include <stdint.h>

#define DHT11_ERR_VALUE ((uint8_t) 255)
#define DHT11_OK ((uint8_t) 0)
#define DHT11_ERR_NO_RESPONSE ((uint8_t) 1)
#define DHT11_ERR_CHKSUM ((uint8_t) 2)

typedef struct {
    uint16_t temp;   // Temperature x 100
    uint16_t rh;     // Relative humidity x 100
    uint8_t status;  // Status
    uint8_t retries; // Number of retries
} DHT11_Result;

bool DHT11_measuring = false;

void DHT11_setOnStartHandler(Procedure_t onStart);

void DHT11_setOnFinishedHandler(Procedure_t onFinished);

/**
 * Requests a measurement from the DHT11 module.
 * 
 * @return The result of the measurement.
 */
DHT11_Result DHT11_measure(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* DHT11_H */

