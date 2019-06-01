/* 
 * File:   serial_communication.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */

#ifndef SERIAL_COMMUNICATION_H
#define	SERIAL_COMMUNICATION_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../lib/requirements.h"
#include "../lib/common.h"
#ifdef BM78_ENABLED
#include "../modules/bm78.h"
#endif
#ifdef DHT11_PORT
#include "../modules/dht11.h"
#endif
#ifdef MCP23017_ENABLED
#include "../modules/mcp23017.h"
#endif
#ifdef RGB_ENABLED
#include "../modules/rgb.h"
#endif
#ifdef SM_MEM_ADDRESS
#include "../modules/state_machine.h"
#endif
#if defined MCP2200_ENABLED || defined MCP2221_ENABLED
#include "../modules/mcp22xx.h"
#endif
#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
#include "../modules/ws281x_light.h"
#else
#include "../modules/ws281x.h"
#endif
#endif
    
#if defined SCOM_ENABLED

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

/**
 * Next message handler type should implement sending a message type define by
 * the "what" parameter.
 * 
 * @param what What to send (message type)
 * @param param Message type parameter.
 * @return Whether the queue item should be consumed or not. In some cases this
 *         can be used to send one message types over multiple packets due to
 *         packet size limitation.
 */
typedef bool (*SCOM_NextMessageHandler_t)(uint8_t what, uint8_t param);

void SCOM_setMessageSentHandler(Procedure_t messageSentHandler);

void SCOM_cancelTransmission(SCOM_Channel_t channel);

/**
 * Resets checksum of last transparent data reception.
 */
void SCOM_resetChecksum(SCOM_Channel_t channel);

/** Whether still awaiting confirmation for last command. */
bool SCOM_awatingConfirmation(SCOM_Channel_t channel);

/**
 * Validates checksum of last transparent data reception.
 *
 * @return Whether the last received checksum is valid with the last transmitted
 *         message
 */
bool SCOM_isChecksumCorrect(SCOM_Channel_t channel);

/**
 * Transparent data transmission retry trigger.
 * 
 * This function should be called periodically, e.g. by a timer.
 */
void SCOM_retryTrigger(void);

/**
 * Add one data byte to the prepared message.
 *
 * @param position Position to put the byte to.
 * @param byte The data byte.
 */
inline void SCOM_addDataByte(SCOM_Channel_t channel, uint8_t position, uint8_t byte);

/**
 * Add one word (2 bytes) to the prepared message.
 *
 * @param position Starting position.
 * @param word The word.
 */
inline void SCOM_addDataByte2(SCOM_Channel_t channel, uint8_t position, uint16_t word);

/**
 * Add data bytes to the prepared message.
 *
 * @param position Starting position.
 * @param length Length of the data.
 * @param data The data.
 */
inline void SCOM_addDataBytes(SCOM_Channel_t channel, uint8_t position, uint8_t length, uint8_t *data);

/**
 * Commit prepared message and send it.
 *
 * @param length Total length of the message.
 * @param maxRetries Maximum number of retries.
 * @return Whether committing was successful or not. Only one message may be
 *         sent at a time. In case a message is currently being send, calling
 *         this function will have no effect and return false.
 */
bool SCOM_commitData(SCOM_Channel_t channel, uint8_t length, uint8_t maxRetries);

/**
 * Transmit transparent data over the bluetooth module.
 * 
 * @param length Length of the data
 * @param data Data pointer
 */
void SCOM_transmitData(SCOM_Channel_t channel, uint8_t length, uint8_t *data, uint8_t maxRetries);

#ifdef BM78_ENABLED
/**
 * Adds a settings message to the send queue. 
 * 
 * @param channel Channel to send the message over.
 */
inline void SCOM_sendBluetoothSettings(SCOM_Channel_t channel);
#endif

#ifdef DHT11_PORT
/**
 * Adds a DHT11 message to the send queue.
 * 
 * @param channel Channel to send the message over.
 */
