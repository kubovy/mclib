/* 
 * File:   bm78.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78.h"

#ifdef BM78_ENABLED

/*
 * BUFFER
 * BM78_readEEPROM : BM78_COMMAND_W[7]                   (=7)
 * BM78_writeEEPROM: BM78_COMMAND_W[7] + data            (=7 + packet size)
 * BM78_data       : 0xAA + LEN[2] + CMD + data + CHKSUM (=5 + packet size)
 */

struct {
    uint8_t index;                           // Byte index in a message.
    BM78_Response_t response;                // Response.
    uint8_t buffer[BM78_DATA_PACKET_MAX_SIZE + 7];
} BM78_rx = {0};

struct {
    uint16_t idle;
    uint8_t missedStatusUpdate;
} BM78_counters = {0, 0};

union {
    struct {
        uint8_t stage  :4; // max 16 stages
        uint8_t attempt:3; // max 7 attempts
        bool keep      :1;
    };
} BM78_init = { 0, 0, false };

struct {
    uint8_t length;         // Transmission length: 0 = nothing to transmit
                            // (only used for transparent data transmission)
    uint8_t chksumExpected; // Expected checksum calculated before sending.
    uint8_t chksumReceived; // Received checksum by the peer.
    uint16_t timeout;       // Timeout countdown used by retry trigger.
    uint8_t retries;        // Number of transparent transmissions retries
    uint8_t buffer[BM78_DATA_PACKET_MAX_SIZE + 7];
    uint8_t data[BM78_DATA_PACKET_MAX_SIZE + 1]; // Only for transparent data
} BM78_tx = {0, 0x00, 0xFF, 0, 0xFF};

BM78_EventState_t BM78_state = BM78_STATE_IDLE;

/* Commands */
//                                        TYPE   OCF   OGF   LEN  PARAM
const uint8_t BM78_CMD_EEPROM_OPEN[4]  = {0x01, 0x03, 0x0c, 0x00};
const uint8_t BM78_CMD_EEPROM_CLEAR[5] = {0x01, 0x2d, 0xfc, 0x01, 0x08};
//                                        TYPE   OCF   OGF   LEN  AD_H  AD_L  SIZE
const uint8_t BM78_CMD_EEPROM_WRITE[7] = {0x01, 0x27, 0xfc, 0x00, 0x00, 0x00, 0x00};
const uint8_t BM78_CMD_EEPROM_READ[7]  = {0x01, 0x29, 0xfc, 0x03, 0x00, 0x00, 0x01};

BM78_SetupAttemptHandler_t BM78_setupAttemptHandler;
BM78_SetupHandler_t BM78_setupHandler;
BM78_EventHandler_t BM78_appModeEventHandler;
BM78_EventHandler_t BM78_testModeEventHandler;
BM78_DataHandler_t BM78_transparentDataHandler;
Procedure_t BM78_messageSentHandler;
BM78_EventHandler_t BM78_appModeErrorHandler;
BM78_EventHandler_t BM78_testModeErrorHandler;

void BM78_retryInitialization(void) {
    switch (BM78_init.stage) {
        case 1:
            printStatus("      ..            ");
            BM78_execute(BM78_CMD_READ_LOCAL_INFORMATION, 0);
            break;
        case 2:
            printStatus("      ...           ");
            BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
            break;
        case 3:
            printStatus("      ..*           ");
            BM78_write(BM78_CMD_WRITE_DEVICE_NAME, BM78_EEPROM_STORE, strlen(BM78.deviceName), (uint8_t *) BM78.deviceName);
            break;
        case 4:
            printStatus("      ..=           ");
            BM78_GetAdvData();
            BM78_write(BM78_CMD_WRITE_ADV_DATA, BM78_EEPROM_STORE, 22, BM78_advData);
            break;
        case 5:
            printStatus("      ....          ");
            BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
            break;
        case 6:
            printStatus("      ...*          ");
            BM78_execute(BM78_CMD_WRITE_PAIRING_MODE_SETTING, 2, BM78_EEPROM_STORE, BM78.pairingMode);
            break;
        case 7:
            printStatus("      .....         ");
            BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
            break;
        case 8:
            printStatus("      ....*         ");
            BM78_write(BM78_CMD_WRITE_PIN_CODE, BM78_EEPROM_STORE, strlen(BM78.pin), (uint8_t *) BM78.pin);
            break;
        case 9:
            printStatus("      ......        ");
            BM78_execute(BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO, 0);
            break;
        case 10:
            printStatus("      .......       ");
            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
            break;
        case 11:
            printStatus("      .......       ");
            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER); //BM78_STANDBY_MODE_ENTER_ONLY_TRUSTED);
            break;
        case 12:
            printStatus("                    ");
            BM78_init.stage = 0;
            BM78_init.attempt = 0;
            BM78.mode = BM78_MODE_APP;
            if (BM78_setupHandler) {
                BM78_setupHandler(BM78.deviceName, BM78.pin);
            }
            break;
    }
}

void BM78_initialize(
               BM78_SetupAttemptHandler_t setupAttemptHandler,
               BM78_SetupHandler_t setupHandler,
               BM78_EventHandler_t appModeEventHandler,
               BM78_EventHandler_t testModeResponseHandler,
               BM78_DataHandler_t transparentDataHandler,
               Procedure_t messageSentHandler,
               BM78_EventHandler_t appModeErrorHandler,
               BM78_EventHandler_t testModeErrorHandler) {
    BM78_setupAttemptHandler = setupAttemptHandler;
    BM78_setupHandler = setupHandler;
    BM78_appModeEventHandler = appModeEventHandler;
    BM78_testModeEventHandler = testModeResponseHandler;
    BM78_transparentDataHandler = transparentDataHandler;
    BM78_messageSentHandler = messageSentHandler;
    BM78_appModeErrorHandler = appModeErrorHandler;
    BM78_testModeErrorHandler = testModeErrorHandler;
    BM78_setup(true);
}

