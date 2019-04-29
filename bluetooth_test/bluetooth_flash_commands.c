/*
 * File:   Bluetooth.c
 * Author: Brandon
 *
 * Created on June 17, 2017, 7:25 AM
 */


#include <xc.h>
#include "../mcc_generated_files/mcc.h"
#include "bluetooth_flash_commands.h"
#include "../mcc_generated_files/interrupt_manager.h"
#include "bluetooth_flash_data.h"

//Load the memoryBlock array from the "bluetooth_flash_data.h" header so we can read it here
extern const flash128_t memoryBlock[16];

//Init Array Buffers
uint8_t BT_WriteBuffer[147];
uint8_t BT_ReadBuffer[147];

//////////////////////////////////
/////////////COMMANDS/////////////
//////////////////////////////////

//These are all set to "const" to prevent them from being loaded in the limited "Data" memory. This also prevents
//them from being directly edited, so they must be copied into the transmit register THEN edited in the transmit
//register.

///OPEN FLASH MEMORY
const uint8_t open_Transmit[17] = {0x01,0x05,0x04,0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t open_Response[21] = {0x04,0x0F,0x04,0x00,0x01,0x05,0x04,0x04,0x03,0x0B,0x00,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00};

///READ COMMAND
const uint8_t read_Transmit[19] = {0x02,0xFF,0x0F,0x0E,0x00,0x10,0x01,0x0A,0x00,0x03,0x00,0x00,0x40,0x03,0x00,0x80,0x00,0x00,0x00};
const uint8_t read_Response[19] = {0x04,0x13,0x05,0x01,0xFF,0x0F,0x01,0x00,0x02,0xFF,0x0F,0x86,0x00,0x10,0x01,0x82,0x00,0x00,0x00};

///ERASE ALL FLASH
const uint8_t erase_Transmit[19] = {0x02,0xFF,0x0F,0x0E,0x00,0x12,0x01,0x0A,0x00,0x03,0x00,0x00,0x40,0x03,0x00,0x00,0x20,0x00,0x00};
const uint8_t erase_Response[27] = {0x04,0x13,0x05,0x01,0xFF,0x0F,0x01,0x00,0x02,0xFF,0x0F,0x0E,0x00,0x12,0x01,0x0A,0x00,0x00,0x00,0x00,0x40,0x03,0x00,0x00,0x20,0x00,0x00};

///WRITE COMMAND
const uint8_t write_Transmit[19] = {0x02,0xFF,0x0F,0x8E,0x00,0x11,0x01,0x8A,0x00,0x03,0x00,0x00,0x40,0x03,0x00,0x80,0x00,0x00,0x00};
const uint8_t write_Response[19] = {0x04,0x13,0x05,0x01,0xFF,0x0F,0x01,0x00,0x02,0xFF,0x0F,0x06,0x00,0x11,0x01,0x02,0x00,0x00,0x00};

//CLOSE FLASH MEMORY
const uint8_t close_Transmit[7] = {0x01,0x06,0x04,0x03,0xFF,0x0F,0x00};
const uint8_t close_Response[14] = {0x04,0x0F,0x04,0x00,0x01,0x06,0x04,0x04,0x05,0x04,0x00,0xFF,0x0F,0x00};

//////////////////////////////////
/////////////COMMANDS/////////////
//////////////////////////////////



//Pulse the reset pin low to reset the unit
void PulseRSTPin(void)
{
    RST_SetLow();       //Set the reset pin low
    __delay_ms(2);      //A reset pulse must be greater than 1ms
    RST_SetHigh();      //Pull the reset pin back up    

}

//Reset the bluetooth unit and enter the mode specified by the SYS CON pin
void BT_Reset(void)
{
    PulseRSTPin();              //Reset the device
    __delay_ms(100);             //Wait a minimum of 24 seconds for mode to be detected
    SYS_CON_SetHigh();          //Reset P2_0 pin to high state
    __delay_ms(100);             //Wait a minimum of 43ms for UART to activate after reset
}

//Reset the unit to and end enter the test mode
void BT_ResetToTestMode(void)
{
    SYS_CON_SetLow();
    BT_Reset();
}

//Reset the unit and enter the Application mode
void BT_ResetToAppMode(void)
{
    SYS_CON_SetHigh();
    BT_Reset();
}

void ClearWriteBuffer(void)
{
    /////////////////////////// THIS COMMENT IS OUT OF DATE ////////////////////////////////////
    /////////////////AFTER RE-WRITTING THE CODE, WE WILL ALWAYS SEND 128 BYTES//////////////////
    /////////////////////BUT THIS INFORMATION IS USEFUL SO I LEFT IT IN/////////////////////////
    //Set all values in the write buffer to 0xFF. We do this because of the
    //nature of EEPROM. EEPROM can bits can only be converted from a 1 to a 0.
    //The only way to change the EEPROM bits back from a 0 to a 1 is to ERASE the whole
    //EEPROM. This means once a byte is written, trying to write a value of
    //0xFF (11111111b) will have no effect on the value, since we are not able
    //to change any of the 0 bits back to a 1, and any bits already set to 1
    //will simply remain as 1. Using this trick, we can write only the data we
    //want, and leave the rest of the data in the EEPROM intact even though we
    //have to write 128 bytes every write. We just populate the empty data with
    //0xFF.
    //
    //Unlike the read buffer, we HAVE to clear this buffer before writing. If not
    //Then we can corrupt data when we write to the EEPROM memory due to bit switching.
    //////////////////////////////////////////////////////////////////////////////////////////////
    
    for(uint8_t i = 0; i < sizeof(BT_WriteBuffer); i++)
    BT_WriteBuffer[i] = 0xFF;
}

//Put a command into the transmit buffer
void BT_LoadSendBuffer( uint8_t *command, uint8_t bytes)
{
    //Clear out the write buffer to make it easier to troubleshoot and read
    ClearWriteBuffer();
    
    //Take the command passed in and store it into the BT_WriteBuffer
    for(uint8_t i = 0; i < bytes; i++)
        BT_WriteBuffer[i] = *command++;
}


//Must be called first to access the flash memory
bool BT_OpenFlashMemory(void)
{
    return BT_SendPacket(open_Transmit,sizeof(open_Transmit),open_Response, sizeof(open_Response));                                                                         //Send Packet
}



void ClearReadBuffer(void)
{
    //Set all values in the read buffer to 0x00
    //We are only reading bits to this buffer, not writing, so we initialize it
    //to 0x00 instead of 0xFF to make it easier to read and troubleshoot.
    //However, you can increase performance by not using this command.
    for(uint8_t i = 0; i < sizeof(BT_ReadBuffer); i++)
        BT_ReadBuffer[i] = 0x00;
}


void BT_SetBufferAddress(uint16_t address, uint8_t lowByte, uint8_t highByte)
{
    //Since the address is 16 bytes instead of 8 bytes we need to split
    //the address in half and send each part of it in a specific slot in
    //the command string. 
    
    //This is because we only have the ability to send a total of 8 bits at a time (1 byte)
    //so we have to break the 16 bit address into two separate 8 bits (2 separate bytes)
    
    //The slots that the two bytes need to go changes
    //between commands, so the "lowByte" is where the 8 most LSB (Least Significant Bit)
    //goes and the high bytes is where the 8 MSB goes (Most Significant Bit)
    
    //An address of 0x1FE0 would be split in half, so you would have
    //0x1F and 0xE0. Where 0x1F is the MSBs and 0xE0 is the LSBs
    
    //We now place these two separate 8 bits into the command at the required 
    
    //Because we are using constants for the commands with the "const" modifier
    //we can not write the address directly to the command array. Instead we have
    //to copy the command to the "BT_WriteBuffer" then update the address while
    //it is in the buffer, since we can edit the buffer, but not the commands.
    //We can only read the commands, not write to them to save space.
    
    //So the command must FIRST be loaded into the buffer THEN use this command
    //to change the address of the command while it is in the buffer.
    
    BT_WriteBuffer[lowByte] = (uint8_t) address;
    BT_WriteBuffer[highByte] += (uint8_t) (address >> 8);
}


bool BT_Send128Bytes(uint16_t address, uint8_t *data)
{
    //Clear the buffers
    ClearWriteBuffer();
    ClearReadBuffer();
    
    //We first add the 19 byte write command to the beginning of the buffer
    BT_LoadSendBuffer(write_Transmit, sizeof(write_Transmit));
    
    //We then update the buffer with the specified address to write 128 bytes to
    BT_SetBufferAddress(address, 11, 12);
    
    //We now add the data we want to change in the EEPROM after the write command (128 bytes)
    //this is why i = sizeof(write_Transmit) and not i = 0. We are skipping past
    //the write command at the beginning of the buffer.
    
    /////////////////////////// THIS COMMENT IS OUT OF DATE ////////////////////////////////////
    /////////////////AFTER RE-WRITTING THE CODE, WE ALWAYS SEND 128 BYTES NOW///////////////////
    /////////////////////BUT THIS INFORMATION IS USEFUL SO I LEFT IT IN/////////////////////////
    //All bytes after the command and data will remain as 0xFF, which as we know from
    //the previous comments, will have no effect on any values in the EEPROM after
    //the bytes we want to change due to the nature of the EEPROM.
    ////////////////////////////////////////////////////////////////////////////////////////////
    for (uint8_t i = sizeof(write_Transmit); i < 147; i++)                             
        BT_WriteBuffer[i] = *data++;   
    
    //We now send the full write buffer to the UART. This includes the 19 byte command 
    //plus 128 bytes of data (including the blank 0xFF values)
    //We then wait for the 19 byte response from the device letting us know the
    //the operation was performed successfully.
    BT_SendPacket(BT_WriteBuffer, 147, BT_ReadBuffer, 19);
    

    
    //Check that the BT device responded with the 
    for(uint8_t i = 0; i < 19; i++)
        if(BT_ReadBuffer[i] != write_Response[i])
            return false;
    
    //If the response from the Bluetooth device was what was expected then return a true
    return true;
    
}

bool BT_Read128Bytes(uint16_t address)
{
    //Clear the buffers
    ClearWriteBuffer();
    ClearReadBuffer();
    
    //Populate the read command in the buffer
    BT_LoadSendBuffer(read_Transmit, sizeof(read_Transmit));

    //Update the address of the read command in the buffer
    BT_SetBufferAddress(address, 11, 12);
    
    //Send the read command
    BT_SendPacket(BT_WriteBuffer, sizeof(read_Transmit), BT_ReadBuffer, sizeof(BT_ReadBuffer));
    
    //Read through the 19byte response from the BT device and confirm the correct
    //command was sent back
    for(uint8_t i = 0; i < 19; i++)
        if(BT_ReadBuffer[i] != read_Response[i])
            return false;
    
    //If the response from the Bluetooth device was what was expected then return a true
    return true;
}

bool BT_Write128BytesToFlash(uint16_t address, uint8_t *data )
{
    //Load the write command into the BT buffer (NOT UART BUFFER)
    if(!BT_Send128Bytes(address, data))
        
        //if the "BT_SendWriteCommand" returns false, then the device did not
        //respond as expected. Also return a false
        return false;

    //Now that we have written our bytes to the device's EEPROM we need to verify
    //it took hold. We now read 128 bytes of EEPROM memory from the address we
    //just wrote to.
    
    //Update the BT send buffer with a command to read at the specified address
    if(!BT_Read128Bytes(address))
        
        //If the "BT_SendReadCommand" returns a false, then the BT device did
        //not respond as expected. Also, return a false.
        return false;

    //We now compare the data we sent vs what is in the devices memory
    //We start at "sizeof(read_Response)" because the device will have a standard
    //response in front of the data, so we skip the BT's response string in the
    //response buffer to get to the actual read data from its memory.
//        
    //We start reading after the command, all the way to the end of the BT_ReadBuffer
    //This is why we have "i < sizeof(BT_ReadBuffer)"
//
    //Every time we read a new byte from the buffer, we read the next byte of the
    //data we sent (using a pointer "*data"). We do this by reading the byte of
    //the array we passed in (with the *data pointer) then moving the pointer to
    //the next byte in the data array using "*data++". When we pass in the name
    //of the array, we get a pointer that points to the very first byte in the
    //array. We can move to the next byte in the array by incrementing the pointer
    //by one.
//    
    //It is a common method to pass arrays between functions using "pointers"
//        
    //If this is confusing, look into "Pointers and Arrays in C" for more info
    for(uint8_t i = sizeof(read_Response); i < sizeof(BT_ReadBuffer); i++)

        //We compare byte by byte between the two buffers
        if(BT_ReadBuffer[i] != *data++)
            
            //if the data does not match, we return a false
            return false;
    
    //If all data matched, we return a true.
    return true;
}

//Exit Memory Access Mode
bool BT_EndFlashMemoryAccess(void)
{
    //Send the close access to the flash memory command and check the response
    return BT_SendPacket(close_Transmit,sizeof(close_Transmit),BT_ReadBuffer, sizeof(close_Response));
    
    
}

///////////////////////////////////////IMPORTANT NOTE///////////////////////////////////////
//Due to the nature of EEPROM (The type of memory being written to on the BT module),
//you must completely erase the memory BEFORE you can write any bytes. 
//This is the only way to ensure the flash will be updated properly and not become corrupted.
//This is because with EEPROM, you are
//only able to CHANGE A BIT FROM 1 TO 0, but NOT a 0 to a 1, without erasing the EEPROM. So
//erasing the memory, resets all values in the EEPROM to 0xFF or (11111111b) which
//you can then change the 0xFF value to the desired value on first write. 
//
//If you try to write to the memory a second time (Without erasing it first) the
//data would become corrupt since you can not undo bits that are already set to 0...
//After the memory is erased, and all bits are set to 0xFF you will be able to write
//once. Any subsequent changes will not yield the desired results until all of the flash
//is erased again to the 0xFF values...
//
//EXAMPLE:
//After a fresh ERASE, a value of 0xFF can be changed to 0x01 (00000001b). This is becuase
//we were able to change the bits from a 1 to a 0... However since
//all bits except for the least significant bit (the only bit set to 1 after 0x01 is sent) are all 0's, 
//only the least significant bit (the only 1 bit) can still be changed since it is the only
//bit left that can go from a 1 TO a 0, since we can not go from the 0's to a one. So after
//writing a value of 0x01, the only value we can change 0x01 to when trying to write over it
//again would be 0x00. Once we write 0x00, we can no longer change the bytes value until after
//erasing it again, since at that point ALL bits will be 0's and we can not change any of them
//back to 1's without erasing the full EEPROM again.
//
//If we write the value 0x01 (00000001b) to a byte in the eprom memory, then we try to write
//the value 0x02 (00000010b) to that same byte, the result will be 0x00 (00000000b) and NOT
//0x02. This is because again... we can not set any of the bits back to 1 without erasing. So
//since we had already written 0x01 (00000001b) and we tried to write 0x02, the EEPROM will still
//change the LSB (the bit all the way on the right that was a 1) in its memory to 0, but it can not
//change the 0 to a 1, so the resulting value is 0x00. 
//
//Another example:
//0xB5 (10110101b) is in the EEPROM's memory. We try to write 0xD7 (11010111b) over the 
//byte in the EEPROM memory that already had the value 0xB5 (10110101b) the result would be
//0x?95? (10010101b) NOT 0xD7, causing data corruption (Bits 1 & 6 can not be changed from the 0
//to the 1 required fro 0xD7. So we must erase the memory every
//time we write to the EEPROM memory.
//
//If we had the ability to change the Erase command, we may be able to only erase one byte at a time
//instead of all bytes every time, but due to the lack of information on the erase command, we are limited
//to erasing the all of the EEPROM at once.
//
//Due to this nature of EEPROM, we must erase ALL of the EEPROM in order to re-write any 
//of the bytes in the BT's EEPROM.
//
//ALSO! There is a limit to how many times the EEPROM of the bluetooth module can be erased
//It is upwards of 100,000 times. HOWEVER! You do NOT want to erase the memory EVERYTIME you
//Start the module. You FIRST want to check the unit to see if the EEPROM of the blueooth
//unit does NOT match the current configuration, THEN erase it. But if you erase the bluetooth's
//EEPROM every time you start the PIC. You will eventually wear out the bluetooth module and
//permanently brick it.
///////////////////////////////////////IMPORTANT NOTE///////////////////////////////////////
bool BT_EraseFlashMemory(void)
{
    if(!BT_SendPacket(erase_Transmit,sizeof(erase_Transmit),erase_Response, sizeof(erase_Response)))
        
        //Return a false if the BT unit did not respond as expected
        return false;
    
    //I am not sure if the BT unit will respond only after it clears its memory or if it responds
    //immediately THEN clears its memory. Either way I give it some time to clear its memory.
    //Wait for the unit to clear its memory before continuing.
    __delay_ms(50); 

    //If we have not returned a false yet, then everything passed, and return true.
    return  true;
}

//Send a packet of hex values to the bluetooth device, then check the response from the unit
//and return true if the response matches the expected recievPacket that is passed in.
//else return false.
bool BT_SendPacket(uint8_t *sendPacket, uint8_t sendSize, uint8_t *recievePacket, uint8_t readSize)
{
    //Send the "sendPacket"
    for(int i = 0 ; i < sendSize; i++)          //Send the command bits, along with the parameters
    {
        EUSART_Write(sendPacket[i]);            //Store each byte in the storePacket into the UART write buffer
    }
    
    //Since we talk to the BT module at 115200 baud (or 115200 bits per second)
    //AND 1 byte is actually 10 bits total for transmission:
    //   1 start bit + 8 data bits + 1 stop bit = 10 bits total per byte
    //
    //AND our longest transmission length is 147 bytes (19 bytes for command + 128 bytes for data)
    //The total bits transmitted is 147 bytes * 10 bits = 1470 bits total for the longest
    //command we send/receive, we take the inverse of 115200 to get the time in 
    //seconds for each bit ( 1 sec / 115200 bits ) and multiply it by our total
    //number of bits (1470 bits) we get the total time required to transmit our
    //longest command:
    // (1 sec / 115200 bits) * 1470 bits = 0.0130667 sec (13.0667 ms)
    //
    //There will also be some hardware delay in response time and calculations etc...
    //So we wait 30ms ( a little over 13ms for some padding) for the unit to completely
    //respond to our command
    //__delay_ms(30);

    //Update the specified buffer with the response in the UART
    for(uint8_t i = 0; i < readSize; i++)
    {
        //when I put this direct, I get XC8 compile error (not enough registers...)
        uint8_t readByte = EUSART_Read();
        recievePacket[i] =  readByte;              //Read each byte returned from the device which is in the Rx buffer
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    //DO NOT PUT BREAKPOINT HERE OR IT WILL PREVENT READING BUFFER FROM POPULATING//
    ////////////////////////////////////////////////////////////////////////////////
    
    return true;                                //If the bytes all matched, return a true    
    
}

uint8_t GetSizeOfMemBlock()
{
    //The "sizeof" function returns the total number of bytes for the structure
    //In the "memoryBlock" array, each index has a total of 130 bytes:
    //Since there are 128 bytes for the data, and 2 8 bit bytes for the address
    //there is a total of 130 bytes per index(row). So we take the total number of bytes
    //and divide it by 130, to get the total indexies/rows in the memoryBlock
    //array.
    
    //avoid calling this repeatedly in for loops to prevent delays. Instead
    //call it once and store it into a uint8_t variable.
    return sizeof(memoryBlock) / 130u;
}

bool BT_SendConfigToEEPROM(void)
{
    uint8_t sizeOfMemoryBlock = GetSizeOfMemBlock();
    
    //cycle through each row/index in the "memoryBlock" array and update the
    //flash memory.
    for(uint8_t i = 0; i < sizeOfMemoryBlock; i++)
    {
        //each row has 128 bytes, so we set bytes to 128. We also send the address
        if(!BT_Write128BytesToFlash(memoryBlock[i].address, memoryBlock[i].data))
        {
            //if the BT_Write128BytesToFlash returns a FALSE we know that the bytes we
            //wrote did not take hold, either because we read different values back, or
            //the device responded incorrectly.
            
            //we return a false signifying that there was an issue with the programming
            //process
            return false;
        }
    }
    
    //If we get to this point, because we have not returned false yet, then everything
    //was successful so we return a TRUE. (This function would have been exited already
    //if the "return false" was executed.)
    return true;
}



bool BT_CheckConfig(void)
{
    uint8_t sizeOfMemoryBlock = GetSizeOfMemBlock();
    bool isAddressInBlock;
    uint8_t index = 0;
    
    //This is a normal for loop, except we are using address instead of i
    //address is also a 16bit word, not an 8 bit word because the
    //bluetooth's addresses are all 16 bits.
    //Since we are reading/writing 128 bytes at a time, we increment by
    //0x0080 each iteration of the for loop. NOT the number 80, but 128 in hex.
    //This causes us to have the address of the next 128 bytes in the memory each
    //iteration of the for loop.
    //Also, according to the BM71's documentation, the last address in the BM70's
    //EEPROM is 0x1FF0. This is why we only loop to it then stop.
    for(uint16_t address = 0x0000; address <= 0x1FF0; address += 0x0080)
    {
        //Reset address is found in the flash block flag
        isAddressInBlock = false;
        
        //Reset the starting index location that we will start from when 
        //searching the flash memory block
        index = 0;
        
        //Search through the Flash Block for the address, WHILE
        //we have not reached the end of the memory block's last index
        //AND
        //We have not found the memory address in the block
        
        //If we find the address, or we reach the end of the indexes
        //in the memory block then stop looking.
        while((index < sizeOfMemoryBlock) &&  !isAddressInBlock)
        {
            //if the current index of the memory block has the same
            //address as the one we are looking for set the isAddressInBlock flag true.
            if(memoryBlock[index].address == address)
                
                //Set the flag that the address was found in the memory block
                isAddressInBlock = true;
            //If the memory address was not found at the current index, then increment it
            //to the next index and keep looking
            else index++;
        }

        //If the address was found in the memory block
        if(isAddressInBlock)
        {
            //Then check that the 128 bytes after that address match the specified
            //bytes in the memory block
            if(!BT_CompareMemory(address, memoryBlock[index].data))
                //if the 128 bytes did not match, then return with a false
                return false;
            
            //If the bytes did match, do nothing and continue testing the rest of
            //the memory
        }
        //Else if the address was not found in the flash memory block
        else
        {
            //Then assume that the 128 bytes after this address should be in the
            //erased state "0xFF". Check that all the following 128 bytes are
            //set to "0xFF". If the address is not found in the memory block it
            //is to be assumed it is a fully erased block.
            if(!BT_CompareEmptyMemory(address))
                
                //If the 128 bytes did not all return as "0xFF" then there is
                //a data mismatch, and return as false.
                return false;
            
            //If all of the 128 bytes are blank then do nothing and continue testing
            //the rest of the memory
        }
    }
    
    //If we reach this point, then we have checked all of the flash memory and
    //everything matched (Since we did not return false yet), so return true.
    return true;
}

bool BT_CompareEmptyMemory(uint16_t address)
{
    if(!BT_Read128Bytes(address))
        return false;
    
    //If the 128 bytes returned from the BM71 is not all set to 0xFF (Not Empty)
    //we return a false
    //
    //We start at i = sizeof(read_Response) to skip the response command sent back. We only care
    //about the data returned, not the response command.
    //
    //We also use i < sizeof(BT_ReadBuffer) to make sure we read all the way to the end of the buffer
    //to read the full 128 bytes.

    for(uint8_t i = sizeof(read_Response); i < sizeof(BT_ReadBuffer); i++)
        if(BT_ReadBuffer[i] != 0xFF)
            return false;
    
    //If the values all matched then return true;
    return true;
}

bool BT_CompareMemory(uint16_t address, uint8_t *expectedData)
{
    ClearReadBuffer();
    
    BT_Read128Bytes(address);
    
    //If the 128 bytes returned from the BM71 does not match the 128 bytes passed
    //into the *expectedData array then return false
    //
    //We start at i = sizeof(read_Response) to skip the response command sent back. We only care
    //about the data returned, not the response command.
    //
    //We also use i < sizeof(BT_ReadBuffer) to make sure we read all the way to the end of the buffer
    //to read the full 128 bytes.

    for(uint8_t i = sizeof(read_Response); i < sizeof(BT_ReadBuffer); i++)
        if(BT_ReadBuffer[i] != *expectedData++)
        {
            return false;
        }
            
    
    //If the values all matched then return true;
    return true;

}

void CheckAndUpdateFlash(void)
{
    //First enter test mode so we can access the BM71 Memory
    BT_ResetToTestMode();
    
    //Send the command to open access to the BM71's memory
    BT_OpenFlashMemory(); 
    
    //Check to see if the BM71's memory matches the current configuration
    if(!BT_CheckConfig())
    {
        //If the memory on the BM71 does NOT match the settings specified by
        //the pic, then update the flash settings

        
        //First completely erase all of the BM71's flash memory
        //(See notes about EEPROM for why we have to do this before we can write)
        BT_EraseFlashMemory();
        
        
        BT_SendConfigToEEPROM();
        if(BT_CheckConfig())
            LED3_SetLow();
    }
    BT_EndFlashMemoryAccess();
    
    BT_ResetToAppMode();
    
}