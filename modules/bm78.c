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
    uint8_t buffer[SCOM_MAX_PACKET_SIZE + 7];
} BM78_tx;

BM78_EventState_t BM78_state = BM78_STATE_IDLE;

/* Commands */
//                                        TYPE   OCF   OGF   LEN  PARAM
const uint8_t BM78_CMD_EEPROM_OPEN[4]  = {0x01, 0x03, 0x0c, 0x00};
const uint8_t BM78_CMD_EEPROM_CLEAR[5] = {0x01, 0x2d, 0xfc, 0x01, 0x08};
//                                        TYPE   OCF   OGF   LEN  AD_H  AD_L  SIZE
const uint8_t BM78_CMD_EEPROM_WRITE[7] = {0x01, 0x27, 0xfc, 0x00, 0x00, 0x00, 0x00};
const uint8_t BM78_CMD_EEPROM_READ[7]  = {0x01, 0x29, 0xfc, 0x03, 0x00, 0x00, 0x01};

UART_Connection_t BM78_uart;
BM78_SetupHandler_t BM78_setupHandler;
BM78_EventHandler_t BM78_appModeEventHandler;
BM78_EventHandler_t BM78_testModeEventHandler;
DataHandler_t BM78_transparentDataHandler;
Procedure_t BM78_cancelTransmissionHandler;
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
            BM78_write(BM78_CMD_WRITE_DEVICE_NAME, BM78_EEPROM_STORE, strlen(BM78.deviceName, 16), (uint8_t *) BM78.deviceName);
            break;
        case 4:
            printStatus("      ....          ");
            BM78_write(BM78_CMD_WRITE_ADV_DATA, BM78_EEPROM_STORE, sizeof(BM78_ADV_DATA), (uint8_t *) BM78_ADV_DATA);
            break;
        case 5:
            printStatus("      .....         ");
            BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
            break;
        case 6:
            printStatus("      ....*         ");
            BM78_execute(BM78_CMD_WRITE_PAIRING_MODE_SETTING, 2, BM78_EEPROM_STORE, BM78.pairingMode);
            break;
        case 7:
            printStatus("      ......        ");
            BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
            break;
        case 8:
            printStatus("      .....*        ");
            BM78_write(BM78_CMD_WRITE_PIN_CODE, BM78_EEPROM_STORE, strlen(BM78.pin, 6), (uint8_t *) BM78.pin);
            break;
        case 9:
            printStatus("      .......       ");
            BM78_execute(BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO, 0);
            break;
        case 10:
            printStatus("      ........      ");
            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
            break;
        case 11:
            printStatus("      ........      ");
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
                UART_Connection_t uart,
                BM78_SetupHandler_t setupHandler,
                BM78_EventHandler_t appModeEventHandler,
                BM78_EventHandler_t testModeResponseHandler,
                DataHandler_t transparentDataHandler,
                Procedure_t cancelTransmissionHandler,
                BM78_EventHandler_t appModeErrorHandler,
                BM78_EventHandler_t testModeErrorHandler) {
    BM78_uart = uart;
    BM78_setupHandler = setupHandler;
    BM78_appModeEventHandler = appModeEventHandler;
    BM78_testModeEventHandler = testModeResponseHandler;
    BM78_transparentDataHandler = transparentDataHandler;
    BM78_cancelTransmissionHandler = cancelTransmissionHandler;
    BM78_appModeErrorHandler = appModeErrorHandler;
    BM78_testModeErrorHandler = testModeErrorHandler;
    BM78_setup(true);
}

