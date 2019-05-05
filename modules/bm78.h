/* 
 * File:   bm78.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * BM78 module implementation. To enable it the following PINs needs to be set
 * up:
 *  - BM78_SW_BTN: Output, digital
 *  - BM78_RST_N: Output, digital, start-high
 *  - BM78_WAKE_UP: Output, digital, start-high
 *  - BM78_DISCONNECT: Output, digital, start-high
 *  - BM78_P2_0: Output, digital, start-high
 *  - BM78_P2_4: Output, digital, start-high
 *  - BM78_EAN: Output, digital
 * 
 * then the BM78_ENABLED will be defined and can be used to determine whether
 * BM78 module is present.
 * 
 * See configuration options below.
 */
#ifndef BM78_H
#define	BM78_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../config.h"

// PIN Configuration:
#ifdef BM78_SW_BTN_PORT     // Output
#ifdef BM78_RST_N_PORT      // Output (start high)
//#ifdef BM78_WAKE_UP_PORT    // Output (start high)
//#ifdef BM78_DISCONNECT_PORT // Output (start high)
#ifdef BM78_P2_0_PORT       // Output (start high)
#ifdef BM78_P2_4_PORT       // Output (start high)
#ifdef BM78_EAN_PORT        // Output
#ifndef BM78_DISABLE
#define BM78_ENABLED
#endif
#endif
#endif
#endif
//#endif
//#endif
#endif
#endif

#ifdef BM78_ENABLED

#ifdef TIMER_PERIOD
#define BM78_TRIGGER_PERIOD TIMER_PERIOD // Timer period in ms.
#else
#define BM78_TRIGGER_PERIOD 10 // Timer period in ms.
#endif
#ifndef BM78_RESEND_DELAY
#define BM78_RESEND_DELAY 3000
#endif
#ifndef BM78_INITIALIZATION_TIMOUT
#define BM78_INITIALIZATION_TIMOUT 1500 // Time to let the device setup itself in ms.
#endif
#ifndef BM78_STATUS_CHECK_TIMEOUT
#define BM78_STATUS_CHECK_TIMEOUT 3000 // Time to let the device refresh its status in ms.
#endif
#ifndef BM78_STATUS_MISS_MAX_COUNT
#define BM78_STATUS_MISS_MAX_COUNT 5 // Number of status refresh attempts before reseting the device.
#endif

#ifndef BM78_DATA_PACKET_MAX_SIZE
#define BM78_DATA_PACKET_MAX_SIZE 32
#endif
#define BM78_EEPROM_SIZE 0x1FF0

// Message kinds:
#ifndef BM78_MESSAGE_KIND_CRC
#define BM78_MESSAGE_KIND_CRC 0x00
#endif
#ifndef BM78_MESSAGE_KIND_IDD
#define BM78_MESSAGE_KIND_IDD 0x01
#endif
#ifndef BM78_MESSAGE_KIND_PLAIN
#define BM78_MESSAGE_KIND_PLAIN 0x02
#endif
#ifndef BM78_MESSAGE_KIND_UNKNOWN
#define BM78_MESSAGE_KIND_UNKNOWN 0xFF    
#endif
    

