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

#include "../lib/requirements.h"

#ifdef BM78_ENABLED

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "uart.h"

// PIN Configuration:
//BM78_SW_BTN_PORT     // Output
//BM78_RST_N_PORT      // Output (start high)
//BM78_WAKE_UP_PORT    // Output (start high)
//BM78_DISCONNECT_PORT // Output (start high)
//BM78_P2_0_PORT       // Output (start high)
//BM78_P2_4_PORT       // Output (start high)
//BM78_EAN_PORT        // Output

#ifndef BM78_TRIGGER_PERIOD
#ifndef TIMER_PERIOD
#error "BM78: BM78_TRIGGER_PERIOD or TIMER_PERIOD is required!"
#endif
#warning "BM78: Trigger period defaults to TIMER_PERIOD"
#define BM78_TRIGGER_PERIOD TIMER_PERIOD // Timer period in ms.
#endif

#ifndef BM78_INIT_CMD_TIMEOUT
#warning "BM78: Initialization timout defaults to 1500ms"
#define BM78_INIT_CMD_TIMEOUT 1500 // Time to let the device setup itself in ms.
#endif

#ifndef BM78_STATUS_REFRESH_INTERVAL
#warning "BM78: Status check timeout defaults to 3000ms"
#define BM78_STATUS_REFRESH_INTERVAL 3000 // Time to let the device refresh its status in ms.
#endif

#ifndef BM78_STATUS_MISS_MAX_COUNT
#warning "BM78: Maximum status updates missed count defaults to 5"
#define BM78_STATUS_MISS_MAX_COUNT 5 // Number of status refresh attempts before reseting the device.
#endif

#define BM78_EEPROM_SIZE 0x1FF0

typedef enum {
    // F  = Configuration Mode (Auto Pattern only)
    // I  = Idle Mode
    // CM = Connected Mode w/ Manual Pattern)
    // CP = Connected Mode w/ Pairing
    //                                                      AUTO, MANUAL PATTERN
    // Common
    BM78_CMD_READ_LOCAL_INFORMATION = 0x01,              // F
    BM78_CMD_RESET = 0x02,                               // N/A
    BM78_CMD_READ_STATUS = 0x03,                         // N/A
    //BM78_CMD_READ_ADC_VALUE = 0x04,                      // N/A
    BM78_CMD_INTO_POWER_DOWN_MODE = 0x05,                // N/A
    //BM78_CMD_DEBUG = 0x06,                               // N/A
    BM78_CMD_READ_DEVICE_NAME = 0x07,                    // F
    BM78_CMD_WRITE_DEVICE_NAME = 0x08,                   // F        I
    BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO = 0x09,        // F        I
    BM78_CMD_READ_PAIRING_MODE_SETTING = 0x0A,           // F
    BM78_CMD_WRITE_PAIRING_MODE_SETTING = 0x0B,          // F        I
    BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO = 0x0C,         // F
    BM78_CMD_DELETE_PAIRED_DEVICE = 0x0D,                // F        I
    //BM78_CMD_DIO_CONTROL = 0x0E,                         // N/A
    //BM78_CMD_PWM_CONTROL = 0x0F,                         // N/A
    // GAP
    BM78_CMD_READ_RSSI_VALUE = 0x10,                     //          CM
    BM78_CMD_WRITE_ADV_DATA = 0x11,                      // F        I
    BM78_CMD_WRITE_SCAN_RES_DATA = 0x12,                 // F        I
    BM78_CMD_SET_ADV_PARAMETER = 0x13,                   // F        I
    //BM78_CMD_SET_SCAN_PARAMETER = 0x15,                  // N/A      I
    BM78_CMD_SET_SCAN_ENABLE = 0x16,                     // N/A      I
    //BM78_CMD_LE_CREATE_CONNECTION = 0x17,                // N/A      I
    //BM78_CMD_LE_CREATE_CONNECTION_CANCEL = 0x18,         // N/A
    //BM78_CMD_CONNECTION_PARAMETER_UPDATE_REQ = 0x19,     // N/A      CM
    BM78_CMD_DISCONNECT = 0x1B,                          // N/A      CM
    BM78_CMD_INVISIBLE_SETTING = 0x1C,                   // N/A      I
    BM78_CMD_SPP_CREATE_LINK = 0x1D,                     // N/A      I
    BM78_CMD_SPP_CREATE_LINK_CANCEL = 0x1E,              // N/A      I
    BM78_CMD_READ_REMOTE_DEVICE_NAME = 0x1F,             // N/A      CM
    //BM78_CMD_BDR_CREATE_CONNECTION = 0x20,               //
    // GATT CLIENT
    //BM78_CMD_DISCOVER_ALL_PRIMARY_SERVICES = 0x30,       // N/A      CM
    //BM78_CMD_DISCOVER_SPECIFIC_PRIMARY_SERVICE_CHARACTERISTICS = 0x31,// N/A  CM
    //BM78_CMD_READ_CHARACTERISTIC_VALUE = 0x32,           // N/A      CM
    //BM78_CMD_READ_USING_CHARACTERISTIC_UUID = 0x33,      // N/A      CM
    //BM78_CMD_WRITE_CHARACTERISTIC_VALUE = 0x34,          // N/A      CM
    //BM78_CMD_ENABLE_TRANSPARENT = 0x35,                  // N/A      CM
    // GATT SERVER
    //BM78_CMD_SEND_CHARACTERISTIC_VALUE = 0x38,           // N/A      CM
    //BM78_CMD_UPDATE_CHARACTERISTIC_VALUE = 0x39,         // N/A
    //BM78_CMD_READ_LOCAL_CHARACTERISTIC_VALUE = 0x3A,     // N/A
    //BM78_CMD_READ_LOCAL_ALL_PRIMARY_SERVICE = 0x3B,      // N/A
    //BM78_CMD_READ_LOCAL_SPECIFIC_PRIMARY_SERVICE = 0x3C, // N/A
    //BM78_CMD_SEND_WRITE_RESPONSE = 0x3D,                 // N/A      CM
    // GATT TRANSPARENT
    //BM78_CMD_SEND_GATT_TRANSPARENT_DATA = 0x3F,          // N/A      CM
    // SPP
    BM78_CMD_SEND_TRANSPARENT_DATA = 0x3A,               // N/A      CM
    BM78_CMD_SEND_SPP_DATA = 0x3B,                       // N/A      CM
#ifdef BM78_ADVANCED_PAIRING
    // Pairing
    BM78_CMD_PASSKEY_ENTRY_RES = 0x40,                   // CP       CP
    BM78_CMD_USER_CONFIRM_RES = 0x41,                    // CP       CP
    BM78_CMD_PAIRING_REQUEST = 0x42,                     // N/A      CM
#endif
    // Common 2
    BM78_CMD_READ_PIN_CODE = 0x50,                       // F        I
    BM78_CMD_WRITE_PIN_CODE = 0x51,                      // F        I
    BM78_CMD_LEAVE_CONFIGURE_MODE = 0x52                 // F        N/A
} BM78_CommandOpCode_t;

