/*
 * File:   bm78_pairing.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_pairing.h"
#include "../lib/common.h"

#ifdef BM78_ENABLED

#define BMP_CMD_TIMEOUT BM78_INITIALIZATION_TIMEOUT / TIMER_PERIOD

bool BMP_waitingForPasskeyConfirmation = false;
uint8_t BMP_passkeyCodeIndex = 0xFF;

typedef enum {
    BMP_CMD_TYPE_NOTHING = 0x00,
    BMP_CMD_TYPE_REMOVE_ALL_PAIRED_DEVICES = 0x01
} BMP_CommandType_t;

struct {
    BMP_CommandType_t type;
    uint8_t step;
    uint16_t timeout;
} BMP_progress = {0, 0, 0};

bool BMP_waiting(void) {
    return BMP_passkeyCodeIndex < 0xFF || BMP_waitingForPasskeyConfirmation;
}

void BMP_cancel(void) {
    BMP_passkeyCodeIndex = 0xFF;
    BMP_waitingForPasskeyConfirmation = false;
}

void BMP_processKey(uint8_t key) {
    if (BMP_passkeyCodeIndex < 0xFF) { // Pairing Passkey
        if (key >= '0' && key <= '9') { // Digit
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_DIGIT_ENTERED, key);
#ifdef LCD_ADDRESS
            LCD_replaceChar(key, (BMP_passkeyCodeIndex++) + 6, 1, true);
#endif
        } else if (key == 'A') {
            //BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_DIGIT_ENTERED, 0x00);
        } else if (key == 'B') { // Clear
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_CLEARED, 0x00);
#ifdef LCD_ADDRESS
            LCD_replaceString("          ", 6, 1, true);
#endif
            BMP_passkeyCodeIndex = 0;
        } else if (key == 'C') { // Confirm
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_ENTRY_COMPLETED, 0x00);
#ifdef LCD_ADDRESS
            LCD_clear();
#endif
            BMP_passkeyCodeIndex = 0xFF;
        } else if (key == 'D') { // Delete
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_DIGIT_ERASED, 0x00);
#ifdef LCD_ADDRESS
            LCD_replaceChar(' ', (--BMP_passkeyCodeIndex) + 6, 1, true);
#endif
        }
    } else if (BMP_waitingForPasskeyConfirmation) {
        switch(key) { // Pairing Confirmation
            case 'C': // Confirm (YES)
                BM78_execute(BM78_CMD_USER_CONFIRM_RES, 1, BM78_PAIRING_USER_CONFIRM_YES);
                BMP_waitingForPasskeyConfirmation = false;
                break;
            case 'A': // Abort (NO)
                BM78_execute(BM78_CMD_USER_CONFIRM_RES, 1, BM78_PAIRING_USER_CONFIRM_NO);
                BMP_waitingForPasskeyConfirmation = false;
                break;
        }
    }
}

void BMP_retryTrigger(void) {
    if (BMP_progress.type > 0) {
        BMP_progress.timeout--;
        if (BMP_progress.timeout == 0) {
            BMP_progress.timeout = BMP_CMD_TIMEOUT;
            switch (BMP_progress.type) { // remove all paired devices
                case BMP_CMD_TYPE_REMOVE_ALL_PAIRED_DEVICES:
                    switch (BMP_progress.step) {
                        case 0:
                            BM78_execute(BM78_CMD_READ_STATUS, 0);
                            break;
                        case 1:
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                            break;
                        case 2:
                            BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                            break;
                        case 3: 
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                            break;
                    }
                    break;
            }
        }
    }
}

void BMP_removeAllPairedDevices(void) {
    BMP_progress.type = 1;
    BMP_progress.step = 0;
    BMP_progress.timeout = BMP_CMD_TIMEOUT;
    printStatus("        .           ");
    if (BM78.enforceState) {
        BM78.enforceState = BM78_STANDBY_MODE_LEAVE;
        BM78_execute(BM78_CMD_READ_STATUS, 0);
    }
}

inline void BMP_bm78PairingEventHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_EVENT_PASSKEY_ENTRY_REQ:
#ifdef LCD_ADDRESS
            LCD_clear();
            LCD_setString("New Pairing Request", 0, true);
            LCD_setString("Code:               ", 1, true);
            LCD_setString("D) Delete   Clear (B", 2, true);
            LCD_setString("C) Confirm  Abort (A", 3, true);
#endif
            BMP_cancel();
            BMP_passkeyCodeIndex = 0;
            break;
        case BM78_EVENT_PAIRING_COMPLETE:
#ifdef LCD_ADDRESS
            LCD_clear();
            switch(response.PairingComplete_0x61.result) {
                case BM78_PAIRING_RESULT_COMPLETE:
                    LCD_setString(" New Device Paired  ", 1, true);
                    break;
                case BM78_PAIRING_RESULT_FAIL:
                    LCD_setString("  Pairing Failed!   ", 1, true);
                    break;
                case BM78_PAIRING_RESULT_TIMEOUT:
                    LCD_setString(" Pairing Timed Out! ", 1, true);
                    break;
                default:
                    break;
            }
#endif
            BMP_cancel();
            break;
        case BM78_EVENT_PASSKEY_DISPLAY_YES_NO_REQ:
#ifdef LCD_ADDRESS
            LCD_clear();
            LCD_setString("New Pairing Request", 1, true);
            //LCD_displayString("                    ", 1, true);
            //if (response.passkey >= 100) LCD_insertChar(dec2hex(response.passkey / 100 % 10), 8, 1);
            //if (response.passkey >= 10) LCD_insertChar(dec2hex(response.passkey / 10 % 10), 9, 1);
            //LCD_insertChar(dec2hex(response.passkey % 10), 10, 1);
            LCD_setString("C) Confirm  Abort (A", 3, true);
#endif
            BMP_cancel();
            BMP_waitingForPasskeyConfirmation = true;
            break;
        case BM78_EVENT_DISCONNECTION_COMPLETE:
            BMP_cancel();
            break;
        default:
            break;
    }
}

inline void BMP_bm78DeleteAllPairedDevicesEventHandler(BM78_Response_t response, uint8_t *data) {
    if (BMP_progress.type == 1) switch (response.op_code) {
        case BM78_EVENT_COMMAND_COMPLETE:
            if (response.CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                switch (response.CommandComplete_0x80.command) {
                    case BM78_CMD_INVISIBLE_SETTING:
                        if (BMP_progress.step == 1) {
                            printStatus("        ...         ");
                            BMP_progress.step = 2;
                            BMP_progress.timeout = BMP_CMD_TIMEOUT;
                            BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                        }
                        if (BMP_progress.step == 3) {
                            printStatus("                    ");
                            BMP_progress.type = 0;
                            BMP_progress.step = 0;
                            BMP_progress.timeout = BMP_CMD_TIMEOUT;
                        }
                        break;
                    case BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO:
                        if (BMP_progress.step == 2) {
                            printStatus("        ....        ");
                            BM78.pairedDevicesCount = 0;
                            BMP_progress.step = 3;
                            BMP_progress.timeout = BMP_CMD_TIMEOUT;
                            BM78.enforceState = BM78_STANDBY_MODE_ENTER;
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                        }
                        break;
                }
            } else {
                switch (response.CommandComplete_0x80.command) {
                    case BM78_CMD_INVISIBLE_SETTING:
                        if (BMP_progress.step == 1) {
                            printStatus("        ..x         ");
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                        }
                        if (BMP_progress.step == 3) {
                            printStatus("        ....x       ");
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                        }
                        break;
                    case BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO:
                        if (BMP_progress.step == 2) {
                            printStatus("        ...x        ");
                            BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                        }
                        break;
                }
            }
            break;
        case BM78_EVENT_BM77_STATUS_REPORT:
            if (BMP_progress.step == 0) {
                if (response.StatusReport_0x81.status == BM78_STATUS_IDLE_MODE) {
                    BMP_progress.step = 2;
                    printStatus("        ...         ");
                    BMP_progress.timeout = BMP_CMD_TIMEOUT;
                    BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                } else {
                    printStatus("        ..          ");
                    BMP_progress.step = 1;
                    BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                }
            }
            break;
    }
}

void BMP_bm78EventHandler(BM78_Response_t response, uint8_t *data) {
    BMP_bm78PairingEventHandler(response, data);
    BMP_bm78DeleteAllPairedDevicesEventHandler(response, data);
}

#endif