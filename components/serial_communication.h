/* 
 * File:   serial_communication.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#ifndef SERIAL_COMMUNICATION_H
#define	SERIAL_COMMUNICATION_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"
    
#if defined SCOM_ENABLED

#include <stdint.h>
#include "../modules/bm78.h"

#ifndef SCOM_QUEUE_SIZE
#warning "SCOM: Queue size defaults to 10"
#define SCOM_QUEUE_SIZE 10
#endif
    
#ifndef SCOM_MAX_PACKET_SIZE
#error "SCOM: Max packet size must be defined!"
#endif

#define SCOM_PARAM_MASK 0x7F
#define SCOM_PARAM_ALL 0x80
#ifdef LCD_ADDRESS
#define SCOM_PARAM_LCD_CLEAR 0x7B
#define SCOM_PARAM_LCD_RESET 0x7C
#define SCOM_PARAM_LCD_NO_BACKLIGH 0x7D
#define SCOM_PARAM_LCD_BACKLIGH 0x7E
#endif

#if defined USB_ENABLED && defined BM78_ENABLED
#define SCOM_CHANNEL_COUNT 2
#elif defined USB_ENABLED || defined BM78_ENABLED
#define SCOM_CHANNEL_COUNT 1
#endif

#define SCOM_NO_RETRY_LIMIT 0xFF // no retry limit

typedef enum {
#if defined USB_ENABLED && defined BM78_ENABLED
    SCOM_CHANNEL_USB = 0x00,
    SCOM_CHANNEL_BT = 0x01
#elif defined USB_ENABLED
    SCOM_CHANNEL_USB = 0x00
#elif defined BM78_ENABLED
    SCOM_CHANNEL_BT = 0x00
#endif
} SCOM_Channel_t;

struct {
    uint8_t stage;
    SCOM_Channel_t channel;
    uint16_t start;
    uint16_t end;
} SCOM_dataTransfer = { 0x00, 0x00, 0x0000, 0x0000 };

/**
 * Data handler.
 *
 * @param channel Channel.
 * @param length Length of data.
 * @param data The data.
 */
typedef void (*SCOM_DataHandler_t)(SCOM_Channel_t channel, uint8_t length, uint8_t *data);

/**
 * Next message handler type should implement sending a message type define by
 * the "what" parameter.
 *
 * @param channel Channel.
 * @param what What to send (message type)
 * @param param1 Message type parameter 1.
 * @param param2 Message type parameter 2.
 * @return Whether the queue item should be consumed or not. In some cases this
 *         can be used to send one message types over multiple packets due to
 *         packet size limitation.
 */
typedef bool (*SCOM_NextMessageHandler_t)(SCOM_Channel_t channel, uint8_t what,
        uint8_t param1, uint8_t param2);

/**
 * Cancels any ongoing transmission.
 * 
 * @param channel Channel to use.
 */
void SCOM_cancelTransmission(SCOM_Channel_t channel);

/**
 * Resets checksum.
 * 
 * @param channel Channel to use.
 */
void SCOM_resetChecksum(SCOM_Channel_t channel);

/**
 * Whether still awaiting confirmation for last command.
 * 
 * @param channel Channel to use.
 */
bool SCOM_awatingConfirmation(SCOM_Channel_t channel);

/**
 * Validates checksum of last transparent data reception.
 *
 * @param channel Channel to use.
 * @return Whether the last received checksum is valid with the last transmitted
 *         message
 */
bool SCOM_isChecksumCorrect(SCOM_Channel_t channel);

/**
 * Checks if messages can be enqueued on the given channel. Usually messages
 * cannot be enqueued when the given channel is disconnected.
 * 
 * @param channel Channel to check.
 * @return Whether messages can be enqueued to the given channel.
 */
inline bool SCOM_canEnqueue(SCOM_Channel_t channel);

/**
 * Checks if messages can be send over the given channel. Usually messages
 * cannot be send when the given channel is disconnected or awaiting something
 * from the other side (e.g. confirmation).
 * 
 * @param channel Channel to check.
 * @return  Whether messages can be send over the given channel.
 */
inline bool SCOM_canSend(SCOM_Channel_t channel);

/**
 * Transparent data transmission retry trigger.
 * 
 * This function should be called periodically, e.g. inside the program loop.
 */
void SCOM_retryTrigger(void);

/**
 * Add one data byte to the prepared message.
 *
 * @param channel Channel to use.
 * @param position Position to put the byte to.
 * @param byte The data byte.
 */
inline void SCOM_addDataByte(SCOM_Channel_t channel, uint8_t position, uint8_t byte);