typedef enum {
    //                                                AUTO PATTERN                   MANUAL PATTERN
    // Common
    BM78_CMD_READ_LOCAL_INFORMATION = 0x01,       // F (Configuration Mode)
    BM78_CMD_RESET = 0x02,
    BM78_CMD_READ_STATUS = 0x03,
    BM78_CMD_INTO_POWER_DOWN_MODE = 0x05,
    BM78_CMD_READ_DEVICE_NAME = 0x07,             // F (Configuration Mode)
    BM78_CMD_WRITE_DEVICE_NAME = 0x08,            // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO = 0x09, // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_READ_PAIRING_MODE_SETTING = 0x0A,    // F (Configuration Mode)
    BM78_CMD_WRITE_PAIRING_MODE_SETTING = 0x0B,   // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO = 0x0C,  // F (Configuration Mode)
    BM78_CMD_DELETE_PAIRED_DEVICE = 0x0D,         // F (Configuration Mode)         I (Idle Mode)
    // GAP
    BM78_CMD_READ_RSSI_VALUE = 0x10,              //                                CM (Connected Mode w/ Manual Pattern)
    BM78_CMD_WRITE_ADV_DATA = 0x11,               // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_WRITE_SCAN_RES_DATA = 0x12,          // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_SET_ADV_PARAMETER = 0x13,            // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_DISCONNECT = 0x1B,                   //                                CM (Connected Mode w/ Manual Pattern)
    BM78_CMD_INVISIBLE_SETTING = 0x1C,            //                                I (Idle Mode)
    BM78_CMD_SPP_CREATE_LINK = 0x1D,              //                                I (Idle Mode)
    BM78_CMD_SPP_CREATE_LINK_CANCEL = 0x1E,       //                                I (Idle Mode)
    BM78_CMD_READ_REMOTE_DEVICE_NAME = 0x1F,      //                                CM (Connected Mode w/ Manual Pattern)
    BM78_CMD_BDR_CREATE_CONNECTION = 0x20,
    // SPP/GATT Transparent
    BM78_CMD_SEND_TRANSPARENT_DATA = 0x3A,        //                                CM (Connected Mode w/ Manual Pattern)
    BM78_CMD_SEND_SPP_DATA = 0x3B,                //                                CM (Connected Mode w/ Manual Pattern)
    // Pairing
    BM78_CMD_PASSKEY_ENTRY_RES = 0x40,            // CP (Connected Mode w/ Pairing) CP (Connected Mode w/ Pairing)
    BM78_CMD_USER_CONFIRM_RES = 0x41,             // CP (Connected Mode w/ Pairing) CP (Connected Mode w/ Pairing)
    // Common 2
    BM78_CMD_READ_PIN_CODE = 0x50,                // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_WRITE_PIN_CODE = 0x51,               // F (Configuration Mode)         I (Idle Mode)
    BM78_CMD_LEAVE_CONFIGURE_MODE = 0x52          // F (Configuration Mode)
} BM78_Command_t;

typedef enum {
    // Pairing Events
    BM78_OPC_PASSKEY_ENTRY_REQ = 0x60,
    BM78_OPC_PAIRING_COMPLETE = 0x61,
    BM78_OPC_PASSKEY_DISPLAY_YES_NO_REQ = 0x62,
    // GAP Events
    BM78_OPC_LE_CONNECTION_COMPLETE = 0x71,
    BM78_OPC_DISCONNECTION_COMPLETE = 0x72,
    BM78_OPC_SPP_CONNECTION_COMPLETE = 0x74,
    // Common Events
    BM78_OPC_COMMAND_COMPLETE = 0x80,
    BM78_OPC_BM77_STATUS_REPORT = 0x81,
    BM78_OPC_CONFIGURE_MODE_STATUS = 0x8F,
    // SPP/GATT Transparent Event
    BM78_OPC_RECEIVED_TRANSPARENT_DATA = 0x9A,
    BM78_OPC_RECEIVED_SPP_DATA = 0x9B
} BM78_OPCode_t;
    
// EEPROM
typedef enum {
    BM78_EEPROM_IGNORE = 0x00,
    BM78_EEPROM_STORE = 0x01,
    BM78_EEPROM_BEACON_IGNORE = 0x80,
    BM78_EEPROM_BEACON_STORE = 0x81
} BM78_EEPROM_t;

// Pairing mode
typedef enum {
    BM78_PAIRING_PIN = 0x00,
    BM78_PAIRING_JUST_WORK = 0x01,
    BM78_PAIRING_PASSKEY = 0x02,
    BM78_PAIRING_USER_CONFIRM = 0x03
} BM78_PairingMode_t;

// Pairing user confirm action
typedef enum {
    BM78_PAIRING_USER_CONFIRM_YES = 0x00,
    BM78_PAIRING_USER_CONFIRM_NO = 0x01
} BM78_PairingUserConfirm_t;

// Pairing passkey action
typedef enum {
    BM78_PAIRING_PASSKEY_DIGIT_ENTERED = 0x01,
    BM78_PAIRING_PASSKEY_DIGIT_ERASED = 0x02,
    BM78_PAIRING_PASSKEY_CLEARED = 0x03,
    BM78_PAIRING_PASSKEY_ENTRY_COMPLETED = 0x04
} BM78_PairingPasskey_t;

// Configuration mode
typedef enum {
    BM78_CONFIG_NONE = 0x00,
    BM78_CONFIG_DISABLE_FOREVER = 0x01
} BM78_ConfigMode_t;