typedef enum {
    // Pairing Events
#ifdef BM78_ADVANCED_PAIRING
    BM78_EVENT_PASSKEY_ENTRY_REQ = 0x60,
#endif
    BM78_EVENT_PAIRING_COMPLETE = 0x61,
#ifdef BM78_ADVANCED_PAIRING
    BM78_EVENT_PASSKEY_DISPLAY_YES_NO_REQ = 0x62,
#endif

    // GAP Events
    //BM78_EVENT_ADVERTISING_REPORT = 0x70,
    BM78_EVENT_LE_CONNECTION_COMPLETE = 0x71,
    BM78_EVENT_DISCONNECTION_COMPLETE = 0x72,
    //BM78_EVENT_CONNECTION_PAREMETER_UPDATE_NOTIFY = 0x73,
    BM78_EVENT_SPP_CONNECTION_COMPLETE = 0x74,

    // Common Events
    BM78_EVENT_COMMAND_COMPLETE = 0x80,
    BM78_EVENT_STATUS_REPORT = 0x81,
#ifndef BM78_MANUAL_MODE
    BM78_EVENT_CONFIGURE_MODE_STATUS = 0x8F,
#endif

    // GATT Client
    //BM78_EVENT_DISCOVER_ALL_PRIMARY_SERVICES_RES = 0x90,
    //BM78_EVENT_DISCOVER_SPECIFIC_PRIMARY_SERVICE_CHARACTERISTIC_RES = 0x91,
    //BM78_EVENT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_RES = 0x92,
    //BM78_EVENT_CHARACTERISTIC_VALUE_RECEIVED = 0x93,

    // GATT Server
    //BM78_EVENT_CLIENT_WRITE_CHARACTERISTIC_VALUE = 0x98,

    // SPP/GATT Transparent
    BM78_EVENT_RECEIVED_TRANSPARENT_DATA = 0x9A,
    BM78_EVENT_RECEIVED_SPP_DATA = 0x9B
} BM78_EventOpCode_t;