void BM78_clear() {
    BM78_rx.index = 0;
    BM78_state = BM78_STATE_IDLE;
    if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
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
                BM78_init.attempt++;
                while(UART_isRXReady(BM78_uart)) UART_read(BM78_uart);
                if (BM78_init.attempt == 3) BM78_reset(); // Soft reset
                BM78_state = BM78_STATE_IDLE;
                BM78_retryInitialization();
                break;
            }
            // no break here
        case BM78_MODE_APP:
            printStatus("      .             ");
            while(UART_isRXReady(BM78_uart)) UART_read(BM78_uart);
            BM78_state = BM78_STATE_IDLE;
            BM78.mode = BM78_MODE_INIT;
            BM78_counters.idle = 0;
            BM78_counters.missedStatusUpdate = 0;
            BM78_init.stage = 0;
            BM78_init.attempt = 0;
            if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
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
        while (!UART_isTXReady(BM78_uart)); // Wait till we can start sending.
        UART_write(BM78_uart, *(data + i)); // Store each byte in the storePacket into the UART write buffer
        while (!UART_isTXDone(BM78_uart)); // Wait until UART TX is done.
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

void BM78_storePairedDevices(uint8_t pairedDevicesCount, BM78_PairedDevice_t *pairedDevices) {
    //<-- 0001 0C
    //                  C  I1 P1 MAC1         I2 P2 MAC2 
    //--> 0014 80 0C 00 02 00 01 123456789ABC 01 02 123456789ABC
    BM78.pairedDevicesCount = pairedDevicesCount;
    for (uint8_t i = 0; i < BM78.pairedDevicesCount; i++) {
        BM78.pairedDevices[i].index= pairedDevices[i].index;
        BM78.pairedDevices[i].priority = pairedDevices[i].priority;
        for (uint8_t j = 0; j < 6; j++) {
            BM78.pairedDevices[i].address[j] = pairedDevices[i].address[j];
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
            switch (BM78_rx.response.opCode) {
                case BM78_EVENT_LE_CONNECTION_COMPLETE:
                    BM78.status = BM78_STATUS_LE_CONNECTED_MODE;
                    BM78.connectionHandle = BM78_rx.response.LEConnectionComplete_0x71.connectionHandle;
                    break;
                case BM78_EVENT_DISCONNECTION_COMPLETE:
                    BM78.status = BM78_STATUS_IDLE_MODE;
                    BM78.connectionHandle = 0x00;
                    if (BM78.enforceState) {
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                    }
                    if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
                    break;
                case BM78_EVENT_SPP_CONNECTION_COMPLETE:
                    BM78.status = BM78_STATUS_SPP_CONNECTED_MODE;
                    BM78.connectionHandle = BM78_rx.response.SPPConnectionComplete_0x74.connectionHandle;
                    if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
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
#ifdef BM78_ADV_DATA_SIZE
                                    BM78_init.stage = 4;
#else
                                    BM78_init.stage = 5;
#endif
                                    strcpy(BM78.deviceName,
                                            (char *) BM78_rx.response.DeviceName_0x80.deviceName,
                                            BM78_rx.response.CommandComplete_0x80.length - 3);
                                    BM78.deviceName[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
#ifdef BM78_SETUP_ENABLED
                                } else if (strcmp(BM78.deviceName,
                                        (char *) BM78_rx.response.DeviceName_0x80.deviceName,
                                        BM78_rx.response.CommandComplete_0x80.length - 3)) {
#ifdef BM78_ADV_DATA_SIZE
                                    BM78_init.stage = 4;
#else
                                    BM78_init.stage = 5;
#endif
                                    BM78.deviceName[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
                                } else {
                                    BM78_init.stage = 3;
                                    BM78_retryInitialization();
                                }
                                break;
                            case BM78_CMD_WRITE_DEVICE_NAME:          // Stage 3
                                BM78_init.attempt = 0;
                                BM78_init.stage = 2;
                                BM78_retryInitialization();
                                break;
                            case BM78_CMD_WRITE_ADV_DATA:             // Stage 4
                                BM78_init.attempt = 0;
                                BM78_init.stage = 5;
                                BM78_retryInitialization();
#endif
                                break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:  // Stage 5
                                BM78_init.attempt = 0;
#ifdef BM78_SETUP_ENABLED
                                if (BM78_init.keep) {
#endif
                                    BM78_init.stage = 7;
                                    BM78.pairingMode = BM78_rx.response.PairingMode_0x80.mode;
                                    BM78_retryInitialization();
#ifdef BM78_SETUP_ENABLED
                                } else if (BM78.pairingMode == BM78_rx.response.PairingMode_0x80.mode) {
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
                                    strcpy(BM78.pin, (char *) BM78_rx.response.PIN_0x80.value, BM78_rx.response.CommandComplete_0x80.length - 3);
                                    BM78.pin[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                    BM78_retryInitialization();
#ifdef BM78_SETUP_ENABLED
                                } else if (strcmp(BM78.pin, (char *) BM78_rx.response.PIN_0x80.value, BM78_rx.response.CommandComplete_0x80.length - 3)) {
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
                                BM78_storePairedDevices(
                                        BM78_rx.response.PairedDevicesInformation_0x80.count,
                                        BM78_rx.response.PairedDevicesInformation_0x80.devices);
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
                case BM78_EVENT_STATUS_REPORT: // Manual mode
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
            if (BM78_rx.response.opCode != BM78_EVENT_COMMAND_COMPLETE 
                    || BM78_rx.response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                switch (BM78_rx.response.opCode) {
                    case BM78_EVENT_LE_CONNECTION_COMPLETE:
                        BM78.status = BM78_STATUS_LE_CONNECTED_MODE;
                        BM78.connectionHandle = BM78_rx.response.LEConnectionComplete_0x71.connectionHandle;
                        break;
                    case BM78_EVENT_DISCONNECTION_COMPLETE:
                        BM78.status = BM78_STATUS_IDLE_MODE;
                        BM78.connectionHandle = 0x00;
                        if (BM78.enforceState) {
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                        }
                        if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
                        break;
                    case BM78_EVENT_SPP_CONNECTION_COMPLETE:
                        if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
                        break;
                    case BM78_EVENT_COMMAND_COMPLETE:
                        switch (BM78_rx.response.CommandComplete_0x80.command) {
                            case BM78_CMD_READ_DEVICE_NAME:
                                strcpy(BM78.deviceName, (char *) BM78_rx.response.DeviceName_0x80.deviceName, BM78_rx.response.CommandComplete_0x80.length - 3);
                                BM78.deviceName[BM78_rx.response.CommandComplete_0x80.length - 3] = '\0';
                                break;
                            //case BM78_CMD_WRITE_DEVICE_NAME:
                            //    BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                            //    break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:
                                BM78.pairingMode = BM78_rx.response.PairingMode_0x80.mode;
                                break;
                            case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                                BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                                break;
                            case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                                BM78_storePairedDevices(
                                        BM78_rx.response.PairedDevicesInformation_0x80.count,
                                        BM78_rx.response.PairedDevicesInformation_0x80.devices);
                                break;
                            case BM78_CMD_READ_PIN_CODE:
                                strcpy(BM78.pin,
                                        (char *) BM78_rx.response.PIN_0x80.value, 
                                        BM78_rx.response.CommandComplete_0x80.length - 3);
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
                    case BM78_EVENT_STATUS_REPORT:
                        BM78_counters.missedStatusUpdate = 0; // Reset missed status update counter.
                        BM78.status = BM78_rx.response.StatusReport_0x81.status;
                        switch (BM78.status) {
                            case BM78_STATUS_STANDBY_MODE:
                                if (BM78.enforceState == BM78_STANDBY_MODE_LEAVE) BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78.enforceState);
                                // No break here
                            case BM78_STATUS_LINK_BACK_MODE:
                            case BM78_STATUS_LE_CONNECTED_MODE:
                                if (BM78_cancelTransmissionHandler) BM78_cancelTransmissionHandler();
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
                        if (BM78_transparentDataHandler) {
                            BM78_transparentDataHandler(
                                    BM78_rx.response.ReceivedTransparentData_0x9A.length - 2,
                                    BM78_rx.response.ReceivedTransparentData_0x9A.data);
                        }
                        break;
                    default:
                        break;
                }

               if (BM78_appModeEventHandler) BM78_appModeEventHandler(&BM78_rx.response);

            } else {
                if (BM78_rx.response.opCode == BM78_EVENT_COMMAND_COMPLETE) {
                    switch (BM78_rx.response.CommandComplete_0x80.status) {
                        case BM78_ERR_COMMAND_DISALLOWED:
                            if (BM78_rx.response.CommandComplete_0x80.command == BM78_CMD_DISCONNECT) {
                                BM78_reset();
                            }
                            break;
                        default:
                            break;
                    }
                }
                if (BM78_appModeErrorHandler) BM78_appModeErrorHandler(&BM78_rx.response);
            }
            break;
        case BM78_MODE_TEST:
            break;
    }
}

inline void BM78_commandPrepareBuffer(BM78_CommandOpCode_t command, uint8_t length) {
    BM78_counters.idle = 0; // Reset idle counter
    BM78_tx.buffer[0] = 0xAA;                // Sync word.
    BM78_tx.buffer[1] = (length + 1) >> 8;   // Length high byte.
    BM78_tx.buffer[2] = (length + 1) & 0xFF; // Length low byte.
    BM78_tx.buffer[3] = command;             // Add command
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

void BM78_write(BM78_CommandOpCode_t command, BM78_EEPROM_t store, uint8_t length, uint8_t *value) {
    BM78_commandPrepareBuffer(command, length + 1);
    BM78_tx.buffer[4] = store;
    for (uint8_t i = 0; i < length; i++) {
        BM78_tx.buffer[i + 5] = *(value + i);
    }
    BM78_tx.buffer[length + 5] = 0xFF - BM78_commandCalculateChecksum(length + 1) + 1;
    BM78_commandCommit(length + 1);
}

void BM78_execute(BM78_CommandOpCode_t command, uint8_t length, ...) {
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

void BM78_send(BM78_Request_t *request) {
    switch (request->opCode) {
        //case BM78_CMD_DIO_CONTROL: // TODO
        //case BM78_CMD_PWM_CONTROL: // TODO
        //    request->length = 0;
        //    break;
        case BM78_CMD_WRITE_DEVICE_NAME:
            request->length = strlen((char *) request->WriteDeviceName_0x08.deviceName, 16);
            break;
        case BM78_CMD_WRITE_ADV_DATA:
            request->length = strlen((char *) request->WriteAdvData_0x11.advertisingData, 31) + 1;
            break;
        case BM78_CMD_WRITE_SCAN_RES_DATA:
            request->length = strlen((char *) request->WriteScanResData_0x12.scanResponseData, 31) + 1;
            break;
        //case BM78_CMD_DISCOVER_SPECIFIC_PRIMARY_SERVICE_CHARACTERISTICS:
        //    request->length = strlen((char *) request->DiscoverSpecificPrimaryServiceCharacteristics_0x31.serviceUUID, 16) + 1;
        //    break;
        //case BM78_CMD_READ_USING_CHARACTERISTIC_UUID:
        //    request->length = strlen((char *) request->ReadByCharacteristicUUID_0x33.characteristicUUID, 16) + 1;
        //    break;
        //case BM78_CMD_READ_LOCAL_SPECIFIC_PRIMARY_SERVICE:
        //    request->length = strlen((char *) request->ReadLocalSpecificPrimaryService_0x3C.serviceUUID, 16);
        //    break;
        case BM78_CMD_WRITE_PIN_CODE:
            request->length = strlen((char *) request->WritePinCode_0x51.pin, 6) + 1;
            break;
        //case BM78_CMD_WRITE_CHARACTERISTIC_VALUE:
        //    request->length += 4;
        //    break;
        //case BM78_CMD_SEND_CHARACTERISTIC_VALUE:
        //    request->length += 3;
        //    break;
        //case BM78_CMD_UPDATE_CHARACTERISTIC_VALUE:
        //    request->length += 2;
        //    break;
        //case BM78_CMD_SEND_GATT_TRANSPARENT_DATA:
        case BM78_CMD_SEND_TRANSPARENT_DATA:
        case BM78_CMD_SEND_SPP_DATA:
            request->length += 1;
            break;
        case BM78_CMD_SET_ADV_PARAMETER:
            request->length = 10;
            break;
        //case BM78_CMD_LE_CREATE_CONNECTION:
        //    request->length = 8;
        //    break;
        //case BM78_CMD_CONNECTION_PARAMETER_UPDATE_REQ:
            request->length = 7;
            break;
        //case BM78_CMD_SET_SCAN_PARAMETER:
        //case BM78_CMD_SEND_WRITE_RESPONSE:
        //    request->length = 5;
        //    break;
        //case BM78_CMD_READ_CHARACTERISTIC_VALUE:
        //case BM78_CMD_ENABLE_TRANSPARENT:
#ifdef BM78_ADVANCED_PAIRING
        case BM78_CMD_PASSKEY_ENTRY_RES:
#endif
            request->length = 3;
            break;
        case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
        case BM78_CMD_READ_RSSI_VALUE:
        case BM78_CMD_SET_SCAN_ENABLE:
        // TODO case BM78_CMD_READ_LOCAL_CHARACTERISTIC_VALUE:
#ifdef BM78_ADVANCED_PAIRING
        case BM78_CMD_USER_CONFIRM_RES:
#endif
            request->length = 2;
            break;
        case BM78_CMD_DELETE_PAIRED_DEVICE:
        case BM78_CMD_INVISIBLE_SETTING:
        case BM78_CMD_SPP_CREATE_LINK:
        case BM78_CMD_READ_REMOTE_DEVICE_NAME:
        //case BM78_CMD_DISCOVER_ALL_PRIMARY_SERVICES:
        case BM78_CMD_LEAVE_CONFIGURE_MODE:
            request->length = 1;
            break;
        default:
            request->length = 0;
    }
    BM78_commandPrepareBuffer(request->opCode, request->length);
    for (uint8_t i = 0; i < request->length; i++) {
        BM78_tx.buffer[i + 4] = request->data[i];
    }
    BM78_tx.buffer[request->length + 4] = 0xFF - BM78_commandCalculateChecksum(request->length) + 1;
    BM78_commandCommit(request->length);
}

void BM78_data(uint8_t command, uint8_t length, uint8_t *data) {
    BM78_commandPrepareBuffer(command, length);
    for (uint8_t i = 0; i < length; i++) {
        BM78_tx.buffer[i + 4] = *(data + i);
    }
    BM78_tx.buffer[length + 4] = 0xFF - BM78_commandCalculateChecksum(length) + 1;
    BM78_commandCommit(length);
}

void BM78_sendTransparentData(uint8_t length, uint8_t *data) {
    BM78_commandPrepareBuffer(BM78_CMD_SEND_TRANSPARENT_DATA, length + 1);
    BM78_tx.buffer[4] = 0x00; // Reserved by BM78
    for (uint8_t i = 0; i < length; i++) {
        BM78_tx.buffer[i + 5] = *(data + i);
    }
    BM78_tx.buffer[length + 5] = 0xFF - BM78_commandCalculateChecksum(length + 1) + 1;
    BM78_commandCommit(length + 1);
}

void BM78_processByteInAppMode(uint8_t byte) {
    switch (BM78_state) {
        case BM78_STATE_IDLE:
            if (byte == 0xAA) { // SYNC WORD
                BM78_rx.response.checksum = 0;
                BM78_state = BM78_EVENT_STATE_LENGTH_HIGH;
            }
            break;
        case BM78_EVENT_STATE_LENGTH_HIGH:
            // Ignoring lengthHigh byte - max length: 255
            BM78_rx.response.length = (byte << 8);
            BM78_rx.response.checksum += byte;
            BM78_state = BM78_EVENT_STATE_LENGTH_LOW;
            break;
        case BM78_EVENT_STATE_LENGTH_LOW:
            BM78_rx.response.length |= (byte & 0x00FF);
            BM78_rx.response.checksum += byte;
            //BM78_rx.index = 0;
            BM78_state = BM78_EVENT_STATE_OP_CODE;
            break;
        case BM78_EVENT_STATE_OP_CODE:
            BM78_rx.response.opCode = byte;
            BM78_rx.response.checksum += byte;
            //BM78_rx.index++;
            BM78_rx.index = 1;
            BM78_state = BM78_EVENT_STATE_ADDITIONAL;
            break;
        case BM78_EVENT_STATE_ADDITIONAL:
            if (BM78_rx.index < BM78_rx.response.length) {
                BM78_rx.response.data[BM78_rx.index - 1] = byte;
                BM78_rx.response.checksum += byte;
                BM78_rx.index++;
            } else {
                BM78_rx.response.checksum = 0xFF - BM78_rx.response.checksum + 1;
                if (BM78_rx.response.checksum == byte
                        && (BM78_rx.response.opCode != BM78_EVENT_COMMAND_COMPLETE || BM78_rx.response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED)) {
                    if (BM78_rx.response.opCode == BM78_EVENT_STATUS_REPORT
                            || (BM78_rx.response.opCode == BM78_EVENT_COMMAND_COMPLETE && BM78_rx.response.CommandComplete_0x80.command == BM78_CMD_INVISIBLE_SETTING)) {
                        BM78_AsyncEventResponse();
                        BM78_state = BM78_STATE_IDLE;
                    } else {
                        BM78_AsyncEventResponse();
                    }
                } else if (BM78_appModeErrorHandler) {
                    BM78_appModeErrorHandler(&BM78_rx.response);
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
               if (BM78_testModeErrorHandler) BM78_testModeErrorHandler(&BM78_rx.response);
               BM78_state = BM78_STATE_IDLE;
            }
            //for (uint8_t i = 0; i < 0xFF; i++) BM78_cmdRX.data[i] = 0x00;
            break;
        case BM78_ISSC_EVENT_STATE_PACKET_TYPE:
            BM78_rx.response.ISSC_Event.packetType = byte;
            if (BM78_rx.response.ISSC_Event.packetType == 0x01) {
                BM78_state = BM78_ISSC_EVENT_STATE_OCF;
            } else {
                if (BM78_testModeErrorHandler) BM78_testModeErrorHandler(&BM78_rx.response);
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
                if (BM78_testModeEventHandler) BM78_testModeEventHandler(&BM78_rx.response);
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
            BM78_rx.response.ISSC_ReadEvent.dataLength = byte;
            if (BM78_rx.response.ISSC_ReadEvent.dataLength + 7 == BM78_rx.response.ISSC_ReadEvent.length) {
                BM78_rx.index = 0;
                BM78_state = BM78_ISSC_EVENT_STATE_DATA;
            } else {
                if (BM78_testModeErrorHandler) BM78_testModeErrorHandler(&BM78_rx.response);
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        case BM78_ISSC_EVENT_STATE_DATA:
            BM78_rx.response.ISSC_ReadEvent.data[BM78_rx.index++] = byte;
            if (BM78_rx.index >= BM78_rx.response.ISSC_ReadEvent.dataLength) {
                if (BM78_testModeEventHandler) BM78_testModeEventHandler(&BM78_rx.response);
                BM78_state = BM78_STATE_IDLE;
            }
            break;
        default:
            BM78_state = BM78_STATE_IDLE;
            break;
    }
}

void BM78_checkNewDataAsync(void) {
    if (UART_isRXReady(BM78_uart)) {
        BM78_counters.idle = 0; // Reset idle counter
        uint8_t byte = UART_read(BM78_uart);
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