// Configuration mode state
typedef enum {
    BM78_CONFIG_MODE_DISABLED = 0x00,
    BM78_CONFIG_MODE_ENABLED = 0x01
} BM78_ConfigModeState_t;

// Pairing response
typedef enum {
    BM78_PAIRING_RESULT_COMPLETE = 0x00,
    BM78_PAIRING_RESULT_FAIL = 0x01,
    BM78_PAIRING_RESULT_TIMEOUT = 0x02
} BM78_PairingResult_t;

// Status
typedef enum {
    BM78_STATUS_POWER_ON = 0x00,
    BM78_STATUS_PAGE_MODE = 0x02,
    BM78_STATUS_STANDBY_MODE = 0x03,
    BM78_STATUS_LINK_BACK_MODE = 0x04,
    BM78_STATUS_SPP_CONNECTED_MODE = 0x07,
    BM78_STATUS_LE_CONNECTED_MODE = 0x08,
    BM78_STATUS_IDLE_MODE = 0x09,
    BM78_STATUS_SHUTDOWN_MODE = 0x0A
} BM78_Status_t;

// Stand-By mode action
typedef enum {
    BM78_STANDBY_MODE_LEAVE = 0x00,
    BM78_STANDBY_MODE_ENTER = 0x01
} BM78_StandByMode_t;
    
// Error codes
typedef enum {
    BM78_COMMAND_SUCCEEDED = 0x00,
    BM78_ERR_UNKNOWN_COMMAND = 0x01,
    BM78_ERR_UNKNOWN_CONNECTION_IDENTIFIER = 0x02,
    BM78_ERR_HARDWARE_FAILURE = 0x03,
    BM78_ERR_AUTHENTICATION_FAILURE = 0x05,
    BM78_ERR_PIN_OR_KEY_MISSING = 0x06,
    BM78_ERR_MEMORY_CAPACITY_EXCEEDED = 0x07,
    BM78_ERR_CONNECTION_TIMEOUT = 0x08,
    BM78_ERR_CONNECTION_LIMIT_EXCEEDED = 0x09,
    BM78_ERR_ACL_CONNECTION_ALREADY_EXISTS = 0x0B,
    BM78_ERR_COMMAND_DISALLOWED = 0x0C,
    BM78_ERR_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES = 0x0D,
    BM78_ERR_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS = 0x0E,
    BM78_ERR_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR = 0x0F,
    BM78_ERR_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED = 0x10,
    BM78_ERR_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE = 0x11,
    BM78_ERR_INVALID_COMMAND_PARAMETERS = 0x12,
    BM78_ERR_REMOTE_USER_TERMINATED_CONNECTION = 0x13,
    BM78_ERR_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES = 0x14,
    BM78_ERR_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF = 0x15,
    BM78_ERR_CONNECTION_TERMINATED_BY_LOCAL_HOST = 0x16,
    BM78_ERR_PAIRING_NOT_ALLOWED = 0x18,
    BM78_ERR_UNSPECIFIED_ERROR = 0x1F,
    BM78_ERR_INSTANT_PASSED = 0x28,
    BM78_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED = 0x29,
    BM78_ERR_INSUFFICIENT_SECURITY = 0x2F,
    BM78_ERR_CONNECTION_REJECTED_DUE_TO_NO_SUITABLE_CHANNEL_FOUND = 0x39,
    BM78_ERR_CONTROLLER_BUSY = 0x3A,
    BM78_ERR_UNACCEPTABLE_CONNECTION_INTERVAL = 0x3B,
    BM78_ERR_DIRECTED_ADVERTISING_TIMEOUT = 0x3C,
    BM78_ERR_CONNECTION_TERMINATED_DUE_TO_MIC_FAILURE = 0x3D,
    BM78_ERR_CONNECTION_FAILED_TO_BE_ESTABLISHED = 0x3E,
    BM78_ERR_INVALID_HANDLE = 0x81,
    BM78_ERR_READ_NOT_PERMITTED = 0x82,
    BM78_ERR_WRITE_NOT_PERMITTED = 0x83,
    BM78_ERR_INVALID_PDU = 0x84,
    BM78_ERR_INSUFFICIENT_AUTHENTICATION = 0x85,
    BM78_ERR_REQUEST_NOT_SUPPORTED = 0x86,
    BM78_ERR_INVALID_OFFSET = 0x77,
    BM78_ERR_INSUFFICIENT_AUTHORIZATION = 0x88,
    BM78_ERR_PREPARE_QUEUE_FULL = 0x89,
    BM78_ERR_ATTRIBUTE_NOT_FOUND = 0x8A,
    BM78_ERR_ATTRIBUTE_NOT_LONG = 0x8B,
    BM78_ERR_INSUFFICIENT_ENCRYPTION_KEY_SIZE = 0x8C,
    BM78_ERR_INVALID_ATTRIBUTE_VALUE_LENGTH = 0x8D,
    BM78_ERR_UNLIKELY_ERROR = 0x8E,
    BM78_ERR_INSUFFICIENT_ENCRYPTION = 0x8F,
    BM78_ERR_UNSUPPORTED_GROUT_TYPE = 0x90,
    BM78_ERR_INSUFFICIENT_RESOURCES = 0x91,
    BM78_ERR_UART_CHECK_SUM_ERROR = 0xFF
} BM78_StatusCode_t;

