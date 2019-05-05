/*
 * File:   bm78_pairing.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_pairing.h"

#ifdef BM78_ENABLED

bool BMP_waitingForPasskeyConfirmation = false;
uint8_t BMP_passkeyCodeIndex = 0xFF;

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

void BMP_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_OPC_PASSKEY_ENTRY_REQ:
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
        case BM78_OPC_PAIRING_COMPLETE:
#ifdef LCD_ADDRESS
            LCD_clear();
            switch(response.PairingComplete_0x61.result) {
                case BM78_PAIRING_RESULT_COMPLETE:
                    LCD_setString("|c|New Device Paired", 1, true);
                    break;
                case BM78_PAIRING_RESULT_FAIL:
                    LCD_setString("|c|Pairing Failed!", 1, true);
                    break;
                case BM78_PAIRING_RESULT_TIMEOUT:
                    LCD_setString("|c|Pairing Timed Out!", 1, true);
                    break;
                default:
                    break;
            }
#endif
            BMP_cancel();
            break;
        case BM78_OPC_PASSKEY_DISPLAY_YES_NO_REQ:
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
        case BM78_OPC_DISCONNECTION_COMPLETE:
            BMP_cancel();
            break;
        default:
            break;
    }
}

#endif