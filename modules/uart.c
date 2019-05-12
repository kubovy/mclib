#include "uart.h"

#if defined UART || defined EUSART

#if defined UART
#include "../../mcc_generated_files/uart1.h"
#elif defined EUSART
#include "../../mcc_generated_files/eusart.h"
#endif


inline bool UART_isRXReady(void) {
#if defined UART
    return UART1_is_rx_ready();
#elif defined EUSART
    return EUSART_is_rx_ready();
#endif
}

inline bool UART_isTXReady(void) {
#if defined UART
    return UART1_is_tx_ready();
#elif defined EUSART
    return EUSART_is_tx_ready();
#endif
}


inline bool UART_isTXDone(void) {
#if defined UART
    return UART1_is_tx_done();
#elif defined EUSART
    return EUSART_is_tx_done();
#endif
}


inline uint8_t UART_read(void) {
#if defined UART
    return UART1_Read();
#elif defined EUSART
    return EUSART_Read();
#endif
}


inline void UART_write(uint8_t byte) {
#if defined UART
    UART1_Write(byte);
#elif defined EUSART
    EUSART_Write(byte);
#endif
}

#endif