typedef enum {
    BM78_ISSC_OCF_OPEN = 0x03,
    BM78_ISSC_OCF_WRITE = 0x27,
    BM78_ISSC_OCF_READ = 0x29,
    BM78_ISSC_OCF_CLEAR = 0x2D
} BM78_ISSC_OCF_t;

typedef enum {
    BM78_ISSC_OGF_COMMAND = 0x0C,
    BM78_ISSC_OGF_OPERATION = 0xFC
} BM78_ISSC_OGF_t; 

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
    BM78_PAIRING_USER_CONFIRM = 0x03,

    //BM78_PAIRING_DISPLAY_ONLY = 0x00,
    //BM78_PAIRING_DISPLAY_YES_NO = 0x01,
    //BM78_PAIRING_KEYBOARD_ONLY = 0x02,
    //BM78_PAIRING_NO_INPUT_NO_OUTPUT = 0x03,
    //BM78_PAIRING_KEYBOARD_DISPLAY = 0x04
} BM78_PairingMode_t;

#ifdef BM78_ADVANCED_PAIRING
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
#endif

// Configuration mode
typedef enum {
    BM78_CONFIG_NONE = 0x00,
    BM78_CONFIG_DISABLE_FOREVER = 0x01
} BM78_ConfigMode_t;

#ifndef BM78_MANUAL_MODE
// Configuration mode state
typedef enum {
    BM78_CONFIG_MODE_NONE = 0x00,
    BM78_CONFIG_MODE_DISABLE_FOREVER = 0x01
} BM78_ConfigOption_t;
typedef enum {
    BM78_CONFIG_MODE_DISABLED = 0x00,
    BM78_CONFIG_MODE_ENABLED = 0x01
} BM78_ConfigModeState_t;
#endif

// Pairing response
typedef enum {
    BM78_PAIRING_RESULT_COMPLETE = 0x00,
    BM78_PAIRING_RESULT_FAIL = 0x01,
    BM78_PAIRING_RESULT_TIMEOUT = 0x02
} BM78_PairingResult_t;

// Status
typedef enum {
    BM78_STATUS_POWER_ON = 0x00,
    //BM78_STATUS_SCANNING_MODE = 0x01,
    BM78_STATUS_PAGE_MODE = 0x02,
    BM78_STATUS_STANDBY_MODE = 0x03,
    BM78_STATUS_LINK_BACK_MODE = 0x04,
    //BM78_STATUS_BROADCAST_MODE = 0x05,
    BM78_STATUS_SPP_CONNECTED_MODE = 0x07,
    BM78_STATUS_LE_CONNECTED_MODE = 0x08, // Transparent service enabled
    BM78_STATUS_IDLE_MODE = 0x09,
    BM78_STATUS_SHUTDOWN_MODE = 0x0A,
    BM78_STATUS_CONFIGURE_MODE = 0x0B,
    BM78_STATUS_PAIRING_IN_PROGRESS_MODE = 0x0C // Reverse engineered stated
    //BM78_STATUS_BLE_CONNECTED_MODE = 0x0C
} BM78_Status_t;

// Stand-By mode action
typedef enum {
    BM78_STANDBY_MODE_LEAVE = 0x00,
    BM78_STANDBY_MODE_ENTER = 0x01,
    BM78_STANDBY_MODE_ENTER_ONLY_TRUSTED = 0x02,
    BM78_STANDBY_BEACON_MODE_ENTER = 0x81,
    BM78_STANDBY_BEACON_MODE_ENTER_ONLY_TRUSTED = 0x82
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
    BM78_ERR_APPLICATION_DEFINED_ERROR = 0xF0,
    BM78_ERR_UART_CHECK_SUM_ERROR = 0xFF
} BM78_StatusCode_t;