void BM78_clear() {
    BM78_rx.index = 0;
    BM78_state = BM78_STATE_IDLE;
    BM78_tx.length = 0;
    BM78_tx.chksumExpected = 0x00;
    BM78_tx.chksumReceived = 0xFF;
}

void BM78_power(bool on) {
    BM78_clear();
    if (BM78_SW_BTN_PORT != on) {
        BM78_SW_BTN_LAT = on;
        __delay_ms(500);
    }
}

void BM78_reset(void) {
    BM78_clear();
    BM78_RST_N_SetLow(); // Set the reset pin low
    __delay_ms(2); // A reset pulse must be greater than 1ms
    BM78_RST_N_SetHigh(); // Pull the reset pin back up    
    __delay_ms(354); // 350 + 4 ms
    __delay_ms(300); // This may need to be adjusted.
}

void BM78_resetTo(BM78_Mode_t mode) {
    switch (mode) {
        case BM78_MODE_INIT:
        case BM78_MODE_APP:
            BM78.mode = BM78_MODE_INIT;
            BM78_P2_0_SetHigh();
            BM78_P2_4_SetHigh();
            BM78_EAN_SetLow();
            break;
        case BM78_MODE_TEST:
            BM78.mode = BM78_MODE_TEST;
            BM78_P2_0_SetLow();
            BM78_P2_4_SetHigh();
            BM78_EAN_SetLow();
            break;
    }
    BM78_reset();
}

void BM78_setup(bool keep) {
    BM78_init.keep = keep;
    switch (BM78.mode) {
        case BM78_MODE_INIT:
            if (BM78_init.attempt < 7) {
                if (BM78_setupAttemptHandler) BM78_setupAttemptHandler(
                        BM78_init.attempt, BM78_init.stage);
                
                BM78_init.attempt++;
                while(UART_isRXReady()) UART_read();
                if (BM78_init.attempt == 3) BM78_reset(); // Soft reset
                BM78_state = BM78_STATE_IDLE;
                BM78_retryInitialization();
                break;
            }
            // no break here
        case BM78_MODE_APP:
            if (BM78_setupAttemptHandler) BM78_setupAttemptHandler(0, 0);
            printStatus("      .             ");
            while(UART_isRXReady()) UART_read();
            BM78_state = BM78_STATE_IDLE;
            BM78.mode = BM78_MODE_INIT;
            BM78_counters.idle = 0;
            BM78_counters.missedStatusUpdate = 0;
            BM78_init.stage = 0;
            BM78_init.attempt = 0;
            BM78_tx.length = 0; // Abort ongoing transmission
            BM78_power(true);
            BM78_reset();
            break;
        case BM78_MODE_TEST:
            break;
    }
}

void BM78_loadTXBuffer(uint8_t length, uint8_t *data) {
    for (uint8_t i = 0; i < length; i++) {
        if (i < sizeof(BM78_tx.buffer)) {
            BM78_tx.buffer[i] = *(data + i);
        }
    }
}

void BM78_sendPacket(uint8_t length, uint8_t *data) {
    //if (clearRX) while (UART1_is_rx_ready()) UART1_Read(); // Clear UART RX queue.
    BM78_counters.idle = 0; // Reset idle counter

    for (uint8_t i = 0; i < length; i++) { // Send the command bits, along with the parameters
        while (!UART_isTXReady()); // Wait till we can start sending.
        UART_write(*(data + i)); // Store each byte in the storePacket into the UART write buffer
        while (!UART_isTXDone()); // Wait until UART TX is done.
    }
    BM78_counters.idle = 0; // Reset idle counter
    BM78_state = BM78_STATE_IDLE;
}

inline void BM78_openEEPROM(void) {
    BM78_sendPacket(sizeof (BM78_CMD_EEPROM_OPEN), (uint8_t *) BM78_CMD_EEPROM_OPEN);
}

inline void BM78_clearEEPROM(void) {
    BM78_sendPacket(sizeof (BM78_CMD_EEPROM_CLEAR), (uint8_t *) BM78_CMD_EEPROM_CLEAR);
}

void BM78_readEEPROM(uint16_t address, uint8_t length) {
    BM78_loadTXBuffer(sizeof (BM78_CMD_EEPROM_READ), (uint8_t *) BM78_CMD_EEPROM_READ);
    BM78_tx.buffer[4] = address >> 8;
    BM78_tx.buffer[5] = address & 0xFF;
    BM78_tx.buffer[6] = length;
    BM78_sendPacket(sizeof (BM78_CMD_EEPROM_READ), BM78_tx.buffer);
}

void BM78_writeEEPROM(uint16_t address, uint8_t length, uint8_t *data) {
    BM78_loadTXBuffer(sizeof (BM78_CMD_EEPROM_WRITE), (uint8_t *) BM78_CMD_EEPROM_WRITE);
    BM78_tx.buffer[3] = length + 3;
    BM78_tx.buffer[4] = address >> 8;
    BM78_tx.buffer[5] = address & 0xFF;
    BM78_tx.buffer[6] = length;
    for (uint8_t i = 0; i < length; i++) {
        if ((i + 7) < sizeof(BM78_tx.buffer)) {
            BM78_tx.buffer[i + 7] = *(data + i);
        }
    }
    BM78_sendPacket(sizeof (BM78_CMD_EEPROM_WRITE) + length, BM78_tx.buffer);
}

