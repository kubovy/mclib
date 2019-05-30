/* 
 * File:   bm78_transmission.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * BM78 communication component.
 * 
 * A component must include configuration and can additionally include modules,
 * libs, but no components.
 */
#ifndef BM78_COMMUNICATION_H
#define	BM78_COMMUNICATION_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../lib/requirements.h"
#include "../lib/common.h"
#include "../modules/bm78.h"
#ifdef DHT11_PORT
#include "../modules/dht11.h"
#endif
#ifdef MCP_ENABLED
#include "../modules/mcp23017.h"
#endif
#ifdef RGB_ENABLED
#include "../modules/rgb.h"
#endif
#ifdef SM_MEM_ADDRESS
#include "../modules/state_machine.h"
#endif
#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
#include "../modules/ws281x_light.h"
#else
#include "../modules/ws281x.h"
#endif
#endif

#ifdef BM78_ENABLED
    
#ifndef BMC_QUEUE_SIZE
#warning "BM78 Communication: Queue size defaults to 10"
#define BMC_QUEUE_SIZE 10
#endif

#define BMC_PARAM_MASK 0x7F
#define BMC_PARAM_ALL 0x80
#ifdef LCD_ADDRESS
#define BMC_PARAM_LCD_CLEAR 0x7B
#define BMC_PARAM_LCD_RESET 0x7C
#define BMC_PARAM_LCD_NO_BACKLIGH 0x7D
#define BMC_PARAM_LCD_BACKLIGH 0x7E
#endif

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
typedef bool (*BMC_NextMessageHandler_t)(uint8_t what, uint8_t param);

/** Adds a settings message to the send queue. */
inline void BMC_sendSettings(void);

#ifdef DHT11_PORT
/** Adds a DHT11 message to the send queue. */
inline void BMC_sendDHT11(void);
#endif

#ifdef LCD_ADDRESS
/**
 * Adds a LCD message to the send queue.
 * 
 * @param line Which line to send. Use BMC_PARAM_ALL to send the while content.
 */
inline void BMC_sendLCD(uint8_t line);

inline void BMC_sendLCDBacklight(bool on);
#endif

#ifdef PIR_PORT
/** Adds a PIR message to the send queue. */
inline void BMC_sendPIR(void);
#endif

#ifdef MCP_ENABLED
/** Adds a MCP23017 message to the send queue. */
inline void BMC_sendMCP23017(uint8_t address);
#endif

#ifdef RGB_ENABLED
/** Adds a RGB message to the send queue. */
inline void BMC_sendRGB(uint8_t index);
#endif

#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
/**
 * Adds a WS281x Light configuration to the send queue.
 * 
 * @param index Configuration index to send. Use BMC_PARAM_ALL to send all
 *              configurations.
 */
inline void BMC_sendWS281xLight(uint8_t index);
#else
/**
 * Adds a WS281x LED message to the send queue.
 * 
 * @param led LED of which to transfer the configuration. Use BMC_PARAM_ALL to
 *            send all LED configurations of the whole strip.
 */
inline void BMC_sendWS281xLED(uint8_t led);
#endif
#endif

/**
 * Next message handler setter. This handler should implement sending a 
 * particular message.
 * 
 * Next message handler type should implement sending a message type define by
 * the "what" parameter. 
 * 
 * The prototype: bool BMC_NextMessageHandler_t(uint8_t what, uint8_t param)
 * - param what : What to send (message type)
 * - param param: Message type parameter.
 * - return     : Whether the queue item should be consumed or not. In some 
 *                cases this can be used to send one message types over multiple
 *                packets due to packet size limitation.
 * 
 * @param NextMessageHandler The handler.
 */
void BMC_setNextMessageHandler(BMC_NextMessageHandler_t nextMessageHandler);

/**
 * Enqueues transmission type with possible parameter.
 * 
 * @param what Definition what to transmit.
 * @param param Possible parameter.
 */
void BMC_enqueue(uint8_t what, uint8_t param);

/**
 * Called when a BT message was sent. 
 *
 * This method is supposed to determine it there is somehing to be send to the
 * connected bluetooth device and send it.
 * 
 * It implement sending messages enqueued with the BMC_enqueue function. 
 * Known message types, e.g., settings, dht11, pir, mcp23017, rgb, ws281x,
 * plain, are sent by this handler.
 * 
 * All non-recognized message types should be send by a BMC_NextMessageHandler_t
 * handler optionally set by the  BMC_setNextMessageHandler function.
 */
void BMC_bm78MessageSentHandler(void);

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
 * @param length Transparent data length.
 * @param data Transparent data.
 */
void BMC_bm78TransparentDataHandler(uint8_t length, uint8_t *data);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* BM78_TRANSMISSION_H */

