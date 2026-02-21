#ifndef MYSERIAL_H
#define MYSERIAL_H

#ifdef __cplusplus

#include "stm32f1xx_hal.h"
#include <cstddef>

class mySerial {
public:
    mySerial();
    ~mySerial();

    void init(UART_HandleTypeDef *huart, size_t fifo_buffer_size = 256, size_t tx_UART_buffer_size = 4);
  
    size_t write(const uint8_t *input_array, size_t len);
 	size_t read(uint8_t *output_array, size_t len);
    size_t TX_callBackPull();
    size_t available();
	size_t fifo_reset(void);
	int8_t restart_RX();
	int8_t receive();// re-arm UART RX interrupt for next byte - should be called in UART RX callback after processing the received byte to ensure continuous reception
    int8_t send();
    int8_t flush(uint32_t flush_timeout);    // Flush the TX FIFO and wait until all data is sent and the UART is ready for next transmission
    void set_ready_TX(); // for the UART_TX callback to set the TX ready flag when transmission is complete
    void set_ready_RX(); // for the UART_RX callback to set the RX ready flag when reception is complete
    bool is_idle_TX();  // check if UART transmission is idle
    bool is_idle_RX();  // check if UART not waiting for data -> might require restart_RX() to re-arm UART RX interrupt
    bool isInitialized() const { return m_initialized; }  // check if init() has been called
	uint8_t* get_uart_rx_buffer();

private:
    UART_HandleTypeDef *m_huart;
    bool m_huart_tx_ready, m_huart_rx_ready;
    bool m_initialized = false;  // guard against uninitialized usage
	const bool m_isTX=1;
    const bool m_isRX=!m_isTX;
    uint8_t *m_tx_fifo;
    uint8_t *m_rx_fifo;
    size_t m_fifo_size;
    size_t m_tx_fifo_head;
    size_t m_tx_fifo_tail;
    size_t m_rx_fifo_head;
    size_t m_rx_fifo_tail;
    uint8_t *m_uart_tx_buffer;
    uint8_t *m_uart_rx_buffer;
    size_t m_uart_tx_buffer_size;
	size_t m_uart_rx_buffer_size=1;
    uint32_t m_flush_timeout = 1000; // default flush timeout in milliseconds

    size_t fifo_free_space(bool m_isTX) ;
    size_t fifo_data_length(bool m_isTX) ;

    void fifo_push(bool isTX, uint8_t c);
    uint8_t fifo_pop(bool isTX);
    int8_t updateSerial();
    

};
#endif // __cplusplus
#endif // MYSERIAL_H
