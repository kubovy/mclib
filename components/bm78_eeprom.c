/* 
 * File:   bm78_eeprom.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_eeprom.h"

#ifdef BM78_ENABLED

/* Commands */
//                                    TYPE   OCF   OGF   LEN  PARAM...
const uint8_t BM78_EEPROM_OPEN[4]  = {0x01, 0x03, 0x0c, 0x00};
const uint8_t BM78_EEPROM_CLEAR[5] = {0x01, 0x2d, 0xfc, 0x01, 0x08};
//                                    TYPE   OCF   OGF   LEN  AD_H  AD_L  SIZE ...
const uint8_t BM78_EEPROM_WRITE[7] = {0x01, 0x27, 0xfc, 0x00, 0x00, 0x00, 0x00};
const uint8_t BM78_EEPROM_READ[7]  = {0x01, 0x29, 0xfc, 0x03, 0x00, 0x00, 0x01};

struct {
    uint8_t timeout;
    uint8_t stage;
    uint8_t retries;
    uint8_t index;
} BM78EEPROM = { 0xFF, 0, 0, 0 };

Procedure_t BM78EEPROM_initializationStartedHandler;
Procedure_t BM78EEPROM_initializationSuccessHandler;
Procedure_t BM78EEPROM_initializatonFailedHandler;

void BM78EEPROM_initialize(void) {
    if (BM78EEPROM_initializationStartedHandler) BM78EEPROM_initializationStartedHandler();
    BM78_resetTo(BM78_MODE_TEST);
    //__delay_ms(1000);
    BM78EEPROM_initializing = true;
    BM78EEPROM.index = 0;
    BM78EEPROM.retries = 0;
    BM78EEPROM.stage = 0;
    BM78EEPROM.timeout = 200;
    BM78_openEEPROM();
}

void BM78EEPROM_setInitializationStartedHandler(Procedure_t handler) {
    BM78EEPROM_initializationStartedHandler = handler;
}

void BM78EEPROM_setInitializationSuccessHandler(Procedure_t handler) {
    BM78EEPROM_initializationSuccessHandler = handler;
}

void BM78EEPROM_setInitializationFailedHandler(Procedure_t handler) {
    BM78EEPROM_initializatonFailedHandler = handler;
}

void BM78EEPROM_testModeResponseHandler(BM78_Response_t response, uint8_t* data) {
    FlashPacket_t config;
    if (BM78EEPROM_initializing) switch (response.ISSC_Event.ogf) {
        case BM78_ISSC_OGF_COMMAND:
            switch (response.ISSC_Event.ocf) {
                case BM78_ISSC_OCF_OPEN:
#ifdef LCD_ADDRESS
                    printProgress("  Initializing BT   ", 1, BM78_CONFIGURATION_SIZE + 3);
#endif
                    //__delay_ms(500);
                    BM78EEPROM.retries = 0;
                    BM78EEPROM.stage = 1;
                    BM78EEPROM.timeout = 200;
                    BM78_clearEEPROM();  
                    break;
                default:
                    break;
            }
            break;
        case BM78_ISSC_OGF_OPERATION:
            switch (response.ISSC_Event.ocf) {
                case BM78_ISSC_OCF_WRITE:
                    config = BM78_configuration[BM78EEPROM.index];
                    //__delay_ms(500);
                    BM78EEPROM.retries = 0;
                    BM78EEPROM.stage = 2;
                    BM78EEPROM.timeout = 200;
                    BM78_readEEPROM(config.address, config.length);
                    break;
                case BM78_ISSC_OCF_READ:
                    config = BM78_configuration[BM78EEPROM.index];
                    bool equals = response.ISSC_ReadEvent.address == config.address
                            && response.ISSC_ReadEvent.data_length == config.length;

                    if (equals) for (uint8_t i = 0; i < response.ISSC_ReadEvent.data_length; i++) {
                        equals = equals && *(data + i) == config.data[i];
                    }

                    if (equals) {
                        BM78EEPROM.index++;
#ifdef LCD_ADDRESS
                        printProgress("  Initializing BT   ", 3 + BM78EEPROM.index, BM78_CONFIGURATION_SIZE + 3);
#endif
                        if (BM78EEPROM.index < BM78_CONFIGURATION_SIZE) {
                            config = BM78_configuration[BM78EEPROM.index];
                            //__delay_ms(500);
                            BM78EEPROM.retries = 0;
                            BM78EEPROM.stage = 2;
                            BM78EEPROM.timeout = 200;
                            BM78_readEEPROM(config.address, config.length);
                        } else {
                            BM78EEPROM_initializing = false;
                            BM78EEPROM.stage = 3;
                            BM78EEPROM.timeout = 0xFF;
                            BM78EEPROM.retries = 0;
                            BM78_resetTo(BM78_MODE_APP);
                            if (BM78EEPROM_initializationSuccessHandler) BM78EEPROM_initializationSuccessHandler();
                        }
                    } else {
                        //__delay_ms(500);
                        BM78EEPROM.retries = 0x80;
                        BM78EEPROM.stage = 2;
                        BM78EEPROM.timeout = 200;
                        BM78_writeEEPROM(config.address, config.length, config.data);
                    }
                    break;
                case BM78_ISSC_OCF_CLEAR:
#ifdef LCD_ADDRESS
                    printProgress("  Initializing BT   ", 2, BM78_CONFIGURATION_SIZE + 3);
#endif
                    config = BM78_configuration[BM78EEPROM.index];
                    //__delay_ms(500);
                    BM78EEPROM.retries = 0;
                    BM78EEPROM.stage = 2;
                    BM78EEPROM.timeout = 200;
                    BM78_readEEPROM(config.address, config.length);
                    break;
                default:
                    break;
            }
            break;
    }
}

