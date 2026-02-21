#include "../inc/mySerial.h"
#include "stm32f1xx_hal_uart.h"							   
#include <cstddef>
#include <cstdint>


mySerial::mySerial()
    : m_huart(nullptr),  m_tx_fifo(nullptr), m_rx_fifo(nullptr),m_fifo_size(0),
      m_tx_fifo_head(0), m_tx_fifo_tail(0), m_rx_fifo_head(0), m_rx_fifo_tail(0), m_uart_tx_buffer(nullptr), m_uart_rx_buffer(nullptr), m_uart_tx_buffer_size(0) {}

mySerial::~mySerial() {
    if (m_tx_fifo) delete[] m_tx_fifo;
    if (m_rx_fifo) delete[] m_rx_fifo;
    if (m_uart_tx_buffer) delete[] m_uart_tx_buffer;
    if (m_uart_rx_buffer) delete[] m_uart_rx_buffer;
}

void mySerial::init(UART_HandleTypeDef *huart,  size_t fifo_buffer_size, size_t UART_tx_buffer_size) {
    m_huart = huart;
    m_huart_tx_ready = true;
    m_huart_rx_ready = true;
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
    
    // Abort any existing receive operation to prevent conflicts
    HAL_UART_AbortReceive(m_huart);
    
    // Start UART receive interrupt - Wait for one character
    m_huart_rx_ready = false;
    HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer, m_uart_rx_buffer_size);
    HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer, m_uart_rx_buffer_size);
    
    m_initialized = true;  // Mark as fully initialized
}

void mySerial::set_ready_TX() {
    m_huart_tx_ready = true;
}

void mySerial::set_ready_RX() {
    m_huart_rx_ready = true;
}

int8_t mySerial::restart_RX(){
    if (!m_initialized || !m_huart) {
        return -1;  // Not initialized
    }
    
    // Abort any existing receive operation to prevent conflicts
    HAL_UART_AbortReceive(m_huart);
    m_tx_fifo_head = m_tx_fifo_tail = 0;
    m_rx_fifo_head = m_rx_fifo_tail = 0;
    m_huart_rx_ready = false;
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

bool mySerial::is_idle_TX() {
    return fifo_data_length(m_isTX) == 0 && m_huart_tx_ready;
}

bool mySerial::is_idle_RX() {
    return fifo_data_length(m_isRX) == 0 && m_huart_rx_ready;
}

int8_t mySerial::flush(uint32_t m_flush_timeout) {
    if (!m_initialized || !m_huart) {
        return -1;  // Not initialized
    }
    uint32_t start = HAL_GetTick();
    while (!is_idle_TX()) {
        if ((HAL_GetTick() - start) > m_flush_timeout) {
            HAL_UART_AbortTransmit_IT(m_huart);
            m_tx_fifo_head = m_tx_fifo_tail = 0; // Clear TX FIFO
            m_huart_tx_ready = true;
            return -1; // Timeout
        }
    }
    return 0;
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
    if (m_isTXfifo) return (m_tx_fifo_head - m_tx_fifo_tail + m_fifo_size) % m_fifo_size;
    else            return (m_rx_fifo_head - m_rx_fifo_tail + m_fifo_size) % m_fifo_size;

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
    if (!m_initialized || !m_tx_fifo || m_fifo_size == 0) {
        return 0;  // Not initialized
    }
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
    if (!m_initialized || !m_rx_fifo || m_fifo_size == 0) {
        return 0;  // Not initialized
    }
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
    if (!m_initialized || !m_huart || !m_uart_tx_buffer || m_fifo_size == 0) {
        return -1;  // Not initialized
    }
    if (!m_huart_tx_ready) // ready check
        return -1;
    size_t available = fifo_data_length(m_isTX);
    if (available == 0)
        return 0;
    size_t to_send = (available < m_uart_tx_buffer_size) ? available : m_uart_tx_buffer_size;
    for (size_t i = 0; i < to_send; ++i) {
        m_uart_tx_buffer[i] = fifo_pop(m_isTX);
    }
    m_huart_tx_ready = false;
    if (HAL_UART_Transmit_IT(m_huart, m_uart_tx_buffer, to_send) == HAL_OK) {
        return (int8_t)to_send;
    } else {
        // On error, push data back to FIFO (not ideal, but prevents loss)
        for (size_t i = 0; i < to_send; ++i) {
            m_tx_fifo_tail = (m_tx_fifo_tail == 0) ? (m_fifo_size - 1) : (m_tx_fifo_tail - 1);
            m_tx_fifo[m_tx_fifo_tail] = m_uart_tx_buffer[to_send - 1 - i];
        }
        m_huart_tx_ready = true;
        return -1;
    }
}

size_t mySerial::TX_callBackPull() {
    if (!m_initialized || !m_huart || !m_uart_tx_buffer || m_fifo_size == 0) {
        return -1;  // Not initialized
    }
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
    if (!m_initialized || !m_huart || !m_rx_fifo || m_fifo_size == 0) {
        return -1;  // Not initialized
    }
	if( !m_huart_rx_ready){
		return fifo_data_length(m_isRX);
    }
	else{
		for (size_t i = 0; i < m_uart_rx_buffer_size; ++i) {
			fifo_push(m_isRX,m_uart_rx_buffer[i]);
		}
	m_huart_rx_ready = false;
	HAL_UART_Receive_IT(m_huart, m_uart_rx_buffer,m_uart_rx_buffer_size);
    return fifo_data_length(m_isRX);
	}
}