// Single event states
typedef enum {
    BM78_STATE_IDLE = 0x00,
    BM78_EVENT_STATE_LENGTH_HIGH = 0x01,
    BM78_EVENT_STATE_LENGTH_LOW = 0x02,
    BM78_EVENT_STATE_OP_CODE = 0x03,
    BM78_EVENT_STATE_ADDITIONAL = 0x04,

    BM78_COMMAND_RESPONSE_STATE_INIT = 0x81,
    BM78_COMMAND_RESPONSE_STATE_LENGTH = 0x82,
    BM78_COMMAND_RESPONSE_STATE_DATA = 0x83,

    BM78_STATE_SENDING = 0xFF
} BM78_State_t;

// Modes
typedef enum {
    BM78_MODE_INIT = 0x00,
    BM78_MODE_APP  = 0x01,
    BM78_MODE_TEST = 0xFF
} BM78_Mode_t;

// Transmission States
typedef enum {
    BM78_TRANSMISSION_IDLE = 0x00,
    BM78_TRANSMISSION_TRANSMITING = 0x01,
    BM78_TRANSMISSION_CONFIMATION = 0x02
} BM78_TransmissionState_t;
    
typedef union {
    struct {
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
    };
    struct { // Passkey Entry Req (0x60)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
    } PasskeyEntryReq_0x60;
    struct { // Pairing Complete (0x61)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        BM78_PairingResult_t result;
    } PairingComplete_0x61;
    struct { // Passkey Display YesNo Req (0x62)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        uint8_t passkey;
    } PasskeyDisplayYesNoReq_0x62;
    struct { // LE Connection Complete (0x71)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        uint8_t status;
        uint8_t connection_handle;
        uint8_t role;
        uint8_t peer_address_type;
        uint8_t peer_address[6];
        uint16_t connection_interval;
        uint16_t connection_latency;
        uint16_t supervision_timeout;
    } LEConnectionComplete_0x71;
    struct { // Disconnection Complete (0x72)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        uint8_t connection_handle;
        uint8_t reason;
    } DisconnectionComplete_0x72;
    struct { // SPP Connection Complete (0x74)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        BM78_StatusCode_t status;
        uint8_t connection_handle;
        uint8_t peer_address[6];
    } SPPConnectionComplete_0x74;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        BM78_Command_t command;
        BM78_StatusCode_t status;
        // return parameters (x bytes)
    } CommandComplete_0x80;
    struct { // BM77 Status Report (0x81)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        uint8_t status;
    } StatusReport_0x81;
    struct { // Configure Mode Status (0x8F)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        uint8_t status;
    } ConfigureModeStatus_0x8F;
    struct { // Received Transparent Data (0x9A)
        uint16_t length;
        BM78_OPCode_t op_code;
        uint8_t checksum_calculated;
        uint8_t checksum_received;
        uint8_t reserved;
        // data (n bytes)
    } ReceivedTransparentData_0x9A;
} BM78_Response_t;

typedef struct {
    uint8_t index;      // Index 
    uint8_t priority;   // Priority
    uint8_t address[6]; // Device's address
} BM78_PairedDevice_t;

