/* 
 * File:   dht11.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "dht11.h"

#ifdef DHT11_PORT

typedef struct {
    uint8_t temp_decimal;
    uint8_t temp_integral;
    uint8_t rh_decimal;
    uint8_t rh_integral;
    uint8_t chksum;
} DHT11_Response;

Procedure_t DHT11_onStart;
Procedure_t DHT11_onFinished;

void DHT11_setOnStartHandler(Procedure_t onStart) {
    DHT11_onStart = onStart;
}

void DHT11_setOnFinishedHandler(Procedure_t onFinished) {
    DHT11_onFinished = onFinished;
}

DHT11_Response DHT11_read(void){
    bool raw[40];
    DHT11_Response result;
    result.chksum = 0;

    uint8_t byte = 0, count, i;
    unsigned int counter = 0;

    DHT11_TRIS = 0; // Configure PIN as output
    DHT11_LAT = 0;  // Sends 0 to the sensor
    __delay_ms(18); // Wait min 18ms
    DHT11_LAT = 1;  // Sends 1 to the sensor
    DHT11_TRIS = 1; // Configure PIN as input
    
    while(DHT11_PORT) { // Wait for falling edge on PIN
        counter++;
        __delay_us(1);
        if(counter == 40) {
            __delay_ms(1000);
            return result;
        }
    }

    while(!DHT11_PORT) { // Wait for rising edge on PIN
        counter++;
        __delay_us(1);
        if(counter==100) {
            __delay_ms(1000);
            return result;
        }
    }

    while(DHT11_PORT) { // Wait for falling edge PIN
        counter++;
        __delay_us(1);
        if(counter==180) {
            __delay_ms(1000);
            return result;
        }
    }

    for(i = 0; i < 40; i++) {
        while(!DHT11_PORT); // Wait for rising edge on PIN
        __delay_us(24);
        raw[i] = DHT11_PORT;
        count = 0;
        while(DHT11_PORT) { // Wait for falling edge PIN
            __delay_us(1);
            if (count++ == 50) break;
        }
    }

    for(i = 0; i < 40; i++) {
        if(raw[i]) {
            byte |= (1 << (7 - (i % 8)));  // Set bit (7-b) 
        } else {
            byte &= ~(1 << (7 - (i % 8))); // Clear bit (7-b)
        }
        if (i % 8 == 7) switch(i / 8) {
            case 0:
                result.rh_integral = byte;
                break;
            case 1:
                result.rh_decimal = byte;
                break;
            case 2:
                result.temp_integral = byte;
                break;
            case 3:
                result.temp_decimal = byte;
                break;
            case 4:
                result.chksum = byte;
                break;
        }
    }

    return result;
}

DHT11_Result DHT11_measure(void) {
    DHT11_Result result;
    DHT11_Response data;
    result.retries = 0;
    bool isValid;
    if (DHT11_onStart) DHT11_onStart();
    DHT11_measuring = true;

    DHT11_read(); // The sensor returns last value - ensure measurement here
    __delay_ms(50);
    do {
        if (result.retries > 0) __delay_ms(50);
        data = DHT11_read();
        isValid = (data.rh_integral + data.rh_decimal
                + data.temp_integral + data.temp_decimal)
                == data.chksum;
    } while(!isValid && result.retries++ < 5);
    
    DHT11_measuring = false;
    if (DHT11_onFinished) DHT11_onFinished();

    if(data.chksum != 0) {
        if(isValid) {
            result.temp = data.temp_integral * 100 + data.temp_decimal % 100;
            result.rh = data.rh_integral * 100 + data.rh_decimal % 100;
            result.status = DHT11_OK;
        } else {
            result.status = DHT11_ERR_CHKSUM;
        }
    } else {
        result.status = DHT11_ERR_NO_RESPONSE;
    }
    return result;
}

#endif
