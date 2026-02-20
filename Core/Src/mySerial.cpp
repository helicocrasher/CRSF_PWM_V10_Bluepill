#include "../inc/mySerial.h"
#include "stm32f1xx_hal_uart.h"							   
#include <cstddef>


mySerial::mySerial()
    : m_huart(nullptr), m_huart_tx_ready(nullptr), m_huart_rx_ready(nullptr),  m_tx_fifo(nullptr), m_rx_fifo(nullptr),m_fifo_size(0),
      m_tx_fifo_head(0), m_tx_fifo_tail(0), m_rx_fifo_head(0), m_rx_fifo_tail(0), m_uart_tx_buffer(nullptr), m_uart_rx_buffer(nullptr), m_uart_tx_buffer_size(0) {}

mySerial::~mySerial() {
    if (m_tx_fifo) delete[] m_tx_fifo;
    if (m_rx_fifo) delete[] m_rx_fifo;
    if (m_uart_tx_buffer) delete[] m_uart_tx_buffer;
    if (m_uart_rx_buffer) delete[] m_uart_rx_buffer;
}

void mySerial::init(UART_HandleTypeDef *huart, bool *huart_tx_ready, bool *huart_rx_ready, size_t fifo_buffer_size, size_t UART_tx_buffer_size) {
    m_huart = huart;
    m_huart_tx_ready = huart_tx_ready;
    m_huart_rx_ready = huart_rx_ready;
    m_fifo_size = fifo_buffer_size;
    m_uart_tx_buffer_size = UART_tx_buffer_size;
    m_tx_fifo_head = m_tx_fifo_tail = 0;
    m_rx_fifo_head = m_rx_fifo_tail = 0;
    if (m_tx_fifo) delete[] m_tx_fifo;
    if (m_rx_fifo) delete[] m_rx_fifo;
    if (m_uart_tx_buffer) delete[] m_uart_tx_buffer;
    if (m_uart_rx_buffer) delete[] m_uart_rx_buffer;
    m_tx_fifo = new uint8_t[m_fifo_size];
    m_rx_fifo = new uint8_t[m_fifo_size];
    m_uart_tx_buffer = new uint8_t[m_uart_tx_buffer_size];
    m_uart_rx_buffer = new uint8_t[m_uart_rx_buffer_size];
    // Start UART receive interrupt - Wait for one character

    *m_huart_rx_ready = false;
    HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer, m_uart_rx_buffer_size);
    HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer, m_uart_rx_buffer_size);
    //
}

int8_t mySerial::restart(){

    m_tx_fifo_head = m_tx_fifo_tail = 0;
    m_rx_fifo_head = m_rx_fifo_tail = 0;
    *m_huart_rx_ready = false;
    HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer, m_uart_rx_buffer_size);
    HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer, m_uart_rx_buffer_size);

    return 0;
}

uint8_t* mySerial::get_uart_rx_buffer(){
	return m_uart_rx_buffer;
}

size_t mySerial::available() {
    return fifo_data_length( m_isRX);
}

size_t mySerial::fifo_free_space(bool m_isTXfifo)  {
    if (m_isTXfifo)	{												 
        if (m_tx_fifo_head >= m_tx_fifo_tail) 
                return m_fifo_size - (m_tx_fifo_head - m_tx_fifo_tail) - 1;
        else    return m_tx_fifo_tail - m_tx_fifo_head - 1;
    }
    else {
        if (m_rx_fifo_head >= m_rx_fifo_tail)
                return m_fifo_size - (m_rx_fifo_head - m_rx_fifo_tail) - 1;
        else    return m_rx_fifo_tail - m_rx_fifo_head - 1;       
    }
}

			 

size_t mySerial::fifo_data_length(bool m_isTXfifo) {
    if (m_isTXfifo){
        if (m_tx_fifo_head >= m_tx_fifo_tail)
                return m_tx_fifo_head - m_tx_fifo_tail;
        else    return m_fifo_size - (m_tx_fifo_tail - m_tx_fifo_head);
    }
    else {
        if (m_rx_fifo_head >= m_rx_fifo_tail)
                return m_rx_fifo_head - m_rx_fifo_tail;
        else    return m_fifo_size - (m_rx_fifo_tail - m_rx_fifo_head);
    } 
}

