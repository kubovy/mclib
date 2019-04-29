/* 
 * File:   bluetooth_flash_commands.h
 * Author: bolg
 *
 * Created on January 26, 2019, 10:24 AM
 */

#ifndef BLUETOOTH_FLASH_COMMANDS_H
#define	BLUETOOTH_FLASH_COMMANDS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h> // include processor files - each processor file is guarded.  

extern uint8_t BT_WriteBuffer[147];
extern uint8_t BT_ReadBuffer[147];

void PulseRSTPin(void);
void BT_Reset(void);
void BT_ResetToTestMode(void);
void BT_ResetToAppMode(void);
void ClearWriteBuffer(void);
void BT_LoadSendBuffer( uint8_t *command, uint8_t bytes);
bool BT_OpenFlashMemory(void);
void ClearReadBuffer(void);
void BT_SetBufferAddress(uint16_t address, uint8_t lowByte, uint8_t highByte);
bool BT_Send128Bytes(uint16_t address, uint8_t *data);
bool BT_Read128Bytes(uint16_t address);
bool BT_EndFlashMemoryAccess(void);
bool BT_EraseFlashMemory(void);
bool BT_SendPacket(uint8_t *sendPacket, uint8_t sendSize, uint8_t *recievePacket, uint8_t readSize);
uint8_t GetSizeOfMemBlock(void);
bool BT_SendConfigToEEPROM(void);
bool BT_CheckConfig(void);
bool BT_CompareEmptyMemory(uint16_t address);
bool BT_CompareMemory(uint16_t address, uint8_t *expectedData);
void CheckAndUpdateFlash(void);

#ifdef	__cplusplus
}
#endif

#endif	/* BLUETOOTH_FLASH_COMMANDS_H */

