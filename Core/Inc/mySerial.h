#ifndef MYSERIAL_H
#define MYSERIAL_H

#ifdef __cplusplus

#include "stm32f1xx_hal.h"
#include <cstddef>

class mySerial {
public:
    mySerial();
    ~mySerial();

    void init(UART_HandleTypeDef *huart, bool *huart_tx_ready, bool *huart_rx_ready, size_t fifo_buffer_size = 256, size_t tx_UART_buffer_size = 4);
  
    size_t write(const uint8_t *input_array, size_t len);
 	size_t read(uint8_t *output_array, size_t len);
    size_t TX_callBackPull();
    size_t available();
	size_t fifo_reset(void);
	int8_t restart();
	int8_t receive();
    int8_t send();
	uint8_t* get_uart_rx_buffer();

private:
    UART_HandleTypeDef *m_huart;
    bool *m_huart_tx_ready,*m_huart_rx_ready;
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

    size_t fifo_free_space(bool m_isTX) ;
    size_t fifo_data_length(bool m_isTX) ;

    void fifo_push(bool isTX, uint8_t c);
    uint8_t fifo_pop(bool isTX);
    int8_t updateSerial();
    

};
#endif // __cplusplus
#endif // MYSERIAL_H