//typedef enum {
//    BM78_GATT_ERR_CODE_NO_ERROR = 0x00,
//    BM78_GATT_ERR_CODE_INVALID_HANDLE = 0x01,
//    BM78_GATT_ERR_CODE_READ_NOT_PERMITTED = 0x02,
//    BM78_GATT_ERR_CODE_WRITE_NOT_PERMITTED = 0x03,
//    BM78_GATT_ERR_CODE_INVALID_PDU = 0x04,
//    BM78_GATT_ERR_CODE_INSUFFICIENT_AUTHENTICATION = 0x05,
//    BM78_GATT_ERR_CODE_REQUEST_NOT_SUPPORTED = 0x06,
//    BM78_GATT_ERR_CODE_INVALID_OFFSET = 0x07,
//    BM78_GATT_ERR_CODE_INSUFFICIENT_AUTHORIZATION = 0x08,
//    BM78_GATT_ERR_CODE_PREPARE_QUEUE_FULL = 0x09,
//    BM78_GATT_ERR_CODE_ATTRIBUTE_NOT_FOUND = 0x0A,
//    BM78_GATT_ERR_CODE_ATTRIBUTE_NOT_LONG = 0x0B,
//    BM78_GATT_ERR_CODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE = 0x0C,
//    BM78_GATT_ERR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH = 0x0D,
//    BM78_GATT_ERR_CODE_UNLIKELY_ERROR = 0x0E,
//    BM78_GATT_ERR_CODE_INSUFFICIENT_ENCRYPTION = 0x0F,
//    BM78_GATT_ERR_CODE_UNSUPPORTED_GROUP_TYPE = 0x10,
//    BM78_GATT_ERR_CODE_INSUFFICIENT_RESOURCES = 0x11
//    //BM78_GATT_ERR_CODE_RESERVED_0x = 0x12 ? 0x7F, // Reserved
//    //BM78_GATT_ERR_CODE_APP_ = 0x80 ? 0x9F, // Application defined errors
//    //BM78_GATT_ERR_CODE_RESERVED_0x = 0xA0 ? 0xDF, // Reserved
//    //BM78_GATT_ERR_CODE_COMMON_ = 0xE0 ? 0xFF, // Common Profile and Service Error Codes 
//} BM78_GattErrorCode_t;

typedef enum {
    BM78_CONFIG_MODE_STATUS_DISABLED = 0x00,
    BM78_CONFIG_MODE_STATUS_ENABLED = 0x01
} BM78_ConfigModeStatus_t;

typedef enum {
    BM78_ISSC_STATUS_SUCCESS = 0x00,
    BM78_ISSC_STATUS_ERROR = 0x01
} BM78_ISSC_StatusCode_t;

// Single event states
typedef enum {
    BM78_STATE_IDLE = 0x00,
    BM78_EVENT_STATE_LENGTH_HIGH = 0x01,
    BM78_EVENT_STATE_LENGTH_LOW = 0x02,
    BM78_EVENT_STATE_OP_CODE = 0x03,
    BM78_EVENT_STATE_ADDITIONAL = 0x04,

    BM78_ISSC_EVENT_STATE_INIT = 0x81,
    BM78_ISSC_EVENT_STATE_LENGTH = 0x82,
    BM78_ISSC_EVENT_STATE_PACKET_TYPE = 0x83,
    BM78_ISSC_EVENT_STATE_OCF = 0x84,
    BM78_ISSC_EVENT_STATE_OGF = 0x85,
    BM78_ISSC_EVENT_STATE_STATUS = 0x86,
    BM78_ISSC_EVENT_STATE_DATA_ADDRESS_HIGH = 0x87,
    BM78_ISSC_EVENT_STATE_DATA_ADDRESS_LOW = 0x88,
    BM78_ISSC_EVENT_STATE_DATA_LENGTH = 0x89,
    BM78_ISSC_EVENT_STATE_DATA = 0x8A
} BM78_EventState_t;

// Modes
typedef enum {
    BM78_MODE_INIT = 0x00,
    BM78_MODE_APP  = 0x01,
    BM78_MODE_TEST = 0xFF
} BM78_Mode_t;

typedef enum {
    /**
     * Connectable undirected advertising. It is used to make BM77 into standby 
     * mode.
     */
    BM78_ADV_IND = 0x00,
    /**
     * Connectable directed advertising. It is used to make BM77 into link back 
     * mode.
     */
    BM78_ADV_DIRECT_IND = 0x01,
    /**
     * Scannable undirected advertising. It is used to make BLEDK into broadcast
     * mode. And it will reply advertising packet only for the observer passive
     * scanning or active scanning to receive advertising events.
     */
    BM78_ADV_SCAN_IND = 0x02,
    /** 
     * Non connectable undirected advertising. It is used to make BM77 into
     * broadcast mode.
     */
    BM78_ADV_NONCONN_IND = 0x03,
    /**
     * Proprietary Beacon Setting
     */
    BM78_SCAN_RSP = 0x04
} BM78_AdvertisingEventType_t;