void BM78EEPROM_testModeErrorHandler(BM78_Response_t response, uint8_t* data) {
    FlashPacket_t config;
    switch (response.ISSC_Event.ogf) {
        case BM78_ISSC_OGF_COMMAND:
            switch (response.ISSC_Event.ocf) {
                case BM78_ISSC_OCF_OPEN:
                    if (BM78EEPROM.stage == 0) {
                        BM78EEPROM.retries++;
                        BM78EEPROM.timeout = 200;
                        BM78_openEEPROM();
                    }
                    break;
                default:
                    break;
            }
        case BM78_ISSC_OGF_OPERATION:
            switch (response.ISSC_Event.ocf) {
                case BM78_ISSC_OCF_WRITE:
                    if (BM78EEPROM.stage == 2) {
                        BM78EEPROM.retries++;
                        BM78EEPROM.timeout = 200;
                        config = BM78_configuration[BM78EEPROM.index];
                        BM78_writeEEPROM(config.address, config.length, config.data);
                    }
                    break;
                case BM78_ISSC_OCF_READ:
                    if (BM78EEPROM.stage == 2) {
                        BM78EEPROM.retries++;
                        BM78EEPROM.timeout = 200;
                        config = BM78_configuration[BM78EEPROM.index];
                        BM78_readEEPROM(config.address, config.length);
                    }
                    break;
                case BM78_ISSC_OCF_CLEAR:
                    if (BM78EEPROM.stage == 1) {
                        BM78EEPROM.retries++;
                        BM78EEPROM.timeout = 200;
                        BM78_clearEEPROM();  
                    }
                    break;
                default:
                    break;
            }
        default:
            break;
    }
}

void BM78EEPROM_bm78TestModeCheck(void) {
    if (BM78.mode == BM78_MODE_TEST && BM78EEPROM_initializing && BM78EEPROM.timeout < 0xFF) {
        if (BM78EEPROM.timeout == 0) {
            BM78EEPROM.timeout = 0xFF;
            FlashPacket_t config = BM78_configuration[BM78EEPROM.index];

            //__delay_ms(500);
            if (BM78EEPROM.stage == 0 || (BM78EEPROM.retries & 0x7F) >= 3) {
                //BM78_resetTo(BM78_MODE_APP);
                //__delay_ms(1000);
                BM78_resetTo(BM78_MODE_TEST);
                //__delay_ms(1000);
                BM78_openEEPROM();
                BM78EEPROM.stage = 0;
                BM78EEPROM.retries = 0;
            } else if ((BM78EEPROM.retries & 0x7F) >= 10) {
                BM78EEPROM_initializing = false;
                BM78EEPROM.stage = 3;
                BM78EEPROM.timeout = 0xFF;
                BM78EEPROM.retries = 0;
                BM78_resetTo(BM78_MODE_APP);
                if (BM78EEPROM_initializatonFailedHandler) BM78EEPROM_initializatonFailedHandler();
            } else if (BM78EEPROM.stage == 1) {
                BM78_clearEEPROM();
            } else if (BM78EEPROM.retries >= 0x80) {
                BM78_writeEEPROM(config.address, config.length, config.data);
            } else {
                BM78_readEEPROM(config.address, config.length);
            }
            
            BM78EEPROM.timeout = 200;
            BM78EEPROM.retries++;
        } else {
            BM78EEPROM.timeout--;
        }
    }
}

#endif