void mySerial::fifo_push(bool m_isTXfifo, uint8_t c) {
    if (m_isTXfifo){
        m_tx_fifo[m_tx_fifo_head] = c;
        m_tx_fifo_head = (m_tx_fifo_head + 1) % m_fifo_size;
//	    if (m_tx_fifo_head == m_tx_fifo_tail) m_tx_fifo_tail=(m_tx_fifo_head+1)%m_fifo_size; // overflow behavior for UART RX: drop oldest byte
    }
    else{
        m_rx_fifo[m_rx_fifo_head] = c;
        m_rx_fifo_head = (m_rx_fifo_head + 1) % m_fifo_size;
	    if (m_rx_fifo_head == m_rx_fifo_tail) m_rx_fifo_tail=(m_rx_fifo_head+1)%m_fifo_size;  // overflow behavior for UART RX: drop oldest byte
    }
}

uint8_t mySerial::fifo_pop(bool m_isTXfifo) {
    uint8_t c;
    if (m_isTXfifo){    // TX FIFO
       c = m_tx_fifo[m_tx_fifo_tail];
       m_tx_fifo_tail = (m_tx_fifo_tail + 1) % m_fifo_size;
    }
    else{               // RX FIFO
       c = m_rx_fifo[m_rx_fifo_tail];
       m_rx_fifo_tail = (m_rx_fifo_tail + 1) % m_fifo_size;       
    }
    return c;
}

size_t mySerial::write(const uint8_t *data_array, size_t len) {
    size_t written = 0;
    size_t free_space = fifo_free_space(m_isTX);
    size_t to_write = (len < free_space) ? len : free_space;
    for (size_t i = 0; i < to_write; ++i) {
        fifo_push(m_isTX,data_array[i]);
        ++written;
    }
    send();
    return written;
}

size_t mySerial::read( uint8_t *data_array, size_t len) {
    size_t byte_read = 0;
    size_t available_data = fifo_data_length(m_isRX);
    size_t to_read = (len < available_data) ? len : available_data;
    for (size_t i = 0; i < to_read; ++i) {
        data_array[i] = fifo_pop(m_isRX);
        ++	byte_read;
    }
    return byte_read;
}

int8_t mySerial::updateSerial() {
    return send();
}

int8_t mySerial::send() {
    if (!m_huart_tx_ready || !(*m_huart_tx_ready)) //pointer check and ready check
        return -1;
    size_t available = fifo_data_length(m_isTX);
    if (available == 0)
        return 0;
    size_t to_send = (available < m_uart_tx_buffer_size) ? available : m_uart_tx_buffer_size;
    for (size_t i = 0; i < to_send; ++i) {
        m_uart_tx_buffer[i] = fifo_pop(m_isTX);
    }
    *m_huart_tx_ready = false;
    if (HAL_UART_Transmit_IT(m_huart, m_uart_tx_buffer, to_send) == HAL_OK) {
        return (int8_t)to_send;
    } else {
        // On error, push data back to FIFO (not ideal, but prevents loss)
        for (size_t i = 0; i < to_send; ++i) {
            m_tx_fifo_tail = (m_tx_fifo_tail == 0) ? (m_fifo_size - 1) : (m_tx_fifo_tail - 1);
            m_tx_fifo[m_tx_fifo_tail] = m_uart_tx_buffer[to_send - 1 - i];
        }
        *m_huart_tx_ready = true;
        return -1;
    }
}

size_t mySerial::TX_callBackPull() {
    if(!m_isTX) return 0; // Only valid for TX mode
    size_t available = fifo_data_length(m_isTX);
    if(available == 0) return 0; // No data to pull
    size_t to_pull = (available < m_uart_tx_buffer_size) ? available : m_uart_tx_buffer_size;
    for (size_t i = 0; i < to_pull; ++i) {
        m_uart_tx_buffer[i] = fifo_pop(m_isTX);
    }
    HAL_UART_Transmit_IT(m_huart,m_uart_tx_buffer,to_pull);
    return to_pull;
}

int8_t mySerial::receive() {
    if (!m_huart_rx_ready) {   
        return -1;
    }
	if( !*m_huart_rx_ready){
		return fifo_data_length(m_isRX);
    }
	else{
		for (size_t i = 0; i < m_uart_rx_buffer_size; ++i) {
			fifo_push(m_isRX,m_uart_rx_buffer[i]);
		}
	*m_huart_rx_ready = false;
	HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer,m_uart_rx_buffer_size);
    return fifo_data_length(m_isRX);
	}
}