struct {
    BM78_Mode_t mode;                      // Dongle mode.
    BM78_Status_t status;                  // Application mode status.
    bool enforceStandBy;                   // Whether to enforce Stand-By mode
                                           // on disconnect or idle.
    BM78_PairingMode_t pairingMode;        // Pairing mode.
    uint8_t pairedDevicesCount;            // Paired devices count.
    BM78_PairedDevice_t pairedDevices[16]; // Paired devices MAC addresses.
    char deviceName[16];                   // Device name.
    char pin[7];                           // PIN.
} BM78 = {BM78_MODE_INIT, BM78_STATUS_POWER_ON, true, BM78_PAIRING_PIN, 0};

void BM78_init(uint8_t (* isRXReady)(void),
               uint8_t (* isTXReady)(void),
               uint8_t (* isTXDone)(void),
               uint8_t (* read)(void),
               void (* write)(uint8_t));

/**
 * Powers the device on or off.
 * 
 * @param on Whether the device should be powered on or off
 */
void BM78_power(bool on);

/**
 * Resets the devices.
 */
void BM78_reset(void);

/*
 * Module          | P2_0 P2_4 EAN | Operational Mode 
 * =====================================================================
 * BM78SPPx5NC2    |   0    1   1  | Write EEPROM and test mode
 * (ROM Variant)   |   1    1   1  | Normal operation/application mode
 * ---------------------------------------------------------------------
 * BM78SPPx5MC2    |   0    0   1  | Write FLASH
 * (Flash Variant) |   0    1   0  | Write EEPROM and test mode
 *                 |   1    1   0  | Normal operational/application mode
 */

/** Reset the unit to and end enter the test mode */
void BM78_resetToTestMode(void);

/** Reset the unit and enter the Application mode */
void BM78_resetToAppMode(void);

/** Opens EEPROM */
void BM78_openEEPROM(void);

/** Clears EEPROM */
void BM78_clearEEPROM(void);

/**
 * Read from EEPROM.
 * 
 * @param address Address to read from.
 * @param length Number of bytes to read.
 */
void BM78_readEEPROM(uint16_t address, uint8_t length);

/**
 * Write to EEPROM.
 * 
 * @param address Address to read from.
 * @param length Number of bytes to write.
 * @param data The data to write.
 */
void BM78_writeEEPROM(uint16_t address, uint8_t length, uint8_t *data);

/** Sets the device up. */
void BM78_setup(void);

/**
 * Checks the device's state and react on it.
 * 
 * This function should be called periodically, e.g. by a timer.
 */
void BM78_checkState(void);

/**
 * Sets the function to be called when setup is triggered.
 * 
 * @param SetupHandler The handler.
 */
void BM78_setSetupHandler(void (* SetupHandler)(void));

/**
 * Sets the function to be called when initialization is complete.
 * 
 * @param InitializationHandler The handler which takes 2 parameters: 
 *                              the device's name and pin.
 */
void BM78_setInitializationHandler(void (* InitializationHandler)(char*, char*));

/**
 * Sets event response handler function callback in application mode.
 * 
 * @param ResponseHandler The handler to be called on events.
 *                        Takes 2 parameters: The response (see BM78_Response) 
 *                        and additional data pointer.
 */
void BM78_setAppModeResponseHandler(void (* ResponseHandler)(BM78_Response_t, uint8_t*));

/**
 * Sets the function to be called when response is received in test mode.
 * 
 * @param CommandResponseHandler The handler.
 */
void BM78_setTestModeResponseHandler(void (* CommandResponseHandler)(uint8_t, uint8_t*));

/**
 * Sets event error handler function callback.
 * 
 * @param ErrorHandler The handler to be called on event errors. 
 *                     Takes 2 parameters: The response (see BM78_Response) and 
 *                     additional data pointer.
 */
void BM78_setErrorHandler(void (* ErrorHandler)(BM78_Response_t, uint8_t*));

/**
 * Sets transparent data handler function callback.
 * 
 * @param TransparentDataHandler The handler to be called on incoming
 *                               transparent data. Takes 3 parameters:
 *                               The starting point, the length and the pointer
 *                               to the received data.
 */
void BM78_setTransparentDataHandler(void (* TransparentDataHandler)(uint8_t, uint8_t*));

/**
 * Sets message successfully sent handler function callback.
 * 
 * @param MessageSentHandler The handler to be called on successful message
 *                           transmission.
 */
void BM78_setMessageSentHandler(void (* MessageSentHandler)(void));

/**
 * Get Advertising data.
 * @param data Advertising data.
 */