void BM78_checkState(void) {
    switch (BM78.mode) {
        case BM78_MODE_INIT: // In Initialization mode retry setting up the BM78
                             // in a defined interval
            if (BM78_counters.idle > (BM78_INIT_CMD_TIMEOUT / BM78_TRIGGER_PERIOD)) {
                BM78_counters.idle = 0; // Reset idle counter.
                BM78_setup(BM78_init.keep);
            }
            break;
        case BM78_MODE_APP: // In Application mode status is refreshed in 
                            // a defined interval. In case a certain consequent
                            // number of refresh attempts will be missed, the
                            // device will refresh itself.
            if (BM78_counters.idle > (BM78_STATUS_REFRESH_INTERVAL / BM78_TRIGGER_PERIOD)) {
                BM78_counters.idle = 0; // Reset idle counter.
                BM78_counters.missedStatusUpdate++; // Status update counter increment.
                if (BM78_counters.missedStatusUpdate >= BM78_STATUS_MISS_MAX_COUNT) {
                    BM78_counters.missedStatusUpdate = 0; // Reset missed status update counter.
                    BM78_reset(); // Reset device.
                } else {
                    BM78_execute(BM78_CMD_READ_STATUS, 0); // Request status update.
                }
            }
            break;
        case BM78_MODE_TEST: // In Test mode nothing is done.
            break;
    }
    BM78_counters.idle++; // Idle counter increment.
}

void BM78_GetAdvData(void) {
    BM78_advData[0] = 0x02; // Size A
    BM78_advData[1] = 0x01; // Flag
    BM78_advData[2] = 0x02; // Dual-Mode
    BM78_advData[3] = strlen(BM78.deviceName); // Size B
    BM78_advData[4] = 0x09;
    for (uint8_t i = 0; i < 16; i++) {
        BM78_advData[i + 5] = BM78.deviceName[i];
        BM78_advData[i + 6] = 0x00;
    }
}

void BM78_storePairedDevices(uint8_t *data) {
    //<-- 0001 0C
    //                  C  I1 P1 MAC1         I2 P2 MAC2 
    //--> 0014 80 0C 00 02 00 01 123456789ABC 01 02 123456789ABC
    BM78.pairedDevicesCount = *data;
    for (uint8_t i = 0; i < BM78.pairedDevicesCount; i++) {
        BM78.pairedDevices[i].index = *(data + i * 8 + 1);
        BM78.pairedDevices[i].priority = *(data + i * 8 + 2);
        for (uint8_t j = 0; j < 6; j++) {
            BM78.pairedDevices[i].address[j] = *(data + i * 8 + 3 + j);
        }
    }
}

