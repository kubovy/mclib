/* 
 * File:   bm78.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78.h"
#ifdef BM78_ENABLED
#include "../lib/common.h"

struct {
    uint16_t index;           // Byte index in a message.
    BM78_EventState_t state;  // Event transmission state.
    BM78_Response_t response; // Response.
    uint8_t data[0xFF];       // Response data.
} BM78_rx = {0, BM78_EVENT_STATE_IDLE};

struct {
    uint8_t index;                    // Byte index in message.
    BM78_CommandResponseState_t state;// Command transmission state.
    uint8_t length;                   // Response length.
    uint8_t data[BM78_BUFFER_SIZE];   // Response data.
} BM78_cmdRX = {0, BM78_COMMAND_RESPONSE_STATE_IDLE, 0};

struct {
    uint16_t idle;
    uint8_t missedStatusUpdate;
} BM78_counters = {0, 0};

struct {
    uint8_t length;         // Transmission length: 0 = nothing to transmit.
    uint8_t chksumExpected; // Expected checksum calculated before sending.
    uint8_t chksumReceived; // Received checksum by the peer.
    uint16_t timeout;       // Timeout countdown used by retry trigger.
    uint8_t data[0xFF];     // Data to send.
} BM78_tx = {0, 0x00, 0xFF, 0};

uint8_t BM78_writeBuffer[BM78_BUFFER_SIZE];

/* Commands */
//                                    C0    C1    C2   PLEN  PARAM
const uint8_t BM78_COMMAND_S1[4] = {0x01, 0x03, 0x0c, 0x00};
const uint8_t BM78_COMMAND_S2[5] = {0x01, 0x2d, 0xfc, 0x01, 0x08};
//                                    C0    C1    C2   PLEN  AD_H  AD_L  SIZE
const uint8_t BM78_COMMAND_W[7] = {0x01, 0x27, 0xfc, 0x00, 0x00, 0x00, 0x00};
const uint8_t BM78_COMMAND_R[7] = {0x01, 0x29, 0xfc, 0x03, 0x00, 0x00, 0x01};

void (*BM78_SetupHandler)(void);
void (*BM78_InitializationHandler)(char*, char*);
void (*BM78_TestModeResponseHandler)(uint8_t, uint8_t*);
void (*BM78_ResponseHandler)(BM78_Response_t, uint8_t*);
void (*BM78_ErrorHandler)(BM78_Response_t, uint8_t*);
void (*BM78_TransparentDataHandler)(uint8_t, uint8_t*);
void (*BM78_MessageSentHandler)(void);

void BM78_init(void) {
    BM78_rx.index = 0;
    BM78_rx.state = BM78_EVENT_STATE_IDLE;
    BM78_tx.length = 0;
    BM78_tx.chksumExpected = 0x00;
    BM78_tx.chksumReceived = 0xFF;
}

void BM78_power(bool on) {
    BM78_init();
    if (BM78_SW_BTN_PORT != on) {
        BM78_SW_BTN_LAT = on;
        __delay_ms(500);
    }
}

void BM78_reset(void) {
    BM78_init();
    BM78_RST_N_SetLow(); // Set the reset pin low
    __delay_ms(2); // A reset pulse must be greater than 1ms
    BM78_RST_N_SetHigh(); // Pull the reset pin back up    
}

/* Reset the bluetooth unit and enter the mode specified by the SYS CON pin */
void BM78_resetMode(void) {
    BM78_reset(); // Reset the device
    __delay_ms(100); // Wait a minimum of 24 seconds for mode to be detected
    //BM78_P2_0_SetHigh();  // Reset P2_0 pin to high state
    //__delay_ms(100); // Wait a minimum of 43ms for UART to activate after reset
}

void BM78_resetToTestMode(void) {
    BM78.mode = BM78_MODE_TEST;

    BM78_power(false);
    __delay_ms(200);
    BM78_power(true);
    __delay_ms(200);
    BM78_reset();
    __delay_ms(200);

    BM78_P2_0_SetLow();
    BM78_P2_4_SetHigh();
    BM78_EAN_SetLow();
    //BM78_reset();
    //__delay_ms(100); // Wait a minimum of 24 seconds for mode to be detected
    BM78_resetMode();
}