void BM78_GetAdvData(uint8_t *data);

/**
 * Writes value to EEPROM.
 * @param command Command to execute.
 * @param store Whether to store or not (BM78_EEPROM_IGNORE/BM78_EEPROM_STORE)
 * @param value Pointer to the value
 */
void BM78_write(uint8_t command, uint8_t store, uint8_t length, uint8_t *value);

/**
 * Executes command on the module.
 * 
 * @param command Command to execute
 * @param length Length of additional parameters (0-4)
 * @param ... Additional parameters (max 4)
 */
void BM78_execute(uint8_t command, uint16_t length, ...);

/**
 * Sends data to the module.
 * 
 * @param command Command to use
 * @param length Length of the data
 * @param data Data pointer
 */
void BM78_data(uint8_t command, uint16_t length, uint8_t *data);

/** Whether still awaiting confirmation for last command. */
bool BM78_awatingConfirmation(void);

/**
 * Transmit transparent data over the bluetooth module.
 * 
 * @param length Length of the data
 * @param data Data pointer
 */
void BM78_transmit(uint16_t length, uint8_t *data);

/**
 * Transparent data transmission retry trigger.
 * 
 * This function should be called periodically, e.g. by a timer.
 */
void BM78_retryTrigger(void);

/**
 * Validates checksum of last transparent data reception.
 * 
 * @return Whether the last received checksum is valid with the last transmitted
 *         message
 */
bool BM78_isChecksumCorrect(void);

/**
 * Resets checksum of last transparent data reception.
 */
void BM78_resetChecksum(void);

