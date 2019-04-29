/*
 * File:   bluetooth_app_commands.c
 * Author: Brandon
 *
 * Created on June 24, 2017, 5:57 PM
 */

#include <xc.h>
#include "../mcc_generated_files/mcc.h"
#include "bluetooth_flash_commands.h"
#include "../mcc_generated_files/interrupt_manager.h"
#include "bluetooth_app_commands.h"

//COMMANDS//
const uint8_t locInfo_Transmit[5] = { 0xAA, 0x00,0x01,0x01,0x00};           //This packet retrieves Local Information

//Add a checksum to the last char in the array
void BT_AddChecksum( uint8_t *packet, uint8_t size)
{
    uint8_t checksum = 0xFF;                   //Start the checksum with value 0xFF as required to calculate the checksum
    
//    //If the first character in the command is 0xAA
//    //Then skip this char, and reduce the size by one
//    if(*packet == 0xAA)
//    {
//        packet++;
//        size--;
//    }         //Skip the first letter if it is equal to 0xAA
    
    //Count each char in the command and add it to the checksum
    for(uint8_t i =1 ; i < (size - 1); i++)    //we subtract 1 so we do not include the null char which is a place holder for the checksum
    {
        checksum -= packet[i];              //Sum up each byte in the array
    }
    
    //To calculate the checksum you first set the checksum to 0xFF, then you XOR each byte
    //To the result of the checksum "byte ^ checksum" then after all bytes have been xor'ed to
    //the checksum, you add a value of 1 to the final result. This gives you the final checksum.

    BT_WriteBuffer[size-1] = checksum += 0x01;
}

void PulseWakeup()
{
    RTS_SetLow();
    __delay_ms(5);
    RTS_SetHigh();
}

void BT_ReadLocalInfo(void)    
{
    ClearWriteBuffer();
    BT_LoadSendBuffer( locInfo_Transmit, sizeof(locInfo_Transmit));
    
    BT_AddChecksum(BT_WriteBuffer, sizeof(locInfo_Transmit));
    
    PulseWakeup();
    
    BT_SendPacket(BT_WriteBuffer, sizeof(locInfo_Transmit), BT_ReadBuffer, 100);

}

////This command is simply used to check the checksum generation and confirm it is generating correctly
//void BT_ChecksumTest(void)
//{
//    //The checksum should come out to 0xFD
//    uint8_t packet[6] = { 0xAA, 0x00,0x02,0x01,0x00,0x00};      //This packet retrieves Local Information
//    BT_AddChecksum(packet, sizeof(packet));
//    //BT_SendPacket(packet, sizeof(packet));                    //Send the packet                                                                       //Send Packet
//}