void BM78_resetToAppMode(void) {
    BM78.mode = BM78_MODE_INIT;
    BM78_P2_0_SetHigh();
    BM78_P2_4_SetHigh();
    BM78_EAN_SetLow();
    //BM78_reset();
    //__delay_ms(100); // Wait a minimum of 24 seconds for mode to be detected
    BM78_resetMode();
}

void BM78_clearWriteBuffer(void) {
    for (uint8_t i = 0; i < sizeof (BM78_writeBuffer); i++) {
        BM78_writeBuffer[i] = 0xFF;
    }
}

void BM78_loadSendBuffer(uint8_t length, uint8_t *data) {
    BM78_clearWriteBuffer();
    for (uint8_t i = 0; i < length; i++) {
        BM78_writeBuffer[i] = *(data + i);
    }
}

void BM78_setup(void) {
    switch (BM78.mode) {
        case BM78_MODE_INIT:
        case BM78_MODE_APP:
            if (BM78_SetupHandler) {
                BM78_SetupHandler();
            }
            BM78_rx.state = BM78_EVENT_STATE_IDLE;
            BM78.mode = BM78_MODE_INIT;
            BM78_counters.idle = 0;
            BM78_counters.missedStatusUpdate = 0;
            BM78_tx.length = 0; // Abort ongoing transmission
            //BM78_power(false);
            //__delay_ms(250);
            BM78_power(true);
            BM78_reset();
            break;
        case BM78_MODE_TEST:
            break;
    }
}

void BM78_sendPacket(uint8_t length, uint8_t *data) {
    uint8_t byte;
    //if (clearRX) while (UART1_is_rx_ready()) UART1_Read(); // Clear UART RX queue.
    BM78_counters.idle = 0; // Reset idle counter
    BM78_rx.state = BM78_EVENT_STATE_IDLE;
    BM78_cmdRX.state = BM78_COMMAND_RESPONSE_STATE_IDLE;

    while (!UART1_is_tx_ready()); // Wait till we can start sending.
    for (int i = 0; i < length; i++) { // Send the command bits, along with the parameters
        byte = *(data + i);
        UART1_Write(byte); // Store each byte in the storePacket into the UART write buffer
        while (!UART1_is_tx_done()); // Wait until UART TX is done.
    }
    BM78_counters.idle = 0; // Reset idle counter
}

void BM78_openEEPROM(void) {
    BM78_sendPacket(sizeof (BM78_COMMAND_S1), (uint8_t *) BM78_COMMAND_S1);
}

void BM78_clearEEPROM(void) {
    BM78_sendPacket(sizeof (BM78_COMMAND_S2), (uint8_t *) BM78_COMMAND_S2);
}

void BM78_readEEPROM(uint16_t address, uint8_t length) {
    BM78_loadSendBuffer(sizeof (BM78_COMMAND_R), (uint8_t *) BM78_COMMAND_R);
    BM78_writeBuffer[4] = address >> 8;
    BM78_writeBuffer[5] = address & 0xFF;
    BM78_writeBuffer[6] = length;

    BM78_sendPacket(sizeof (BM78_COMMAND_R), BM78_writeBuffer);
}

void BM78_writeEEPROM(uint16_t address, uint8_t length, uint8_t *data) {
    BM78_loadSendBuffer(sizeof (BM78_COMMAND_W), (uint8_t *) BM78_COMMAND_W);
    BM78_writeBuffer[3] = length + 3;
    BM78_writeBuffer[4] = address >> 8;
    BM78_writeBuffer[5] = address & 0xFF;
    BM78_writeBuffer[6] = length;
    for (uint8_t i = 0; i < length; i++) {
        BM78_writeBuffer[i + 7] = *(data + i);
    }

    BM78_sendPacket(sizeof (BM78_COMMAND_W) + length, BM78_writeBuffer);
}