typedef enum {
    BM78_ADDRESS_TYPE_PUBLIC = 0x00,
    BM78_ADDRESS_TYPE_RANDOM = 0x01,
    BM78_ADDRESS_TYPE_PAIRED = 0x02
} BM78_AddressType_t;

typedef enum {
    BM78_ROLE_MASTER = 0x00,
    BM78_ROLE_SLAVE = 0x01
} BM78_Role_t;

//typedef enum {
//    BM78_CHARACTERISTIC_DESCRIPTOR_16BIT= 0x01,
//    BM78_CHARACTERISTIC_DESCRIPTOR_128BIT= 0x02
//} BM78_CharacteristicDescriptorFormat_t;

//typedef enum {
//    BM78_SCAN_TYPE_PASSIVE = 0x00,
//    BM78_SCAN_TYPE_ACTIVE = 0x01
//} BM78_ScanType_t;

//typedef enum {
//    BM78_CHARACTERISTIC_WRITE_TYPE_WITH_RESPONSE = 0x00,
//    BM78_CHARACTERISTIC_WRITE_TYPE_WITHOUT_RESPONSE = 0x01
//} BM78_CharacteristicWriteType_t;

typedef struct {
    uint8_t index;      // Index 
    uint8_t priority;   // Priority
    uint8_t address[6]; // Device's address
} BM78_PairedDevice_t;

//typedef struct {
//    uint16_t startGroupHandle;
//    uint16_t endGroupHandle;
//    uint8_t serviceUUID[16];
//} BM78_AttributeData_t;

//typedef struct {
//    bool characteristicPropertyBroadcast :1;
//    bool characteristicPropertyRead :1;
//    bool characteristicPropertyWriteWithoutResponse :1;
//    bool characteristicPropertyWrite :1;
//    bool characteristicPropertyNotify :1;
//    bool characteristicPropertyIndicate :1;
//    bool characteristicPropertyAuthenticatedSignedWrites :1;
//    bool characteristicPropertyExtendedProperties :1;
//    uint16_t characteristicValue;
//    uint8_t characteristicUUID[16];
//} BM78_Characteristic_t;

//typedef struct {
//    uint16_t attributeHandle;
//    BM78_Characteristic_t characteristic;
//} BM78_Attribute_t;

//typedef struct {
//    uint16_t handle;
//    uint8_t uuid[16];
//} BM78_CharacteristicDescriptor_t;

#define BM78_DATA_MAX_SIZE SCOM_MAX_PACKET_SIZE + 8 > 32 ? SCOM_MAX_PACKET_SIZE + 8 : 32