/**
 * Checks new data asynchronously.
 * 
 * This function should be called periodically, e.g. by a timer.
 * 
 * Data examples:
 * Ready
 * --> 0002 81 09 (Idle Mode)
 *  
 *  
 *  Read Information: Version: 2220000103, Address: 3481F41A4B29
 * <-- 000101
 * --> 000E8001002220000103294B1AF48134
 * 
 * Reset
 * <-- 000102
 * --> 0002 81 09 (Idle Mode)
 * 
 * Read Status: 0x09 Idle Mode
 * <-- 000103
 * --> 0002 81 09 (Idle Mode)
 * 
 * Read Device Name: BM78PT
 * <-- 000107
 * --> 0009800700424D37385054
 * 
 * Write Device Name: Nuklear Football (don't change EEPROM)
 * <-- 0012 08 00 4E 75 6B 6C 65 61 72 20 46 6F 6F 74 62 61 6C 6C
 * --> 0003800800
 * 
 * Write Device Name: Nuklear Football (change EEPROM)
 * <-- 001208014E756B6C65617220466F6F7462616C6C
 * --> 0003800800
 * 
 * Read Device Name: Nuklear Football
 * <-- 000107
 * --> 00138007004E756B6C65617220466F6F7462616C6C
 * 
 * Read Pairing Mode Settings: 0x01 Just-Work
 * <-- 00010A
 * --> 0004800A0001
 * 
 * Write Pairing Mode Settings: 0x03 User Confirm (don't change EEPROM)
 * <-- 00030B0003
 * --> 0003800B00
 * 
 * Write Pairing Mode Settings: 0x03 User Confirm (change EEPROM)
 * <-- 00030B0103
 * --> 0003800B00
 * 
 * Read All Paired Device Information: Index: 00, Priority: 01, Device Address: 0xC0EEFB4BACF7
 * <-- 0001 0C
 * --> 000C 80 0C 00 01 00 01 F7AC4BFBEEC0
 * 
 * Read All Paired Device Information: 
 * * Index: 00, Priority: 01, Device Address: 0xC0EEFB4BACF7
 *  * Index: 01, Priority: 02, Device Address: 0x001A7DDA7113
 * <-- 0001 0C
 *                   C  I1 P1 MAC1         I2 P2 MAC2 
 * --> 0014 80 0C 00 02 00 01 F7AC4BFBEEC0 01 02 1371DA7D1A00
 * 
 * Delete Paired Device at index 0x00
 * <-- 00020D00
 * --> 0003800D00
 * 
 * Erase all paired device information
 * <-- 000109
 * --> 0003800900
 * 
 * Read All Paired Device Information: no devices
 * <-- 00010C
 * --> 0004800C0000
 * 
 * Read PIN Code: 1234
 * <-- 000150
 * --> 000780500031323334
 * 
 * Write PIN Code: 123456 (don't change EEPROM)
 * <-- 00085100313233343536
 * --> 0003805100
 * 
 * Write PIN Code: 123456 (change EEPROM)
 * <-- 00085101313233343536
 * --> 0003805100
 * 
 * Read PIN Code: 123456
 * <-- 000150
 * --> 0009805000313233343536
 * 
 * Visible Settings - Enter Standby Mode
 * <-- 0002 1C 01
 * --> 0003 80 1C 00
 * --> 0002 81 03 (Standby Mode)
 * 
 * <-- 0002 1C 01
 * --> 0003 80 1C 00
 * --> 0002 81 03 (Standby Mode)
 * --> 0011 71 00 80 01 00 13 71 DA 7D 1A 00 00 38 00 00 00 2A
 * 
 * Visible Settings - Enter Standby Mode and only connectable for trusted devices
 * <-- 0002 1C 02
 * --> 0003 80 1C 00
 * --> 0002 81 03 (Standby Mode)
 * 
 * Visible Settings - Leave Standby Mode
 * <-- 0002 1C 00
 * --> 0003 80 1C 00
 * --> 0002 81 09 (Idle Mode)
 * 
 * Pairing failed
 * --> 0002 74 0E
 * 
 * Pairing successful
 * --> 000A 74 00 01 F7 AC 4B FB EE C0 02
 * --> 0003 72 01 13
 * --> 0002 81 03 (Standby Mode)
 * 
 * --> 000A 74 00 01 13 71 DA 7D 1A 00 00
 * --> 0003 72 01 13
 * --> 0002 81 03 (Standby Mode)
 * 
 * Adv Data will be stored in EEPROM: Flag: 0x02 Dual Mode, Local Name: Nuklear Football
 *          OP EE S1 FL DM S2 LN N  u  k  l  e  a  r     F  o  o  t  b  a  l  l
 * <-- 0017 11 01 02 01 02 11 09 4E 75 6B 6C 65 61 72 20 46 6F 6F 74 62 61 6C 6C
 * <-- 0016 11 01 02 01 02 10 09 4E 75 6C 6B 65 61 72 20 46 6F 6F 74 62 61 6C
 * --> 0003 80 11 00
 *             EE       N  u  k  l  e  a  r     F  o  o  t  b  a  l  l
 * <-- 0014 11 01 11 09 4E 75 6C 6B 65 61 72 20 46 6F 6F 74 62 61 6C 6C
 * 
 * BDR Create Connection: Address: C0EEFB4BACF7
 * <-- 0007 20 F7AC4BFBEEC0
 * --> 0002 81 02
 * --> 000A 74 00 01 F7 AC 4B FB EE C0 02
 * --> 0002 81 07 (SPP Connected Mode)
 * 
 * BDR Create Connection: Address: 001167500000 (failed)
 * <-- 0007 20 000050671100
 * --> 0003 80 1D 0C
 * 
 * BDR Create Connection: Address: 001A7DDA7113 (disconnected)
 * <-- 0007201371DA7D1A00
 * --> 0002 81 02
 * --> 000A 74 00 01 13 71 DA 7D 1A 00 02
 * --> 0002 81 03 (Standby Mode)
 * --> 0003 72 01 16
 * --> 0002 81 09 (Idle Mode)
 * 
 * Send "ss"
 * <-- 0004 3A 00 73 73
 * --> 0003803A00
 * 
 * Receive "xx"
 * --> 0004 9A 00 78 78
 * 
 * Receive "ss"
 * --> 0004 9A 00 73 73
 *  
 * Link Back: Device Index: 0x00
 * <-- 0002 1D 00
 * --> 0002 81 04 (Link Back Mode)
 * --> 000A 74 00 01 F7 AC 4B FB EE C0 02
 * --> 0002 81 07 (SPP Connected Mode)
 * 
 * Read Remote Device Name: Xylaria
 * <-- 00011F
 * --> 000A801F0058796C61726961
 * 
 * Disconnect
 * <-- 0002 1B 00
 * --> 0003 72 01 16
 * --> 0002 81 09 (Idle Mode)
 * 
 * HELO        X  H  E  L  O
 * --> 0006 9B 00 48 45 4C 4F
 */
void BM78_checkNewDataAsync(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* BM78_H */