void BM78_checkState(void) {
    switch (BM78.mode) {
        case BM78_MODE_INIT: // In Initialization mode retry setting up the BM78
                             // in a defined interval
            if (BM78_counters.idle > (BM78_INITIALIZATION_TIMOUT / BM78_TRIGGER_PERIOD)) {
                LED1_Toggle();
                BM78_counters.idle = 0; // Reset idle counter.
                BM78_setup();
            }
            break;
        case BM78_MODE_APP: // In Application mode status is refreshed in 
                            // a defined interval. In case a certain consequent
                            // number of refresh attempts will be missed, the
                            // device will refresh itself.
            if (BM78_counters.idle > (BM78_STATUS_CHECK_TIMEOUT / BM78_TRIGGER_PERIOD)) {
                LED1_Toggle();
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

void BM78_setSetupHandler(void (* SetupHandler)(void)) {
    BM78_SetupHandler = SetupHandler;
}

void BM78_setInitializationHandler(void (* InitializationHandler)(char*, char*)) {
    BM78_InitializationHandler = InitializationHandler;
}

void BM78_setAppModeResponseHandler(void (* ResponseHandler)(BM78_Response_t, uint8_t*)) {
    BM78_ResponseHandler = ResponseHandler;
}

void BM78_setTestModeResponseHandler(void (* TestModeResponseHandler)(uint8_t, uint8_t*)) {
    BM78_TestModeResponseHandler = TestModeResponseHandler;
}

void BM78_setErrorHandler(void (* ErrorHandler)(BM78_Response_t, uint8_t*)) {
    BM78_ErrorHandler = ErrorHandler;
}

void BM78_setTransparentDataHandler(void (* TransparentDataHandler)(uint8_t, uint8_t*)) {
    BM78_TransparentDataHandler = TransparentDataHandler;
}

void BM78_setMessageSentHandler(void (* MessageSentHandler)(void)) {
    BM78_MessageSentHandler = MessageSentHandler;
}

void BM78_GetAdvData(uint8_t *data) {
    uint8_t i;
    data[0] = 0x02; // Size A
    data[1] = 0x01; // Flag
    data[2] = 0x02; // Dual-Mode
    data[3] = strlen(BM78.deviceName) + 1; // Size B
    data[4] = 0x09;
    for (i = 0; i < 16; i++) {
        data[i + 5] = BM78.deviceName[i];
        data[i + 6] = 0x00;
    }
}

void BM78_AsyncEventResponse(BM78_Response_t response, uint8_t *data) {
    uint8_t transparentData[0xFF];
    uint8_t chksum, chksumReceived;
    switch (BM78.mode) {
            /*
             * Initialization Mode
             */
        case BM78_MODE_INIT:
            switch (response.op_code) {
                    // On disconnection enter stand-by mode if this mode is enfoced
                    // and ...
                case BM78_OPC_DISCONNECTION_COMPLETE:
                    if (BM78.enforceStandBy) {
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                    }
                    // ... on connection, both LE or SPP, abort ongoing transmission.
                case BM78_OPC_LE_CONNECTION_COMPLETE:
                case BM78_OPC_SPP_CONNECTION_COMPLETE:
                    BM78_tx.length = 0; // Abort ongoing transmission.
                    break;
                case BM78_OPC_COMMAND_COMPLETE:
                    // Initialization flow:
                    if (response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                        switch (response.CommandComplete_0x80.command) {
                            case BM78_CMD_READ_LOCAL_INFORMATION:
                                BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                                break;
                            case BM78_CMD_READ_DEVICE_NAME:
                                strcpy(BM78.deviceName, (char *) data, response.CommandComplete_0x80.length - 1);
                                BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                                break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:
                                BM78.pairingMode = data[0];
                                BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                                break;
                            case BM78_CMD_READ_PIN_CODE:
                                strcpy(BM78.pin, (char *) data, response.CommandComplete_0x80.length - 1);
                                BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                                break;
                            case BM78_CMD_INVISIBLE_SETTING:
                                if (BM78.mode == BM78_MODE_INIT) BM78.mode = BM78_MODE_APP;
                                LED2_SetLow();
                                if (BM78_InitializationHandler) {
                                    BM78_InitializationHandler(BM78.deviceName, BM78.pin);
                                }
                                break;
                            default:
                                break;
                        }
                    } else { // Repeat last initialization command on failure.
                        switch (response.CommandComplete_0x80.command) {
                            case BM78_CMD_READ_LOCAL_INFORMATION:
                                BM78_execute(BM78_CMD_READ_LOCAL_INFORMATION, 0);
                                break;
                            case BM78_CMD_READ_DEVICE_NAME:
                                BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                                break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:
                                BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                                break;
                            case BM78_CMD_READ_PIN_CODE:
                                BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                                break;
                            case BM78_CMD_INVISIBLE_SETTING:
                                BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case BM78_OPC_BM77_STATUS_REPORT: // Manual mode
                    BM78.status = response.StatusReport_0x81.status;
                    switch (BM78.status) {
                        case BM78_STATUS_IDLE_MODE:
                            BM78_execute(BM78_CMD_READ_LOCAL_INFORMATION, 0);
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
            if (response.op_code != BM78_OPC_COMMAND_COMPLETE || response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                switch (response.op_code) {
                    // On disconnection enter stand-by mode if this mode is
                    // enfoced and ...
                    case BM78_OPC_DISCONNECTION_COMPLETE:
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                    case BM78_OPC_LE_CONNECTION_COMPLETE:
                    case BM78_OPC_SPP_CONNECTION_COMPLETE:
                        BM78_tx.length = 0; // Abort ongoing transmission.
                        break;
                    case BM78_OPC_COMMAND_COMPLETE:
                        switch (response.CommandComplete_0x80.command) {
                            case BM78_CMD_READ_DEVICE_NAME:
                                strcpy(BM78.deviceName, (char *) data, response.CommandComplete_0x80.length - 1);
                                break;
                            case BM78_CMD_READ_PAIRING_MODE_SETTING:
                            case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                                BM78.pairingMode = data[0];
                                break;
                            case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                                //<-- 0001 0C
                                //                  C  I1 P1 MAC1         I2 P2 MAC2 
                                //--> 0014 80 0C 00 02 00 01 123456789ABC 01 02 123456789ABC
                                BM78.pairedDevicesCount = data[0];
                                for (uint8_t i = 0; i < BM78.pairedDevicesCount; i++) {
                                    BM78.pairedDevices[i].index = data[i * 8 + 1];
                                    BM78.pairedDevices[i].priority = data[i * 8 + 2];
                                    for (uint8_t j = 0; j < 6; j++) {
                                        BM78.pairedDevices[i].address[j] = data[i * 8 + 3 + j];
                                    }
                                }
                                break;
                            case BM78_CMD_READ_PIN_CODE:
                                strcpy(BM78.pin, (char *) data, response.CommandComplete_0x80.length - 1);
                                break;
                            case BM78_CMD_WRITE_PIN_CODE:
                                // Re-read PIN code after writing PIN code.
                                BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                                break;
                            case BM78_CMD_DISCONNECT:
                                if (BM78.enforceStandBy) {
                                    BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case BM78_OPC_BM77_STATUS_REPORT:
                        BM78_counters.missedStatusUpdate = 0; // Reset missed status update counter.
                        if (BM78.enforceStandBy) LED2_Toggle();
                        BM78.status = response.StatusReport_0x81.status;
                        switch (BM78.status) {
                            case BM78_STATUS_STANDBY_MODE:
                            case BM78_STATUS_LINK_BACK_MODE:
                            case BM78_STATUS_LE_CONNECTED_MODE:
                                BM78_tx.length = 0; // Abort ongoing transmission.
                                break;
                            case BM78_STATUS_SPP_CONNECTED_MODE:
                                break;
                            case BM78_STATUS_IDLE_MODE:
                                if (BM78.enforceStandBy) {
                                    BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                                }
                                break;
                            case BM78_STATUS_POWER_ON:
                            case BM78_STATUS_PAGE_MODE:
                            case BM78_STATUS_SHUTDOWN_MODE:
                            default:
                                break;
                        }
                        break;
                    case BM78_OPC_RECEIVED_TRANSPARENT_DATA:
                    case BM78_OPC_RECEIVED_SPP_DATA:
                        chksum = 0;
                        chksumReceived = data[0];
                        for (uint8_t i = 1; i < response.ReceivedTransparentData_0x9A.length - 2; i++) {
                            uint8_t byte = data[i];
                            chksum = chksum + byte;
                        }
                        if (chksum == chksumReceived) {
                            switch (data[1]) {
                                case BM78_MESSAGE_KIND_CRC:
                                    if (response.ReceivedTransparentData_0x9A.length == 5) {
                                        BM78_tx.chksumReceived = data[2];
                                    }
                                    break;
                                default:
                                    for (uint8_t i = 1; i < response.ReceivedTransparentData_0x9A.length - 2; i++) {
                                        uint8_t byte = data[i];
                                        transparentData[i - 1] = byte;
                                    }

                                    if (BM78_TransparentDataHandler) {
                                        BM78_TransparentDataHandler(response.ReceivedTransparentData_0x9A.length - 3, transparentData);
                                    }

                                    BM78_execute(BM78_CMD_SEND_TRANSPARENT_DATA, 4, 0x00, chksum, BM78_MESSAGE_KIND_CRC, chksum);
                                    break;
                            }
                        }
                        break;
                    default:
                        break;
                }

                if (BM78_ResponseHandler) BM78_ResponseHandler(response, data);

            } else {
                if (response.op_code == BM78_OPC_COMMAND_COMPLETE) {
                    switch (response.CommandComplete_0x80.status) {
                        case BM78_ERR_INVALID_COMMAND_PARAMETERS:
                            if (response.CommandComplete_0x80.command == BM78_CMD_INVISIBLE_SETTING) {
                                BM78_execute(BM78_CMD_READ_STATUS, 0);
                            }
                            break;
                        case BM78_ERR_COMMAND_DISALLOWED:
                            BM78_execute(BM78_CMD_READ_STATUS, 0);
                            break;
                        default:
                            break;
                    }
                }
                if (BM78_ErrorHandler) BM78_ErrorHandler(response, data);
            }
            break;
        case BM78_MODE_TEST:
            break;
    }
}

void BM78_write(uint8_t command, uint8_t store, uint8_t *value) {
    uint8_t i, length = 0;
    uint8_t data[0xFF];
    data[0] = store;
    while (value[length++] != '\0');
    for (i = 0; i < length; i++) {
        data[i + 1] = value[i];
    }
    BM78_data(command, length + 1, data);
}

void BM78_execute(uint8_t command, uint16_t length, ...) {
    uint16_t i;
    uint8_t byte, chksum = 0;
    va_list argp;
    va_start(argp, length);
    uint8_t data[0xFF];

    // Data
    for (i = 0; i < length; i++) {
        data[i] = va_arg(argp, uint8_t);
    }

    BM78_data(command, length, data);
    va_end(argp);
}

void BM78_data(uint8_t command, uint16_t length, uint8_t *data) {
    uint16_t i;
    uint8_t byte, chksum = 0;

    BM78_counters.idle = 0; // Reset idle counter

    BM78_clearWriteBuffer();
    BM78_writeBuffer[0] = 0xAA; // Sync word.
    BM78_writeBuffer[1] = (length + 1) >> 8; // Length high byte.
    BM78_writeBuffer[2] = (length + 1) & 0x00FF; // Length low byte.
    BM78_writeBuffer[3] = command; // Add command
    for (i = 1; i < 4; i++) { // Add bytes 1-3 (inclusive) to the checksum.
        chksum += BM78_writeBuffer[i];
    }
    for (i = 0; i < length; i++) { // Append data (inclusive checksum)
        BM78_writeBuffer[i + 4] = *(data + i);
        chksum += *(data + i);
    }
    BM78_writeBuffer[length + 4] = 0xFF - chksum + 1; // Add checksum.
    BM78_sendPacket(length + 5, BM78_writeBuffer); // Send
    BM78_counters.idle = 0; // Reset idle counter
}

void BM78_send(uint8_t length, ...) {
    uint16_t i;
    uint8_t data[0xFF];

    va_list argp;
    va_start(argp, length);

    for (i = 0; i < length; i++) {
        data[i] = va_arg(argp, uint8_t);
    }

    BM78_transmit(length, data);
    va_end(argp);
}

bool BM78_awatingConfirmation(void) {
    return BM78_tx.length > 0;
}

void BM78_transmit(uint16_t length, uint8_t *data) {
    if (BM78_tx.length == 0) {
        BM78_tx.chksumExpected = 0x00;
        BM78_tx.chksumReceived = 0xFF;
        for (uint16_t i = 0; i < length; i++) {
            BM78_tx.chksumExpected = BM78_tx.chksumExpected + *(data + i);
        }
        if (BM78_tx.chksumExpected == BM78_tx.chksumReceived) {
            BM78_tx.chksumReceived = 0x00;
        }

        BM78_tx.data[0] = 0x00; // Reserved by BM78
        BM78_tx.data[1] = BM78_tx.chksumExpected;
        for (uint16_t i = 0; i < length; i++) {
            BM78_tx.data[i + 2] = *(data + i);
        }

        BM78_tx.length = length + 2;
        BM78_tx.timeout = BM78_RESEND_DELAY / BM78_TRIGGER_PERIOD;
        BM78_data(BM78_CMD_SEND_TRANSPARENT_DATA, BM78_tx.length, BM78_tx.data);
    }
}

void BM78_retryTrigger(void) {
    if (BM78_tx.length > 0 && BM78_isChecksumCorrect()) {
        BM78_tx.length = 0;
        if (BM78_MessageSentHandler) {
            BM78_MessageSentHandler();
        }
    } else if (BM78_tx.length > 0 && !BM78_isChecksumCorrect() && BM78_tx.timeout == 0) {
        BM78_tx.timeout = BM78_RESEND_DELAY / BM78_TRIGGER_PERIOD;
        BM78_data(BM78_CMD_SEND_TRANSPARENT_DATA, BM78_tx.length, BM78_tx.data);
    } else if (BM78_tx.timeout > 0) {
        BM78_tx.timeout--;
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
    switch (BM78_rx.state) {
        case BM78_EVENT_STATE_IDLE:
            if (byte == 0xAA) {
                BM78_rx.response.checksum_calculated = 0;
                BM78_rx.state = BM78_EVENT_STATE_LENGTH_HIGH;
            }
            break;
        case BM78_EVENT_STATE_LENGTH_HIGH:
            // Ignoring lengthHigh byte - max length: 255
            BM78_rx.response.length = (byte << 8);
            BM78_rx.response.checksum_calculated += byte;
            BM78_rx.state = BM78_EVENT_STATE_LENGTH_LOW;
            break;
        case BM78_EVENT_STATE_LENGTH_LOW:
            BM78_rx.response.length |= (byte & 0x00FF);
            BM78_rx.response.checksum_calculated += byte;
            BM78_rx.index = 0;
            BM78_rx.state = BM78_EVENT_STATE_OP_CODE;
            break;
        case BM78_EVENT_STATE_OP_CODE:
            BM78_rx.response.op_code = byte;
            BM78_rx.response.checksum_calculated += byte;
            BM78_rx.index++;
            BM78_rx.state = BM78_EVENT_STATE_ADDITIONAL;
            break;
        case BM78_EVENT_STATE_ADDITIONAL:
            if (BM78_rx.index < BM78_rx.response.length) {
                switch (BM78_rx.response.op_code) {
                    case BM78_OPC_PAIRING_COMPLETE:
                        BM78_rx.response.PairingComplete_0x61.result = byte;
                        break;
                    case BM78_OPC_PASSKEY_DISPLAY_YES_NO_REQ:
                        BM78_rx.response.PasskeyDisplayYesNoReq_0x62.passkey = byte;
                        break;
                        // GAP Events
                    case BM78_OPC_LE_CONNECTION_COMPLETE:
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
                    case BM78_OPC_DISCONNECTION_COMPLETE:
                        BM78.status = BM78_STATUS_IDLE_MODE;
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.DisconnectionComplete_0x72.connection_handle = byte;
                        } else if (BM78_rx.index == 2) {
                            BM78_rx.response.DisconnectionComplete_0x72.reason = byte;
                        }
                        break;
                    case BM78_OPC_SPP_CONNECTION_COMPLETE:
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
                    case BM78_OPC_COMMAND_COMPLETE:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.CommandComplete_0x80.command = byte;
                        } else if (BM78_rx.index == 2) {
                            BM78_rx.response.CommandComplete_0x80.status = byte;
                        } else {
                            *(BM78_rx.data + BM78_rx.index - 3) = byte;
                            *(BM78_rx.data + BM78_rx.index - 2) = 0x00;
                        }
                        break;
                    case BM78_OPC_BM77_STATUS_REPORT:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.StatusReport_0x81.status = byte;
                        }
                        break;
                    case BM78_OPC_CONFIGURE_MODE_STATUS:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.ConfigureModeStatus_0x8F.status = byte;
                        }
                        break;
                        // SPP/GATT Transparent Event
                    case BM78_OPC_RECEIVED_TRANSPARENT_DATA:
                    case BM78_OPC_RECEIVED_SPP_DATA:
                        if (BM78_rx.index == 1) {
                            BM78_rx.response.ReceivedTransparentData_0x9A.reserved = byte;
                        } else {
                            *(BM78_rx.data + BM78_rx.index - 2) = byte;
                            *(BM78_rx.data + BM78_rx.index - 1) = 0x00;
                        }
                        break;
                    default:
                        BM78_rx.state = BM78_EVENT_STATE_IDLE;
                        break;
                }
                BM78_rx.response.checksum_calculated += byte;
                BM78_rx.index++;
            } else {
                BM78_rx.response.checksum_received = byte;
                BM78_rx.response.checksum_calculated = 0xFF - BM78_rx.response.checksum_calculated + 1;
                if (BM78_rx.response.checksum_calculated == BM78_rx.response.checksum_received) {
                    BM78_AsyncEventResponse(BM78_rx.response, BM78_rx.data);
                } else if (BM78_ErrorHandler) {
                    BM78_ErrorHandler(BM78_rx.response, BM78_rx.data);
                }
                BM78_rx.state = BM78_EVENT_STATE_IDLE;
            }
            break;
        default:
            BM78_rx.state = BM78_EVENT_STATE_IDLE;
            break;
    }
}

void BM78_processByteInTestMode(uint8_t byte) {
    switch (BM78_cmdRX.state) {
        case BM78_COMMAND_RESPONSE_STATE_IDLE:
            BM78_cmdRX.state = byte == 0x04
                    ? BM78_COMMAND_RESPONSE_STATE_INIT
                    : BM78_COMMAND_RESPONSE_STATE_IDLE;
            break;
        case BM78_COMMAND_RESPONSE_STATE_INIT:
            BM78_cmdRX.state = byte == 0x0e
                    ? BM78_COMMAND_RESPONSE_STATE_LENGTH
                    : BM78_COMMAND_RESPONSE_STATE_IDLE;
            break;
        case BM78_COMMAND_RESPONSE_STATE_LENGTH:
            BM78_cmdRX.index = 0;
            BM78_cmdRX.length = byte;
            BM78_cmdRX.state = BM78_COMMAND_RESPONSE_STATE_DATA;
            //for (uint8_t i = 0; i < 0xFF; i++) BM78_cmdRX.data[i] = 0x00;
            break;
        case BM78_COMMAND_RESPONSE_STATE_DATA:
            BM78_cmdRX.data[BM78_cmdRX.index++] = byte;
            if (BM78_cmdRX.index >= BM78_cmdRX.length) {
                if (BM78_TestModeResponseHandler) {
                    BM78_TestModeResponseHandler(BM78_cmdRX.length, BM78_cmdRX.data);
                }
                BM78_cmdRX.state = BM78_COMMAND_RESPONSE_STATE_IDLE;
            }
            break;
    }
}

void BM78_checkNewDataAsync(void) {
    uint8_t byte;

    if (UART1_is_rx_ready()) {
        BM78_counters.idle = 0; // Reset idle counter
        byte = UART1_Read();
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