void BM78_AsyncEventResponse() {
    uint8_t chksum;
    switch (BM78.mode) {
        /*
         * Initialization Mode
         */
        case BM78_MODE_INIT:
            switch (BM78_rx.response.op_code) {
                // On disconnection enter stand-by mode if this mode is enforced
                // and ...
                case BM78_EVENT_DISCONNECTION_COMPLETE:
                    if (BM78.enforceState) {
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                    }
                // ... on connection, both LE or SPP, abort ongoing transmission.
                case BM78_EVENT_LE_CONNECTION_COMPLETE:
                case BM78_EVENT_SPP_CONNECTION_COMPLETE:
                    BM78_tx.length = 0; // Abort ongoing transmission.
                    break;
                case BM78_EVENT_COMMAND_COMPLETE:
                    // Initialization flow:
                    if (BM78_rx.response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                        switch (BM78_rx.response.CommandComplete_0x80.command) {
                            case BM78_CMD_READ_LOCAL_INFORMATION:     // Stage 1
                                BM78_init.attempt = 0;
                                BM78_init.stage = 2;
                                BM78_retryInitialization();
                                break;
                            case BM78_CMD_READ_DEVICE_NAME:           // Stage 2
                                BM78_init.attempt = 0;
#ifdef BM78_SETUP_ENABLED
                                if (BM78_init.keep) {
#endif
                                    BM78_init.stage = 5;
                                    strcpy(BM78.deviceName, (char *) BM78_rx.buffer, BM78_rx.response.CommandComplete_0x80.length - 3);
                                    BM78.deviceName[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
#ifdef BM78_SETUP_ENABLED
                                } else if (strcmp(BM78.deviceName, (char *) BM78_rx.buffer, BM78_rx.response.CommandComplete_0x80.length - 3)) {
                                    BM78_init.stage = 5;
                                    BM78.deviceName[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
                                } else {
                                    BM78_init.stage = 3;
                                    BM78_retryInitialization();
                                }
                                break;
                            case BM78_CMD_WRITE_DEVICE_NAME:          // Stage 3
                                BM78_init.attempt = 0;
                                BM78_init.stage = 4;
                                BM78_retryInitialization();
                                break;
                            case BM78_CMD_WRITE_ADV_DATA:             // Stage 4
                                BM78_init.attempt = 0;
                                BM78_init.stage = 2;
                                BM78_retryInitialization();
#endif
                                break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:  // Stage 5
                                BM78_init.attempt = 0;
#ifdef BM78_SETUP_ENABLED
                                if (BM78_init.keep) {
#endif
                                    BM78_init.stage = 7;
                                    BM78.pairingMode = BM78_rx.buffer[0];
                                    BM78_retryInitialization();
#ifdef BM78_SETUP_ENABLED
                                } else if (BM78.pairingMode == BM78_rx.buffer[0]) {
                                    BM78_init.stage = 7;
                                    BM78_retryInitialization();
                                } else {
                                    BM78_init.stage = 6;
                                    BM78_retryInitialization();
                                }
                                break;
                            case BM78_CMD_WRITE_PAIRING_MODE_SETTING:// Stage 6
                                BM78_init.attempt = 0;
                                BM78_init.stage = 5;
#endif
                                break;
                            case BM78_CMD_READ_PIN_CODE:              // Stage 7
                                BM78_init.attempt = 0;
#ifdef BM78_SETUP_ENABLED
                                if (BM78_init.keep) {
#endif
                                    BM78_init.stage = 9;
                                    strcpy(BM78.pin, (char *) BM78_rx.buffer, BM78_rx.response.CommandComplete_0x80.length - 3);
                                    BM78.pin[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
#ifdef BM78_SETUP_ENABLED
                                } else if (strcmp(BM78.pin, (char *) BM78_rx.buffer, BM78_rx.response.CommandComplete_0x80.length - 3)) {
                                    BM78_init.stage = 9;
                                    BM78.pin[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
                                } else {
                                    BM78_init.stage = 8;
                                    BM78_retryInitialization();
                                }
                                break;
                            case BM78_CMD_WRITE_PIN_CODE:             // Stage 8
                                BM78_init.attempt = 0;
                                BM78_init.stage = 7;
                                BM78_retryInitialization();
#endif
                                break;
                            case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:// Stage 9
                                BM78_init.attempt = 0;
                                BM78_storePairedDevices(BM78_rx.buffer);
                                if (BM78.status == BM78_STATUS_STANDBY_MODE) {
                                    BM78_init.stage = 12;
                                    BM78_retryInitialization();
                                } else {
                                    if (BM78.pairedDevicesCount == 0) {
                                        BM78_init.stage = 10;
                                        BM78_retryInitialization();
                                    } else {
                                        BM78_init.stage = 11;
                                        BM78_retryInitialization();
                                    }
                                }
                                break;
                            case BM78_CMD_INVISIBLE_SETTING:          // Stage X
                                BM78_init.attempt = 0;
                                BM78_init.stage = 12;
                                BM78_retryInitialization();
                                break;
                            default:
                                break;
                        }
                    } else { // Repeat last initialization command on failure.
                        switch (BM78_rx.response.CommandComplete_0x80.command) {
                            case BM78_CMD_INVISIBLE_SETTING:
                                BM78_execute(BM78_CMD_READ_STATUS, 0);
                                break;
                            //case BM78_CMD_READ_LOCAL_INFORMATION:
                            //case BM78_CMD_READ_DEVICE_NAME:
                            //case BM78_CMD_WRITE_DEVICE_NAME:
                            //case BM78_CMD_WRITE_ADV_DATA:
                            //case BM78_CMD_READ_PAIRING_MODE_SETTING:
                            //case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                            //case BM78_CMD_READ_PIN_CODE:
                            //case BM78_CMD_WRITE_PIN_CODE:
                            //case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                            default:
                                BM78_retryInitialization();
                                break;
                        }
                    }
                    break;
                case BM78_EVENT_BM77_STATUS_REPORT: // Manual mode
                    BM78.status = BM78_rx.response.StatusReport_0x81.status;
                    switch (BM78.status) {
                        case BM78_STATUS_IDLE_MODE:
                            if (BM78_init.stage == 0) BM78_init.stage = 1;
                            BM78_retryInitialization();
                            break;
                        case BM78_STATUS_STANDBY_MODE:
                            if (BM78_init.stage >= 10) BM78_init.stage = 12;
                            BM78_retryInitialization();
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
            break;
        /*
         * Application Mode
         */
        case BM78_MODE_APP:
            if (BM78_rx.response.op_code != BM78_EVENT_COMMAND_COMPLETE 
                    || BM78_rx.response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                switch (BM78_rx.response.op_code) {
                    // On disconnection enter stand-by mode if this mode is
                    // enfoced and ...
                    // Enforcing status is done by BM78_checkState
                    //case BM78_EVENT_DISCONNECTION_COMPLETE:
                    //    BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                    case BM78_EVENT_LE_CONNECTION_COMPLETE:
                    case BM78_EVENT_SPP_CONNECTION_COMPLETE:
                        BM78_tx.length = 0; // Abort ongoing transmission.
                        break;
                    case BM78_EVENT_COMMAND_COMPLETE:
                        switch (BM78_rx.response.CommandComplete_0x80.command) {
                            case BM78_CMD_READ_DEVICE_NAME:
                                strcpy(BM78.deviceName, (char *) BM78_rx.buffer, BM78_rx.response.CommandComplete_0x80.length - 3);
                                BM78.deviceName[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                break;
                            //case BM78_CMD_WRITE_DEVICE_NAME:
                            //    BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                            //    break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:
                                BM78.pairingMode = *BM78_rx.buffer;
                                break;
                            case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                                BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                                break;
                            case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                                BM78_storePairedDevices(BM78_rx.buffer);
                                break;
                            case BM78_CMD_READ_PIN_CODE:
                                strcpy(BM78.pin, (char *) BM78_rx.buffer, BM78_rx.response.CommandComplete_0x80.length - 3);
                                BM78.pin[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                break;
                            case BM78_CMD_WRITE_PIN_CODE:
                                // Re-read PIN code after writing PIN code.
                                BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                                break;
                            // Enforcing status is done by BM78_checkState
                            //case BM78_CMD_DISCONNECT:
                            //    if (BM78.enforceState) BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                            //    break;
                            default:
                                break;
                        }
                        break;
                    case BM78_EVENT_BM77_STATUS_REPORT:
                        BM78_counters.missedStatusUpdate = 0; // Reset missed status update counter.
                        BM78.status = BM78_rx.response.StatusReport_0x81.status;
                        switch (BM78.status) {
                            case BM78_STATUS_STANDBY_MODE:
                                if (BM78.enforceState == BM78_STANDBY_MODE_LEAVE) BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                                // No break here
                            case BM78_STATUS_LINK_BACK_MODE:
                            case BM78_STATUS_LE_CONNECTED_MODE:
                                BM78_tx.length = 0; // Abort ongoing transmission.
                                break;
                            case BM78_STATUS_SPP_CONNECTED_MODE:
                                break;
                            case BM78_STATUS_IDLE_MODE:
                                if (BM78.enforceState != BM78_STANDBY_MODE_LEAVE) BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                                break;
                            //case BM78_STATUS_POWER_ON:
                            //case BM78_STATUS_PAGE_MODE:
                            //case BM78_STATUS_SHUTDOWN_MODE:
                            default:
                                break;
                        }
                        break;
                    case BM78_EVENT_RECEIVED_TRANSPARENT_DATA:
                    case BM78_EVENT_RECEIVED_SPP_DATA:
                        chksum = 0; // Calculated checksum
                        for (uint8_t i = 1; i < BM78_rx.response.ReceivedTransparentData_0x9A.length - 2; i++) {
                            chksum = chksum + BM78_rx.buffer[i];
                        }
                        if (chksum == BM78_rx.buffer[0]) { // Compare calculated with received checksum
                            switch (BM78_rx.buffer[1]) {
                                case BM78_MESSAGE_KIND_CRC:
                                    if (BM78_rx.response.ReceivedTransparentData_0x9A.length == 5) {
                                        BM78_tx.chksumReceived = BM78_rx.buffer[2];
                                    }
                                    break;
                                case BM78_MESSAGE_KIND_IDD:
                                    // Do nothing, just ping.
                                    break;
                                default:
                                    // Shift data in the array one up to reuse it
                                    for (uint8_t i = 1; i < BM78_rx.response.ReceivedTransparentData_0x9A.length - 2; i++) {
                                        if ((i - 1) < BM78_DATA_PACKET_MAX_SIZE) {
                                            BM78_rx.buffer[i - 1] = BM78_rx.buffer[i];
                                        }
                                    }

                                    if (BM78_transparentDataHandler) {
                                        BM78_transparentDataHandler(BM78_rx.response.ReceivedTransparentData_0x9A.length - 3, BM78_rx.buffer);
                                    }

                                    // Shift data in the array one down to rebuild original data
                                    for (uint8_t i = BM78_rx.response.ReceivedTransparentData_0x9A.length - 3; i > 0; i--) {
                                        if (i < BM78_DATA_PACKET_MAX_SIZE) {
                                            BM78_rx.buffer[i] = BM78_rx.buffer[i - 1];
                                        }
                                    }
                                    BM78_rx.buffer[0] = chksum; // reconstruct 1st byte

                                    BM78_execute(BM78_CMD_SEND_TRANSPARENT_DATA, 4, 0x00, chksum, BM78_MESSAGE_KIND_CRC, chksum);
                                    break;
                            }
                        }
                        break;
                    default:
                        break;
                }

               if (BM78_appModeEventHandler) BM78_appModeEventHandler(BM78_rx.response, BM78_rx.buffer);

            } else {
                if (BM78_rx.response.op_code == BM78_EVENT_COMMAND_COMPLETE) {
                    switch (BM78_rx.response.CommandComplete_0x80.status) {
                        // Read status is done by BM78_checkState periodically
                        //case BM78_ERR_INVALID_COMMAND_PARAMETERS:
                        //    if (BM78_rx.response.CommandComplete_0x80.command == BM78_CMD_INVISIBLE_SETTING) {
                        //        BM78_execute(BM78_CMD_READ_STATUS, 0);
                        //    }
                        //    break;
                        case BM78_ERR_COMMAND_DISALLOWED:
                            if (BM78_rx.response.CommandComplete_0x80.command == BM78_CMD_DISCONNECT) {
                                BM78_reset();
                            //} else {
                            // Read status is done by BM78_checkState periodically
                            //    BM78_execute(BM78_CMD_READ_STATUS, 0);
                            }
                            break;
                        default:
                            break;
                    }
                }
                if (BM78_appModeErrorHandler) BM78_appModeErrorHandler(BM78_rx.response, BM78_rx.buffer);
            }
            break;
        case BM78_MODE_TEST:
            break;
    }
}

inline void BM78_commandPrepareBuffer(uint8_t command, uint8_t length) {
    BM78_counters.idle = 0; // Reset idle counter
    //BM78_state = BM78_STATE_SENDING;
    BM78_tx.buffer[0] = 0xAA; // Sync word.
    BM78_tx.buffer[1] = (length + 1) >> 8; // Length high byte.
    BM78_tx.buffer[2] = (length + 1) & 0x00FF; // Length low byte.
    BM78_tx.buffer[3] = command; // Add command
}

inline uint8_t BM78_commandCalculateChecksum(uint8_t length) {
    uint8_t chksum = 0;
    // Add bytes 1-3 (inclusive) to the checksum.
    for (uint8_t i = 1; i < (length + 4); i++) {
        chksum += BM78_tx.buffer[i];
    }
    return chksum;
}

inline void BM78_commandCommit(uint8_t length) {
    BM78_sendPacket(length + 5, BM78_tx.buffer); // Send
    BM78_counters.idle = 0; // Reset idle counter
}

void BM78_write(uint8_t command, uint8_t store, uint8_t length, uint8_t *value) {
    BM78_commandPrepareBuffer(command, length + 1);
    BM78_tx.buffer[4] = store;
    for (uint8_t i = 0; i < length; i++) {
        BM78_tx.buffer[i + 5] = *(value + i);
    }
    BM78_tx.buffer[length + 5] = 0xFF - BM78_commandCalculateChecksum(length + 1) + 1;
    BM78_commandCommit(length + 1);
}

void BM78_execute(uint8_t command, uint8_t length, ...) {
    va_list argp;
    va_start(argp, length);
    
    BM78_commandPrepareBuffer(command, length);
    for (uint8_t i = 0; i < length; i++) {
        BM78_tx.buffer[i + 4] = va_arg(argp, uint8_t);
    }
    va_end(argp);

    BM78_tx.buffer[length + 4] = 0xFF - BM78_commandCalculateChecksum(length) + 1;
    BM78_commandCommit(length);
}

void BM78_data(uint8_t command, uint8_t length, uint8_t *data) {
    BM78_commandPrepareBuffer(command, length);
    for (uint8_t i = 0; i < length; i++) {
        BM78_tx.buffer[i + 4] = *(data + i);
    }
    BM78_tx.buffer[length + 4] = 0xFF - BM78_commandCalculateChecksum(length) + 1;
    BM78_commandCommit(length);
}

bool BM78_awatingConfirmation(void) {
    return BM78_tx.length > 0;
}   

void BM78_retryTrigger(void) {
    if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        BM78_tx.length = 0; // Cancel transmission
    }
    if (BM78_tx.length > 0 && BM78_isChecksumCorrect()) {
        BM78_tx.length = 0;
        if (BM78_messageSentHandler) BM78_messageSentHandler();
    } else if (BM78_tx.length > 0 && !BM78_isChecksumCorrect() && BM78_tx.timeout == 0) {
        if (BM78_tx.retries == 0) {
            BM78_tx.retries = BM78_NO_RETRY_LIMIT;
            BM78_tx.length = 0; // Cancel transmission
            if (BM78_messageSentHandler) BM78_messageSentHandler();
            return;
        } else if (BM78_tx.retries < BM78_NO_RETRY_LIMIT) BM78_tx.retries--;
        BM78_tx.timeout = BM78_RESEND_TIMEOUT / BM78_TRIGGER_PERIOD;
        BM78_commandPrepareBuffer(BM78_CMD_SEND_TRANSPARENT_DATA, BM78_tx.length + 1);
        BM78_tx.buffer[4] = 0x00; // Reserved by BM78
        
        // (Re)calculate Transparent Checksum
        BM78_tx.chksumExpected = 0x00;
        for (uint8_t i = 1; i < BM78_tx.length; i++) {
            BM78_tx.chksumExpected = BM78_tx.chksumExpected + BM78_tx.data[i];
        }
        BM78_tx.data[0] = BM78_tx.chksumExpected;
        BM78_tx.chksumReceived = BM78_tx.chksumExpected == 0x00 ? 0xFF : 0x00;
        
        // Fill TX buffer
        for (uint8_t i = 0; i < BM78_tx.length; i++) {
            BM78_tx.buffer[i + 5] = BM78_tx.data[i];
        }
        BM78_tx.buffer[BM78_tx.length + 5] = 0xFF - BM78_commandCalculateChecksum(BM78_tx.length + 1) + 1;
        BM78_commandCommit(BM78_tx.length + 1);
    } else if (BM78_tx.length == 0 && BM78_tx.timeout == 0) {
        // Periodically try message sent handler if someone has something to send
        BM78_tx.timeout = BM78_RESEND_TIMEOUT / BM78_TRIGGER_PERIOD;
        if (BM78_messageSentHandler) BM78_messageSentHandler();
    } else if (BM78_tx.timeout > 0) {
        BM78_tx.timeout--;
    }
}

inline void BM78_addDataByte(uint8_t position, uint8_t byte) {
    if (BM78_tx.length == 0) BM78_tx.data[position + 1] = byte;
}

inline void BM78_addDataByte2(uint8_t position, uint16_t word) {
    if (BM78_tx.length == 0) {
        BM78_tx.data[position + 1] = word >> 8;
        BM78_tx.data[position + 2] = word & 0xFF;
    }
}

inline void BM78_addDataBytes(uint8_t position, uint8_t length, uint8_t *data) {
    if (BM78_tx.length == 0)  for (uint8_t i = 0; i < length; i++) {
        BM78_tx.data[position + i + 1] = *(data + i);
    }
}

bool BM78_commitData(uint8_t length, uint8_t maxRetries) {
    if (BM78_tx.length == 0) {
        BM78_tx.length = length + 1;
        BM78_tx.timeout = 0; // Send right-away
        BM78_tx.retries = maxRetries;
        BM78_resetChecksum();
        //BM78_retryTrigger(); this will be triggered by a timer.
        return true;
    }
    return false;
}

void BM78_transmitData(uint8_t length, uint8_t *data, uint8_t maxRetries) {
    if (BM78_tx.length == 0) {
        for (uint8_t i = 0; i < length; i++) {
            BM78_addDataByte(i, *(data + i));
        }
        BM78_commitData(length, maxRetries);
    }
}

void BM78_resetChecksum(void) {
    BM78_tx.chksumExpected = 0x00;
    BM78_tx.chksumReceived = 0xFF;
}

bool BM78_isChecksumCorrect(void) {
    return BM78_tx.chksumExpected == BM78_tx.chksumReceived;
}

void BM78_processByteInAppMode(uint8_t byte) {
    switch (BM78_state) {
        case BM78_STATE_IDLE:
            if (byte == 0xAA) { // SYNC WORD
                BM78_rx.response.checksum_calculated = 0;
                BM78_state = BM78_EVENT_STATE_LENGTH_HIGH;
            }
            break;
        case BM78_EVENT_STATE_LENGTH_HIGH:
            // Ignoring lengthHigh byte - max length: 255
            BM78_rx.response.length = (byte << 8);
            BM78_rx.response.checksum_calculated += byte;
            BM78_state = BM78_EVENT_STATE_LENGTH_LOW;
            break;
        case BM78_EVENT_STATE_LENGTH_LOW:
            BM78_rx.response.length |= (byte & 0x00FF);
            BM78_rx.response.checksum_calculated += byte;
            //BM78_rx.index = 0;
            BM78_state = BM78_EVENT_STATE_OP_CODE;
            break;
        case BM78_EVENT_STATE_OP_CODE:
            BM78_rx.response.op_code = byte;
            BM78_rx.response.checksum_calculated += byte;
            //BM78_rx.index++;
            BM78_rx.index = 1;
            BM78_state = BM78_EVENT_STATE_ADDITIONAL;
            break;
        case BM78_EVENT_STATE_ADDITIONAL:
            if (BM78_rx.index < BM78_rx.response.length) {
                switch (BM78_rx.response.op_code) {
#ifdef BM78_ADVANCED_PAIRING
                    case BM78_EVENT_PASSKEY_ENTRY_REQ:
                        //BM78_rx.response.PasskeyEntryReq_0x60
                        break;
#endif
                    case BM78_EVENT_PAIRING_COMPLETE:
                        BM78_rx.response.PairingComplete_0x61.result = byte;
                        break;
#ifdef BM78_ADVANCED_PAIRING
                    case BM78_EVENT_PASSKEY_DISPLAY_YES_NO_REQ:
                        BM78_rx.response.PasskeyDisplayYesNoReq_0x62.passkey[BM78_rx.index - 1] = byte;
                        break;
#endif
                        // GAP Events
                    case BM78_EVENT_LE_CONNECTION_COMPLETE:
                        BM78.status = BM78_STATUS_LE_CONNECTED_MODE;
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.LEConnectionComplete_0x71.status = byte;
                        } else if (BM78_rx.index == 2) {
                            BM78_rx.response.LEConnectionComplete_0x71.connection_handle = byte;
                        } else if (BM78_rx.index == 3) {
                            BM78_rx.response.LEConnectionComplete_0x71.role = byte;
                        } else if (BM78_rx.index == 4) {
                            BM78_rx.response.LEConnectionComplete_0x71.peer_address_type = byte;
                        } else if (BM78_rx.index >= 5 && BM78_rx.index < 11) {
                            BM78_rx.response.LEConnectionComplete_0x71.peer_address[BM78_rx.index - 5] = byte;
                        } else if (BM78_rx.index == 11) {
                            BM78_rx.response.LEConnectionComplete_0x71.connection_interval = (byte << 8);
                        } else if (BM78_rx.index == 12) {
                            BM78_rx.response.LEConnectionComplete_0x71.connection_interval |= (byte & 0x00FF);
                        } else if (BM78_rx.index == 13) {
                            BM78_rx.response.LEConnectionComplete_0x71.connection_latency = (byte << 8);
                        } else if (BM78_rx.index == 14) {
                            BM78_rx.response.LEConnectionComplete_0x71.connection_latency |= (byte & 0x00FF);
                        } else if (BM78_rx.index == 15) {
                            BM78_rx.response.LEConnectionComplete_0x71.supervision_timeout = (byte << 8);
                        } else if (BM78_rx.index == 16) {
                            BM78_rx.response.LEConnectionComplete_0x71.supervision_timeout |= (byte & 0x00FF);
                        }
                        break;
                    case BM78_EVENT_DISCONNECTION_COMPLETE:
                        BM78.status = BM78_STATUS_IDLE_MODE;
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.DisconnectionComplete_0x72.connection_handle = byte;
                        } else if (BM78_rx.index == 2) {
                            BM78_rx.response.DisconnectionComplete_0x72.reason = byte;
                        }
                        break;
                    case BM78_EVENT_SPP_CONNECTION_COMPLETE:
                        BM78.status = BM78_STATUS_SPP_CONNECTED_MODE;
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.SPPConnectionComplete_0x74.status = byte;
                        } else if (BM78_rx.index == 2) {
                            BM78_rx.response.SPPConnectionComplete_0x74.connection_handle = byte;
                        } else if (BM78_rx.index >= 3 && BM78_rx.index < 9) {
                            BM78_rx.response.SPPConnectionComplete_0x74.peer_address[BM78_rx.index - 3] = byte;
                        }
                        break;
                        // Common Events
                    case BM78_EVENT_COMMAND_COMPLETE:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.CommandComplete_0x80.command = byte;
                        } else if (BM78_rx.index == 2) {
                            BM78_rx.response.CommandComplete_0x80.status = byte;
                        } else {
                            *(BM78_rx.buffer + BM78_rx.index - 3) = byte;
                            *(BM78_rx.buffer + BM78_rx.index - 2) = 0x00;
                        }
                        break;
                    case BM78_EVENT_BM77_STATUS_REPORT:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.StatusReport_0x81.status = byte;
                        }
                        break;
#ifndef BM78_MANUAL_MODE
                    case BM78_EVENT_CONFIGURE_MODE_STATUS:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.ConfigureModeStatus_0x8F.status = byte;
                        }
                        break;
#endif
                        // SPP/GATT Transparent Event
                    case BM78_EVENT_RECEIVED_TRANSPARENT_DATA:
                    case BM78_EVENT_RECEIVED_SPP_DATA:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.ReceivedTransparentData_0x9A.reserved = byte;
                        } else {
                            *(BM78_rx.buffer + BM78_rx.index - 2) = byte;
                            *(BM78_rx.buffer + BM78_rx.index - 1) = 0x00;
                        }
                        break;
                    default:
                        BM78_state = BM78_STATE_IDLE;
                        break;
                }
                BM78_rx.response.checksum_calculated += byte;
                BM78_rx.index++;
            } else {
                BM78_rx.response.checksum_received = byte;
                BM78_rx.response.checksum_calculated = 0xFF - BM78_rx.response.checksum_calculated + 1;
                if (BM78_rx.response.checksum_calculated == BM78_rx.response.checksum_received) {
                    BM78_AsyncEventResponse();
                } else if (BM78_appModeErrorHandler) {
                    BM78_appModeErrorHandler(BM78_rx.response, BM78_rx.buffer);
                }
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        default:
            BM78_state = BM78_STATE_IDLE;
            break;
    }
}

void BM78_processByteInTestMode(uint8_t byte) {
    switch (BM78_state) {
        case BM78_STATE_IDLE:
            BM78_state = byte == 0x04
                    ? BM78_ISSC_EVENT_STATE_INIT
                    : BM78_STATE_IDLE;
            break;
        case BM78_ISSC_EVENT_STATE_INIT:
            BM78_state = byte == 0x0E
                    ? BM78_ISSC_EVENT_STATE_LENGTH
                    : BM78_STATE_IDLE;
            break;
        case BM78_ISSC_EVENT_STATE_LENGTH:
            BM78_rx.response.ISSC_Event.length = byte;
            if (BM78_rx.response.ISSC_Event.length >= 4) {
                BM78_state = BM78_ISSC_EVENT_STATE_PACKET_TYPE;
            } else {
               if (BM78_testModeErrorHandler) BM78_testModeErrorHandler(BM78_rx.response, BM78_rx.buffer);
               BM78_state = BM78_STATE_IDLE;
            }
            //for (uint8_t i = 0; i < 0xFF; i++) BM78_cmdRX.data[i] = 0x00;
            break;
        case BM78_ISSC_EVENT_STATE_PACKET_TYPE:
            BM78_rx.response.ISSC_Event.packet_type = byte;
            if (BM78_rx.response.ISSC_Event.packet_type == 0x01) {
                BM78_state = BM78_ISSC_EVENT_STATE_OCF;
            } else {
                if (BM78_testModeErrorHandler) BM78_testModeErrorHandler(BM78_rx.response, BM78_rx.buffer);
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        case BM78_ISSC_EVENT_STATE_OCF:
            BM78_rx.response.ISSC_Event.ocf = byte;
            BM78_state = BM78_ISSC_EVENT_STATE_OGF;
            break;
        case BM78_ISSC_EVENT_STATE_OGF:
            BM78_rx.response.ISSC_Event.ogf = byte;
            BM78_state = BM78_ISSC_EVENT_STATE_STATUS;
            break;
        case BM78_ISSC_EVENT_STATE_STATUS:
            BM78_rx.response.ISSC_Event.status = byte;
            if (BM78_rx.response.ISSC_Event.ocf == BM78_ISSC_OCF_READ && BM78_rx.response.ISSC_Event.status == BM78_ISSC_STATUS_SUCCESS
                    && BM78_rx.response.ISSC_Event.length > 7) { // 4 + 2 byte address + 1 byte length
                BM78_state = BM78_ISSC_EVENT_STATE_DATA_ADDRESS_HIGH;
            } else {
                if (BM78_testModeEventHandler) BM78_testModeEventHandler(BM78_rx.response, BM78_rx.buffer);
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        case BM78_ISSC_EVENT_STATE_DATA_ADDRESS_HIGH:
            BM78_rx.response.ISSC_ReadEvent.address = (byte << 8);
            BM78_state = BM78_ISSC_EVENT_STATE_DATA_ADDRESS_LOW;
            break;
        case BM78_ISSC_EVENT_STATE_DATA_ADDRESS_LOW:
            BM78_rx.response.ISSC_ReadEvent.address |= (byte & 0xFF);
            BM78_state = BM78_ISSC_EVENT_STATE_DATA_LENGTH;
            break;
        case BM78_ISSC_EVENT_STATE_DATA_LENGTH:
            BM78_rx.response.ISSC_ReadEvent.data_length = byte;
            if (BM78_rx.response.ISSC_ReadEvent.data_length + 7 == BM78_rx.response.ISSC_ReadEvent.length) {
                BM78_rx.index = 0;
                BM78_state = BM78_ISSC_EVENT_STATE_DATA;
            } else {
                if (BM78_testModeErrorHandler) BM78_testModeErrorHandler(BM78_rx.response, BM78_rx.buffer);
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        case BM78_ISSC_EVENT_STATE_DATA:
            BM78_rx.buffer[BM78_rx.index++] = byte;
            if (BM78_rx.index >= BM78_rx.response.ISSC_ReadEvent.data_length) {
                if (BM78_testModeEventHandler) BM78_testModeEventHandler(BM78_rx.response, BM78_rx.buffer);
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        default:
            BM78_state = BM78_STATE_IDLE;
            break;
    }
}

void BM78_checkNewDataAsync(void) {
    if (UART_isRXReady()) {
        BM78_counters.idle = 0; // Reset idle counter
        uint8_t byte = UART_read();
        switch (BM78.mode) {
            case BM78_MODE_INIT:
            case BM78_MODE_APP:
                BM78_processByteInAppMode(byte);
                break;
            case BM78_MODE_TEST:
                BM78_processByteInTestMode(byte);
                break;
        }
    }
}

#endif