inline void SCOM_sendDHT11(SCOM_Channel_t channel);
#endif

#ifdef LCD_ADDRESS
/**
 * Adds a LCD message to the send queue.
 * 
 * @param channel Channel to send the message over.
 * @param line Which line to send. Use SCOM_PARAM_ALL to send the while content.
 */
inline void SCOM_sendLCD(SCOM_Channel_t channel, uint8_t line);

/**
 * Adds a LCD's backlight status message to the send queue.
 * 
 * @param channel Channel to send the message over.
 * @param on True if backlight is on, false otherwise.
 */
inline void SCOM_sendLCDBacklight(SCOM_Channel_t channel, bool on);
#endif

#ifdef PIR_PORT
/**
 * Adds a PIR message to the send queue.
 * 
 * @param channel Channel to send the message over.
 */
inline void SCOM_sendPIR(SCOM_Channel_t channel);
#endif

#ifdef MCP23017_ENABLED
/**
 * Adds a MCP23017 message to the send queue.
 * 
 * @param channel Channel to send the message over.
 * @param address I2C address of the MCP23017 chip.
 */
inline void SCOM_sendMCP23017(SCOM_Channel_t channel, uint8_t address);
#endif

#ifdef RGB_ENABLED
/** 
 * Adds a RGB message to the send queue.
 * 
 * @param channel Channel to send the message over.
 * @param index Index of the configuration to send. Use SCOM_PARAM_ALL to send
 *              all configurations.
 */
inline void SCOM_sendRGB(SCOM_Channel_t channel, uint8_t index);
#endif

#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
/**
 * Adds a WS281x Light configuration to the send queue.
 * 
 * @param channel Channel to send the message over.
 * @param index Configuration index to send. Use SCOM_PARAM_ALL to send all
 *              configurations.
 */
inline void SCOM_sendWS281xLight(SCOM_Channel_t channel, uint8_t index);
#else
/**
 * Adds a WS281x LED message to the send queue.
 * 
 * @param channel Channel to send the message over.
 * @param led LED of which to transfer the configuration. Use SCOM_PARAM_ALL to
 *            send all LED configurations of the whole strip.
 */
inline void SCOM_sendWS281xLED(SCOM_Channel_t channel, uint8_t led);
#endif
#endif

/**
 * Next message handler setter. This handler should implement sending a 
 * particular message.
 * 
 * Next message handler type should implement sending a message type define by
 * the "what" parameter. 
 * 
 * The prototype: bool SCOM_NextMessageHandler_t(uint8_t what, uint8_t param)
 * - param what : What to send (message type)
 * - param param: Message type parameter.
 * - return     : Whether the queue item should be consumed or not. In some 
 *                cases this can be used to send one message types over multiple
 *                packets due to packet size limitation.
 * 
 * @param channel Channel the handler is for.
 * @param nextMessageHandler The handler.
 */
void SCOM_setNextMessageHandler(SCOM_Channel_t channel, SCOM_NextMessageHandler_t nextMessageHandler);

/**
 * Enqueues transmission type with possible parameter.
 * 
 * @param channel Channel to send the message over.
 * @param what Definition what to transmit.
 * @param param Possible parameter.
 */
void SCOM_enqueue(SCOM_Channel_t channel, MessageKind_t what, uint8_t param);

/**
 * Called when a BT message was sent. 
 *
 * This method is supposed to determine it there is somehing to be send to the
 * connected bluetooth device and send it.
 * 
 * It implement sending messages enqueued with the SCOM_enqueue function. 
 * Known message types, e.g., settings, dht11, pir, mcp23017, rgb, ws281x,
 * plain, are sent by this handler.
 * 
 * All non-recognized message types should be send by a SCOM_NextMessageHandler_t
 * handler optionally set by the  SCOM_setNextMessageHandler function.
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
 * @param channel Channel to send the message over.
 * @param length Transparent data length.
 * @param data Transparent data.
 */
void SCOM_dataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_COMMUNICATION_H */

