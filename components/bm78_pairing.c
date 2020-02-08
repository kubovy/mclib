/*
 * File:   bm78_pairing.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_pairing.h"
#include "../lib/common.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif

#ifdef BM78_ENABLED

typedef enum {
    BMP_CMD_TYPE_NOTHING = 0x00,
    BMP_CMD_TYPE_ENTER_PAIRABLE_MODE = 0x01,
    BMP_CMD_TYPE_REMOVE_ALL_PAIRED_DEVICES = 0x02,
} BMP_CommandType_t;

struct {
    BMP_CommandType_t type;
    uint8_t step;
    uint16_t timeout;
} BMP_progress = {BMP_CMD_TYPE_NOTHING, 0, 0};

#ifdef BM78_ADVANCED_PAIRING
bool BMP_waitingForPasskeyConfirmation = false;
uint8_t BMP_passkeyCodeIndex = 0xFF;

bool BMP_waiting(void) {
    return BMP_passkeyCodeIndex < 0xFF || BMP_waitingForPasskeyConfirmation;
}

void BMP_cancel(void) {
    BMP_passkeyCodeIndex = 0xFF;
    BMP_waitingForPasskeyConfirmation = false;
}

bool BMP_processKey(uint8_t key) {
    if (BMP_passkeyCodeIndex < 0xFF) { // Pairing Passkey
        if (key >= '0' && key <= '9') { // Digit
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_DIGIT_ENTERED, key);
#ifdef LCD_ADDRESS
            LCD_replaceChar(key, (BMP_passkeyCodeIndex++) + 10, 1, true);
#endif
            return true;
        } else if (key == 'A') {
            //BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_DIGIT_ENTERED, 0x00);
            return true;
        } else if (key == 'B') { // Clear
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_CLEARED, 0x00);
#ifdef LCD_ADDRESS
            LCD_replaceString("          ", 10, 1, true);
#endif
            BMP_passkeyCodeIndex = 0;
            return true;
        } else if (key == 'C') { // Confirm
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_ENTRY_COMPLETED, 0x00);
#ifdef LCD_ADDRESS
            LCD_clear();
#endif
            BMP_passkeyCodeIndex = 0xFF;
            return true;
        } else if (key == 'D') { // Delete
            BM78_execute(BM78_CMD_PASSKEY_ENTRY_RES, 2, BM78_PAIRING_PASSKEY_DIGIT_ERASED, 0x00);
#ifdef LCD_ADDRESS
            LCD_replaceChar(' ', (--BMP_passkeyCodeIndex) + 10, 1, true);
#endif
            return true;
        }
    } else if (BMP_waitingForPasskeyConfirmation) {
        switch(key) { // Pairing Confirmation
            case 'C': // Confirm (YES)
#ifdef LCD_ADDRESS
                LCD_clear();
                LCD_setString(" Pairing Confirmed  ", 1, true);
#endif
                BM78_execute(BM78_CMD_USER_CONFIRM_RES, 1, BM78_PAIRING_USER_CONFIRM_YES);
                BMP_waitingForPasskeyConfirmation = false;
                return true;
            case 'A': // Abort (NO)
#ifdef LCD_ADDRESS
                LCD_clear();
                LCD_setString("  Pairing Aborted   ", 1, true);
#endif
                BM78_execute(BM78_CMD_USER_CONFIRM_RES, 1, BM78_PAIRING_USER_CONFIRM_NO);
                BMP_waitingForPasskeyConfirmation = false;
                return true;
        }
    }
    return false;
}

inline void BMP_bm78PairingEventHandler(BM78_Response_t *response) {
    switch (response->opCode) {
        case BM78_EVENT_PASSKEY_ENTRY_REQ:
#ifdef LCD_ADDRESS
            LCD_clear();
            LCD_setString("New Pairing Request ", 0, true);
            LCD_setString("    Code:           ", 1, true);
            LCD_setString("D) Delete   Clear (B", 2, true);
            LCD_setString("C) Confirm  Abort (A", 3, true);
#endif
            BMP_cancel();
            BMP_passkeyCodeIndex = 0;
            break;
        case BM78_EVENT_PAIRING_COMPLETE:
#ifdef LCD_ADDRESS
            LCD_clear();
            switch(response->PairingComplete_0x61.result) {
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
            LCD_setString("New Pairing Request ", 0, true);
            LCD_setString("    Code:           ", 1, false);
            for (uint8_t i = 0; i < 6; i++) {
                LCD_replaceChar(
                        response->PasskeyDisplayYesNoReq_0x62.passkey[i],
                        10 + i, 1, false);
            }
            LCD_displayLine(1);
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

#endif

void BMP_retryTrigger(void) {
    if (BMP_progress.type > 0) {
        BMP_progress.timeout--;
        if (BMP_progress.timeout == 0) {
            BMP_progress.timeout = BMP_CMD_TIMEOUT / BM78_TRIGGER_PERIOD;
            switch (BMP_progress.type) { // remove all paired devices
                // Read status is done by BM78_checkState periodically
                //case BMP_CMD_TYPE_ENTER_PAIRABLE_MODE:
                //    switch (BMP_progress.step) {
                //        case 0:
                //            BM78_execute(BM78_CMD_READ_STATUS, 0);
                //            break;
                //    }
                //    break;
                case BMP_CMD_TYPE_REMOVE_ALL_PAIRED_DEVICES:
                    switch (BMP_progress.step) {
                        // Read status is done by BM78_checkState periodically
                        //case 0:
                        //    BM78_execute(BM78_CMD_READ_STATUS, 0);
                        //    break;
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
                default:
                    break;
            }
        }
    }
}

void BMP_enterPairingMode(void) {
    BMP_progress.type = BMP_CMD_TYPE_ENTER_PAIRABLE_MODE;
    BMP_progress.step = 0;
    BMP_progress.timeout = BMP_CMD_TIMEOUT / BM78_TRIGGER_PERIOD;
    printStatus("         <>         ");
    BM78.enforceState = BM78_STANDBY_MODE_LEAVE;
    // Read status is done by BM78_checkState periodically
    //BM78_execute(BM78_CMD_READ_STATUS, 0);
}

void BMP_removeAllPairedDevices(void) {
    printStatus("         .          ");
    BMP_progress.type = BMP_CMD_TYPE_REMOVE_ALL_PAIRED_DEVICES;
    BMP_progress.step = 0;
    BMP_progress.timeout = BMP_CMD_TIMEOUT / BM78_TRIGGER_PERIOD;
    BM78.enforceState = BM78_STANDBY_MODE_LEAVE;
    // Read status is done by BM78_checkState periodically
    //BM78_execute(BM78_CMD_READ_STATUS, 0);
}

inline void BMP_bm78ProcessEventHandler(BM78_Response_t *response) {
    switch (BMP_progress.type) {
        case BMP_CMD_TYPE_ENTER_PAIRABLE_MODE:
            switch (response->opCode) {
                case BM78_EVENT_STATUS_REPORT:
                    if (BMP_progress.step == 0) {
                        if (response->StatusReport_0x81.status == BM78_STATUS_IDLE_MODE) {
                            printStatus("                    ");
                            BMP_progress.type = 0;
                            BM78.enforceState = BM78_STANDBY_MODE_ENTER;
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                        } else {
                            BM78.enforceState = BM78_STANDBY_MODE_ENTER;
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        case BMP_CMD_TYPE_REMOVE_ALL_PAIRED_DEVICES:
            switch (response->opCode) {
                case BM78_EVENT_COMMAND_COMPLETE:
                    if (response->CommandComplete_0x80.status == BM78_COMMAND_SUCCEEDED) {
                        switch (response->CommandComplete_0x80.command) {
                            case BM78_CMD_INVISIBLE_SETTING:
                                if (BMP_progress.step == 1) {
                                    printStatus("        ...         ");
                                    BMP_progress.step = 2;
                                    BMP_progress.timeout = BMP_CMD_TIMEOUT / BM78_TRIGGER_PERIOD;
                                    BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                                }
                                if (BMP_progress.step == 3) {
                                    printStatus("                    ");
                                    BMP_progress.type = 0;
                                }
                                break;
                            case BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO:
                                if (BMP_progress.step == 2) {
                                    printStatus("        ....        ");
                                    BM78.pairedDevicesCount = 0;
                                    BMP_progress.step = 3;
                                    BMP_progress.timeout = BMP_CMD_TIMEOUT / BM78_TRIGGER_PERIOD;
                                    BM78.enforceState = BM78_STANDBY_MODE_ENTER;
                                    BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                                }
                                break;
                            default:
                                break;
                        }
                    } else {
                        switch (response->CommandComplete_0x80.command) {
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
                            default:
                                break;
                        }
                    }
                    break;
                case BM78_EVENT_STATUS_REPORT:
                    if (BMP_progress.step == 0) {
                        if (response->StatusReport_0x81.status == BM78_STATUS_IDLE_MODE) {
                            BMP_progress.step = 2;
                            printStatus("        ...         ");
                            BMP_progress.timeout = BMP_CMD_TIMEOUT / BM78_TRIGGER_PERIOD;
                            BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                        } else {
                            printStatus("        ..          ");
                            BMP_progress.step = 1;
                            BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;

    }
}

void BMP_bm78EventHandler(BM78_Response_t *response) {
#ifdef BM78_ADVANCED_PAIRING
    BMP_bm78PairingEventHandler(response);
#endif
    BMP_bm78ProcessEventHandler(response);
}

#endif