typedef union {
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t data[BM78_DATA_MAX_SIZE];
    };
    // Common Commands
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_EEPROM_t storeOption;
        uint8_t deviceName[16];
    } WriteDeviceName_0x08;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_EEPROM_t storeOption;
        BM78_PairingMode_t mode;
    } WritePairingModeSetting_0x0B;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t deviceIndex; // Range: 0-7
    } DeletePairedDevice_0x0D;
    // GAP Commands
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint16_t connectionHandle;
    } ReadRSSI_0x10;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_EEPROM_t storeOption;
        uint8_t advertisingData[31];
    } WriteAdvData_0x11;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_EEPROM_t storeOption;
        uint8_t scanResponseData[31];
    } WriteScanResData_0x12;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint16_t advertisingInterval; // Range: 0x0020 to 0x4000, Default: N = 0x0800 (1.28 second), Time = N * 0.625 msec, Time Range: 20 ms to 10.24 sec
        BM78_AdvertisingEventType_t advertisingType;
        BM78_AddressType_t directAddressType;
        uint8_t directAddress[6];
    } SetAdvertisingParameter_0x13;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint16_t scanInterval; // Range: 0x0004 to 0x4000, Default: 0x0010 (10 ms), Time = N * 0.625 msec, Time Range: 2.5 msec to 10.24 seconds
    //    uint16_t scanWindow;   // Range: 0x0004 to 0x4000, Default: 0x0010 (10 ms), Time = N * 0.625 msec, Time Range: 2.5 msec to 10240 msec
    //    BM78_ScanType_t scanType;
    //} SetScanParameter_0x15;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    bool scanEnable;
    //    bool filterDuplicate;
    //} SetScanEnable_0x16;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t filterPolicy;
    //    BM78_AddressType_t peerAddressType;
    //    uint8_t peerAddress[6];
    //} LECreateConnection_0x17;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    uint16_t connectionInterval; // Conn_Interval_Max., Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds.
    //    uint16_t connectionLatency;  // Range: 0x0000 to 0x01F4
    //    uint16_t supervisionTimeout; // Range: 0x000A to 0x0C80, Time = N * 10 msec, Time Range: 100 msec to 32 seconds 
    //} ConnectionParameterUpdateReq_0x19;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_StandByMode_t mode;
    } InvisibleSettings_0x1C;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t deviceIndex;
    } SPPCreateLink_0x1D;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t connectionHandle;
    } ReadRemoteDeviceName_0x1F;
    // GATT Client Commands
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //} DiscoverAllPrimaryServices_0x30;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    uint8_t serviceUUID[16];
    //} DiscoverSpecificPrimaryServiceCharacteristics_0x31;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    uint16_t characteristicValueHandle;
    //} ReadCharacteristicValue_0x32;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    uint8_t characteristicUUID[16];
    //} ReadByCharacteristicUUID_0x33;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    BM78_CharacteristicWriteType_t writeType;
    //    uint16_t characteristicValueHandle;
    //    uint8_t characteristicValue[20];
    //} WriteCharacteristicValue_0x34;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    bool serverTransparentCtrl;
    //    uint8_t clientTransparentMode;
    //} EnableTransparent_0x35;
    // GATT Server Commands
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    uint16_t characteristicValueHandle;
    //    uint8_t characteristicValue[20];
    //} SendCharacteristicValue_0x38;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint16_t characteristicValueHandle;
    //    uint8_t characteristicValue[20];
    //} UpdateCharacteristicValue_0x39;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint16_t characteristicValueHandle;
    //} ReadLocalCharacteristicValue_0x3A;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t reserved;
    //    uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
    //} SendTransparentData_0x3A;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t serviceUUID[16];
    //} ReadLocalSpecificPrimaryService_0x3C;
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    BM78_CommandOpCode_t requestOpCode;
    //    uint16_t attributeHandle;
    //    BM78_GattErrorCode_t errorCode;
    //} SendWriteResponse_0x3D;
    // GATT Transparent Commands
    //struct {
    //    uint8_t length;
    //    BM78_CommandOpCode_t opCode;
    //    uint8_t connectionHandle;
    //    uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
    //} GATTSendTransparentData_0x3F;
    // SPP Transparent Commands
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t reserved;
        uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
    } SPPSendTransparentData_0x3A;
    // Pairing Commands
#ifdef BM78_ADVANCED_PAIRING
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t connectionHandle;
        BM78_PairingPasskey_t notificationType;
        uint8_t enteredPasskey;
    } PasskeyEntryRes_0x40;
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t connectionHandle;
        BM78_PairingUserConfirm_t notificationType;
    } UserConfirmRes_0x41;
#endif
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        uint8_t connectionHandle;
    } PairingRequest_0x42;
    // Common 2 Commands
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_EEPROM_t storeOption;
        uint8_t pin[6];
    } WritePinCode_0x51;
#ifndef BM78_MANUAL_MODE
    struct {
        uint8_t length;
        BM78_CommandOpCode_t opCode;
        BM78_ConfigOption_t option;
    } LeaveConfigureMode_0x52;
#endif
} BM78_Request_t;