/**
 * Add one word (2 bytes) to the prepared message.
 *
 * @param channel Channel to use.
 * @param position Starting position.
 * @param word The word.
 */
inline void SCOM_addDataByte2(SCOM_Channel_t channel, uint8_t position, uint16_t word);

/**
 * Add data bytes to the prepared message.
 *
 * @param channel Channel to use.
 * @param position Starting position.
 * @param length Length of the data.
 * @param data The data.
 */
inline void SCOM_addDataBytes(SCOM_Channel_t channel, uint8_t position, uint8_t length, uint8_t *data);

/**
 * Commit prepared message and send it.
 *
 * @param channel Channel to use.
 * @param length Total length of the message.
 * @param maxRetries Maximum number of retries.
 * @return Whether committing was successful or not. Only one message may be
 *         sent at a time. In case a message is currently being send, calling
 *         this function will have no effect and return false.
 */
bool SCOM_commitData(SCOM_Channel_t channel, uint8_t length, uint8_t maxRetries);

/**
 * Transmit transparent data over the channel.
 * 
 * @param channel Channel to use.
 * @param length Length of the data
 * @param data Data pointer
 */
void SCOM_transmitData(SCOM_Channel_t channel, uint8_t length, uint8_t *data, uint8_t maxRetries);

#ifdef BM78_ENABLED

/**
 * Data download message to the send queue.
 * 
 * @param channel Channel to use.
 * @param num Data part/ID
 * @param start Start address.
 * @param length Data length.
 */
inline void SCOM_sendData(SCOM_Channel_t channel, uint8_t num, uint16_t start, uint16_t length);
#endif

/**
 * Additional data handler setter.
 *
 * @param handler Data handler.
 */
void SCOM_setAdditionalDataHandler(SCOM_DataHandler_t handler);

/**
 * Next message handler setter. This handler should implement sending a 
 * particular message.
 * 
 * Next message handler type should implement sending a message type define by
 * the "what" parameter. 
 * 
 * The prototype: bool SCOM_NextMessageHandler_t(SCOM_Channel_t channel, uint8_t what, uint8_t param1, uint8_t param2)
 * - param channel: Channel.
 * - param what   : What to send (message type)
 * - param param1  : Message type parameter 1.
 * - param param2  : Message type parameter 2.
 * - return       : Whether the queue item should be consumed or not. In some 
 *                  cases this can be used to send one message types over
 *                  multiple packets due to packet size limitation.
 * 
 * @param channel Channel the handler is for.
 * @param nextMessageHandler The handler.
 */
void SCOM_setNextMessageHandler(SCOM_NextMessageHandler_t nextMessageHandler);

/**
 * Check whether channel's queue is empty
 *
 * @param channel Channel.
 * @return True/false
 */
bool SCOM_isQueueEmpty(SCOM_Channel_t channel);

/**
 * Enqueues transmission type with possible parameter.
 * 
 * @param channel Channel to use.
 * @param what Definition what to transmit.
 * @param param1 Possible parameter 1.
 * @param param2 Possible parameter 2.
 */
void SCOM_enqueue(SCOM_Channel_t channel, MessageKind_t what, uint8_t param1,
        uint8_t param2);

/**
 * Called when a BT message was sent. 
 *
 * This method is supposed to determine it there is something to be send to the
 * connected device and send it.
 * 
 * It implement sending messages enqueued with the SCOM_enqueue function. 
 * Known message types, e.g., settings, dht11, pir, mcp23017, rgb, ws281x,
 * plain, are sent by this handler.
 * 
 * All non-recognized message types should be send by a SCOM_NextMessageHandler_t
 * handler optionally set by the  SCOM_setNextMessageHandler function.
 * 
 * @param channel Channel to use.
 */
void SCOM_messageSentHandler(SCOM_Channel_t channel);

/**
 * BM78's transparent data handler implementation.
 * 
 * This particular implementation listens to data packets and recognizes known
 * messages, e.g., settings, dht11, pir, mcp23017, rgb, ws281x, plain. It reacts
 * to those messages by calling appropriate functions in the corresponding
 * modules.
 * 
 * Non-recognized message are ignored here and another BM78 Transparent Data
 * Handler should be implemented in an appropriate module to handle those.
 * 
 * @param channel Channel to use.
 * @param length Transparent data length.
 * @param data Transparent data.
 */
void SCOM_dataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data);

/**
 * BM78's test mode response handler for dealing with BM78's EEPROM.
 * 
 * This needs to be add to the BM78's test response handlers.
 * 
 * @param response The response.
 */
void SCOM_bm78TestModeResponseHandler(BM78_Response_t *response);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_COMMUNICATION_H */

