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

#include <stdbool.h>
#include <stdint.h>
#include "../../config.h"

#ifdef DHT11_PORT
#define DHT11_ERR_VALUE ((uint8_t) 255)
#define DHT11_OK ((uint8_t) 0)
#define DHT11_ERR_NO_RESPONSE ((uint8_t) 1)
#define DHT11_ERR_CHKSUM ((uint8_t) 2)

typedef struct {
    uint8_t temp;    // Temperature
    uint8_t rh;      // Relative humidity
    uint8_t status;  // Status
    uint8_t retries; // Number of retries
} DHT11_Result;

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