typedef union {
    struct {
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t data[SCOM_MAX_PACKET_SIZE + 9];
    };
#ifdef BM78_ADVANCED_PAIRING
    struct { // Passkey Entry Req (0x60)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t connectionHandle;
    } PasskeyEntryReq_0x60;
#endif
    struct { // Pairing Complete (0x61)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t connectionHandle;
        BM78_PairingResult_t result;
    } PairingComplete_0x61;
#ifdef BM78_ADVANCED_PAIRING
    struct { // Passkey Display YesNo Req (0x62)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t passkey[6];
    } PasskeyDisplayYesNoReq_0x62;
#endif
    struct {
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_AdvertisingEventType_t eventType;
        BM78_AddressType_t addressType;
        uint8_t address[6];
        uint8_t dataLength;
        uint8_t data[0x20];
        int rssi;
    } AdvertisingReport_0x70;
    struct { // LE Connection Complete (0x71)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t status;
        uint8_t connectionHandle;
        BM78_Role_t role;
        BM78_AddressType_t peerAddressType;
        uint8_t peerAddress[6];
        uint16_t connectionInterval; // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4000 msec. 
        uint16_t connectionLatency;  // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4000 msec. 
        uint16_t supervisionTimeout; // Range: 0x000A to 0x0C80, Time = N * 10 msec, Time Range: 100 msec to 32 second
    } LEConnectionComplete_0x71;
    struct { // Disconnection Complete (0x72)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t connectionHandle;
        uint8_t reason;
    } DisconnectionComplete_0x72;
    //struct {
    //    uint16_t length;
    //    BM78_EventOpCode_t opCode;
    //    uint8_t checksum;
    //    uint8_t connectionHandle;
    //    uint16_t connectionInterval; // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4000 msec. 
    //    uint16_t connectionLatency;  // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4000 msec. 
    //    uint16_t supervisionTimeout; // Range: 0x000A to 0x0C80, Time = N * 10 msec, Time Range: 100 msec to 32 second
    //} ConnectionParameterUpdateNotify_0x73;
    struct { // SPP Connection Complete (0x74)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_StatusCode_t status;
        uint8_t connectionHandle;
        uint8_t peerAddress[6];
    } SPPConnectionComplete_0x74;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
        // return parameters (x bytes)
    } CommandComplete_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t version[5];
        uint8_t bluetoothAddress[6];
        //uint8_t hwVersion;
    } LocalInformation_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        char deviceName[16];
    } DeviceName_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        BM78_PairingMode_t mode;
    } PairingMode_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t count;
        BM78_PairedDevice_t devices[8];
    } PairedDevicesInformation_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t value;
    } RSSI_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t name[16];
    } RemoteDeviceName_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t value[20];
    } Characteristic_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint16_t handle;
        uint8_t value[20];
    } CharacteristicByUUID_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t value[20];
    } LocalCharacteristic_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
    } TransparentData_0x80;
    struct { // Command Complete (0x80)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_CommandOpCode_t command;
        BM78_StatusCode_t status;
        char value[6];
    } PIN_0x80;
    struct { // BM77 Status Report (0x81)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_Status_t status;
    } StatusReport_0x81;
    struct { // Configure Mode Status (0x8F)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        BM78_ConfigModeStatus_t status;
    } ConfigureModeStatus_0x8F;
    //struct { // Discover All Primary Services Result (0x90)
    //    uint16_t length;
    //    BM78_EventOpCode_t opCode;
    //    uint8_t checksum;
    //    uint8_t connectionHandle;
    //    uint8_t attributeLength;
    //    BM78_AttributeData_t attributeData[30]; // The value 30 was guessed :(
    //} DiscoverAllPrimaryServicesRes_0x90;
    //struct { // Discover Specific Primary Service Characteristic Result (0x91)
    //    uint16_t length;
    //    BM78_EventOpCode_t opCode;
    //    uint8_t checksum;
    //    uint8_t connectionHandle;
    //    uint8_t attributeLength;
    //    BM78_Attribute_t attribute[30]; // The value 30 was guessed :(        
    //} DiscoverSpecificPrimaryServiceCharacteristicRes_0x91;
    //struct { // Discover All Characteristic Descriptors Result (0x92)
    //    uint16_t length;
    //    BM78_EventOpCode_t opCode;
    //    uint8_t checksum;
    //    uint8_t connectionHandle;
    //    BM78_CharacteristicDescriptorFormat_t format;
    //    BM78_CharacteristicDescriptor_t descriptor[30]; // The value 30 was guessed :(
    //} DiscoverAllCharacteristicDescriptorsResult_0x92;
    //struct { // Characteristic Value Received (0x93)
    //    uint16_t length;
    //    BM78_EventOpCode_t opCode;
    //    uint8_t checksum;
    //    uint8_t connectionHandle;
    //    uint16_t valueHandler;
    //    uint8_t value[20];
    //} CharacteristicValueReceived_0x93;
    //struct { // Client Write Characteristic Value (0x98)
    //    uint16_t length;
    //    BM78_EventOpCode_t opCode;
    //    uint8_t checksum;
    //    uint8_t connectionHandle;
    //    uint16_t valueHandler;
    //    uint8_t value[20];
    //} ClientWriteCharacteristicValue_0x98;
    struct { // Received Transparent Data (0x9A)
        uint16_t length;
        BM78_EventOpCode_t opCode;
        uint8_t checksum;
        uint8_t reserved;
        uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
    } ReceivedTransparentData_0x9A;
    // ISSC Events
    struct {
        uint8_t length;
        uint8_t packetType;
        BM78_ISSC_OCF_t ocf;
        BM78_ISSC_OGF_t ogf;
        BM78_ISSC_StatusCode_t status;
    } ISSC_Event;
    struct {
        uint8_t length;
        uint8_t packetType;
        BM78_ISSC_OCF_t ocf;
        BM78_ISSC_OGF_t ogf;
        BM78_ISSC_StatusCode_t status;
        uint16_t address;
        uint8_t dataLength;
        uint8_t data[SCOM_MAX_PACKET_SIZE + 7];
    } ISSC_ReadEvent;
} BM78_Response_t;

struct {
    BM78_Mode_t mode;                      // Dongle mode.
    BM78_Status_t status;                  // Application mode status.
    uint8_t enforceState;                  // Whether to enforce Stand-By mode
                                           // on disconnect or idle.
    uint8_t connectionHandle;              // Connection handle
    BM78_PairingMode_t pairingMode;        // Pairing mode.
    uint8_t pairedDevicesCount;            // Paired devices count.
    BM78_PairedDevice_t pairedDevices[16]; // Paired devices MAC addresses.
    char deviceName[17];                   // Device name.
    char pin[7];                           // PIN.
} BM78 = {BM78_MODE_INIT, BM78_STATUS_POWER_ON, BM78_STANDBY_MODE_ENTER, 0x00, BM78_PAIRING_PIN, 0};

uint8_t BM78_advData[22];

typedef void (*BM78_SetupHandler_t)(char *deviceName, char *pin);
typedef void (*BM78_EventHandler_t)(BM78_Response_t *response);

/**
 * BM78 Initialization.
 * 
 * @param setupAttemptHandler Triggered on each setup attempt.
 * @param setupHandler Triggered on setup success. The takes 2 parameters: 
 *                     the device's name and pin.
 * @param eventHandler Triggered on successful event.
 * @param testModeResponseHandler Triggered on command response in test mode.
 * @param transparentDataHandler Triggered on transparent data in addition and
 *                               before the defined eventHandler.
 * @param messageSentHandler Triggered on successful message sent.
 * @param errorHandler Triggered in event retrieval error.
 */
void BM78_initialize(
                UART_Connection_t uart,
                BM78_SetupHandler_t setupHandler,
                BM78_EventHandler_t appModeEventHandler,
                BM78_EventHandler_t testModeEventHandler,
                DataHandler_t transparentDataHandler,
                Procedure_t cancelTransmissionHandler,
                BM78_EventHandler_t appModeErrorHandler,
                BM78_EventHandler_t testModeErrorHandler);

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

void BM78_resetTo(BM78_Mode_t mode);

/**
 * Setup BM78
 * 
 * @param keep Keep BM78's device name, PIN and pairing mode.
 */
void BM78_setup(bool keep);

/** Opens EEPROM */
inline void BM78_openEEPROM(void);

/** Clears EEPROM */
inline void BM78_clearEEPROM(void);

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

/**
 * Checks the device's state and react on it.
 * 
 * This function should be called periodically, e.g. by a timer.
 */
void BM78_checkState(void);

/**
 * Writes value to EEPROM.
 *
 * @param command Command to execute.
 * @param store Whether to store or not (BM78_EEPROM_IGNORE/BM78_EEPROM_STORE)
 * @param value Pointer to the value
 */
void BM78_write(BM78_CommandOpCode_t command, BM78_EEPROM_t store, uint8_t length, uint8_t *value);

/**
 * Executes command on the module.
 * 
 * @param command Command to execute
 * @param length Length of additional parameters
 * @param ... Additional parameters
 */
void BM78_execute(BM78_CommandOpCode_t command, uint8_t length, ...);

/**
 * Sends a request object.
 * 
 * @param request The request.
 */
void BM78_send(BM78_Request_t *request);

/**
 * Sends data to the module.
 * 
 * @param command Command to use
 * @param length Length of the data
 * @param data Data pointer
 */
void BM78_data(uint8_t command, uint8_t length, uint8_t *data);

/**
 * Sends transparent data over the BM78 module.
 * 
 * @param length Data length.
 * @param data Data.
 */
void BM78_sendTransparentData(uint8_t length, uint8_t *data);

/**
 * Checks new data asynchronously.
 * 
 * This function should be called periodically.
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
