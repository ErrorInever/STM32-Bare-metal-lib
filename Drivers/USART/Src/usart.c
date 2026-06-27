#include "usart.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#define ENTER_CRITICAL() __disable_irq()
#define EXIT_CRITICAL()  __enable_irq()

// Size of ringbuffer: must be a power of two
#define RING_BUF_SIZE 128
#define RING_BUF_DMA_SIZE_TX 128
#define RING_BUF_DMA_SIZE_RX 512

#define NUM_USART 4
// Usart hanlder
static usart_t *usart_handler[NUM_USART] = {NULL};

static uint8_t rx_storage[4][RING_BUF_SIZE];    // ringbuffer recive storage
static RingBuffer_t rx_handler[4];              // ringbuffer recive struct

static uint8_t tx_storage[4][RING_BUF_SIZE];    // ringbuffer recive storage
static RingBuffer_t tx_handler[4];              // ringbuffer recive struct

// DMA storage
//static uint8_t dma_tx_storage[RING_BUF_DMA_SIZE_TX];
static uint8_t dma_rx_storage[RING_BUF_DMA_SIZE_RX];

//static RingBuffer_t tx_handler_circular;
static RingBuffer_t rx_handler_circular;


static uint8_t get_usart_index(const USART_TypeDef *usart) {
    if(usart == USART1) { return 0; }
    else if(usart == USART2) { return 1; }
    else if(usart == USART3) { return 2; }
    else if(usart == USART6) { return 3; }
    return 4;
}

static void usart_enable_clock(USART_TypeDef *usart) {
    if(usart == USART1) { RCC->APB2ENR |= RCC_APB2ENR_USART1EN; }
    else if(usart == USART2) { RCC->APB1ENR |= RCC_APB1ENR_USART2EN; }
    else if(usart == USART3) { RCC->APB1ENR |= RCC_APB1ENR_USART3EN; }
    else if(usart == USART6) { RCC->APB2ENR |= RCC_APB2ENR_USART6EN; }
    (void)RCC->APB1ENR;
    (void)RCC->APB2ENR;
}

void usart_init(usart_t *usart, uint32_t baudrate) {
    assert(usart->instance != NULL);
    assert((usart->instance->CR1 & USART_CR1_OVER8) == 0);
    uint8_t idx = get_usart_index(usart->instance);
    assert(idx != 4);
    // RCC
    usart_enable_clock(usart->instance);
    // Init ringbuffer for USART
    ring_buffer_init(&rx_handler[idx], rx_storage[idx], 
        RING_BUF_SIZE, sizeof(uint8_t));
    usart->rx_buffer = &rx_handler[idx];
   ring_buffer_init(&tx_handler[idx], tx_storage[idx], 
        RING_BUF_SIZE, sizeof(uint8_t));
    usart->tx_buffer = &tx_handler[idx];
    // Setup baudrate
    usart->baudrate = baudrate;
    uint32_t div = (usart->bus_freq + baudrate/2) / baudrate;        // valid only for OVER8=0
    // Configure USART
    usart->instance->BRR = div;
    usart->instance->CR1 |= USART_CR1_TE |                           // Enable transmitter
                            USART_CR1_RE |                           // Enable receiver
                            USART_CR1_RXNEIE |                       // Enable interrupt on receive
                            USART_CR1_UE;                            // Enable USART
    usart->instance->CR1 &= ~USART_CR1_TXEIE;

    // Enable IRQs
    if(usart->instance == USART1) { NVIC_EnableIRQ(USART1_IRQn); }
    else if(usart->instance == USART2) {NVIC_EnableIRQ(USART2_IRQn); }
    else if(usart->instance == USART3) {NVIC_EnableIRQ(USART3_IRQn); }
    else if(usart->instance == USART6) {NVIC_EnableIRQ(USART6_IRQn); }
}

void usart2_rx_init_dma(usart_t *usart, uint32_t baudrate) {
    assert(usart->instance != NULL);
    assert(usart->instance == USART2);
    usart_handler[1] = usart;
    // Configure DMA
    // RCC DMA1 & USART
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    usart_enable_clock(usart->instance);
    (void)RCC->AHB1ENR;

    // Init instance
    usart->dma_instance = DMA1;
    usart->dma_stream_rx = DMA1_Stream5;
    usart->dma_rx_channel = 4U;

    // Disable streams
    usart->dma_stream_rx->CR &= ~DMA_SxCR_EN;
    while(usart->dma_stream_rx->CR & DMA_SxCR_EN);
    // setup USART BR
    usart->baudrate = baudrate;
    uint32_t div = (usart->bus_freq + baudrate / 2) / baudrate;        
    usart->instance->BRR = div;

    // init rx buffer
    ring_buffer_init(&rx_handler_circular, dma_rx_storage, 
        RING_BUF_DMA_SIZE_RX, sizeof(uint8_t));
    usart->rx_buffer = &rx_handler_circular;
    
    uint8_t idx = get_usart_index(usart->instance);
    assert(idx != 4);
    // init tx buffer for testing printf
    ring_buffer_init(&tx_handler[idx], tx_storage[idx], RING_BUF_SIZE, sizeof(uint8_t));
    usart->tx_buffer = &tx_handler[idx];

    // configure addresses
    usart->dma_stream_rx->PAR = (uint32_t)&usart->instance->DR; // from Data register
    usart->dma_stream_rx->M0AR = (uint32_t)dma_rx_storage;    // to ring buffer
    usart->dma_stream_rx->NDTR = RING_BUF_DMA_SIZE_RX;

    // configure DMA CR
    uint32_t rx_temp_cr = 0;
    rx_temp_cr |= ((uint32_t)usart->dma_rx_channel << DMA_SxCR_CHSEL_Pos); // select channel
    rx_temp_cr |= DMA_SxCR_MINC;                                        // enable memory increment
    rx_temp_cr &= ~DMA_SxCR_PINC;                                       // disable peripheral increment
    rx_temp_cr &= ~DMA_SxCR_DIR;                                        // direction peripheral to memory
    rx_temp_cr |= DMA_SxCR_CIRC;                                        // enable circular mode

    // configure USART
    usart->instance->CR1 |= USART_CR1_TE | 
                            USART_CR1_RE | 
                            USART_CR1_IDLEIE | 
                            USART_CR1_UE;

    
    usart->dma_stream_rx->CR = rx_temp_cr;
    // reset flags
    if (usart->dma_stream_rx == DMA1_Stream2) {
        DMA1->LIFCR = (0x3DU << 16); // reset FEIF, DMEIF, TEIF, HTIF, TCIF for Stream 2
    } else if (usart->dma_stream_rx == DMA1_Stream5) {
        DMA1->HIFCR = (0x3DU << 6);  // reset FEIF, DMEIF, TEIF, HTIF, TCIF for Stream 5
    }
    
    // enable stream in DMA
    usart->dma_stream_rx->CR |= DMA_SxCR_EN;
    while (!(usart->dma_stream_rx->CR & DMA_SxCR_EN));
    // disable interrupts rxne
    usart->instance->CR1 &= ~USART_CR1_RXNEIE;
    // enable DMA in USART and irq IDLE line detection
    usart->instance->CR3 |= USART_CR3_DMAR;
    usart->instance->CR1 |= USART_CR1_IDLEIE;
    // reset DMA IDLE tail
    usart->dma_rx_tail = 0U;
    // enable irq in NVIC
    NVIC_EnableIRQ(USART2_IRQn);
}

void usart_send(const usart_t *usart, const char *str) {
    uint8_t idx = get_usart_index(usart->instance);
    ENTER_CRITICAL();
    while(*str) {
        char ch = *str++;
        ring_buffer_push(&tx_handler[idx], &ch);
        
    }
    usart->instance->CR1 |= USART_CR1_TXEIE;
    EXIT_CRITICAL();
}

bool usart_available(const usart_t *usart) {
    return !is_empty(usart->rx_buffer);
}

bool usart_read_byte(const usart_t *usart, uint8_t *out_data) {
    return ring_buffer_pop(usart->rx_buffer, out_data);
}

void usart_printf(const usart_t *usart, const char *format, ...) {
    char temp_str[128]; // temp buffer for formatted str
    va_list args;
    va_start(args, format);
    // Formatting the string into our temporary buffer
    int len = vsnprintf(temp_str, sizeof(temp_str), format, args);
    va_end(args);

    if (len < 0)
        return;
    if (len >= sizeof(temp_str))
        len = sizeof(temp_str) - 1;
    if(len > 0) {
        // Push each character into the ring buffer
        ENTER_CRITICAL();
        for (int i = 0; i < len; i++) {
            ring_buffer_push(usart->tx_buffer, &temp_str[i]);
        }
        usart->instance->CR1 |= USART_CR1_TXEIE;
        EXIT_CRITICAL();
    }
}

void process_simple_commands(const usart_t *usart) {
    uint8_t ch;
    // if rx not empty
    if(ring_buffer_pop(usart->rx_buffer, &ch)) {
        if (ch == '1') {
            usart_send(usart, "LOVIL 1: LED ON\r\n");
            usart_printf(usart, "TEST SIMPLE 1 + 1 = %d", 2);
        } 
        else if (ch == '2') {
            usart_send(usart, "LOVIL 2: LED OFF\r\n");
        } 
        else if (ch == 'h' || ch == 'H') {
            usart_send(usart, "REZHIM: 1-ON, 2-OFF, H-HELP\r\n");
        } 
        else {
            // echo-mode
            while (!(usart->instance->SR & USART_SR_TXE));
            usart->instance->DR = ch;
        }
    }
}

void usart2_send_dma(usart_t *usart, const uint8_t *data, uint16_t size) {
    // wait previous transmitt is ready
    while (usart->dma_stream_tx->CR & DMA_SxCR_EN);
    assert(usart->instance != NULL);
    assert(usart->instance == USART2);

    // Init instance
    usart->dma_instance = DMA1;
    usart->dma_stream_tx = DMA1_Stream6;
    usart->dma_tx_channel = 4U;

    // reset flags
    DMA1->HIFCR = 0x00FC0000U;

    // configure addresses
    usart->dma_stream_tx->PAR  = (uint32_t)&usart->instance->DR; // to peripheral
    usart->dma_stream_tx->M0AR = (uint32_t)data;                // from memory
    usart->dma_stream_tx->NDTR = size;                          // size

    // CR configure
    uint32_t cr = 0;
    cr |= ((uint32_t)usart->dma_tx_channel << DMA_SxCR_CHSEL_Pos); // select channel 4
    cr |= DMA_SxCR_MINC;                                       // increment memory
    cr |= DMA_SxCR_DIR_0;                                      // direction M2P 
    cr &= ~DMA_SxCR_CIRC;                                      // mode NORMAL
    cr |= DMA_SxCR_TCIE;                                       // enable interrupt Transmit complete
    usart->dma_stream_tx->CR = cr;

    // enable irq in DMA
    usart->instance->CR3 |= USART_CR3_DMAT;
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    usart->dma_stream_tx->CR |= DMA_SxCR_EN;
}


static void generic_usart_irq_handler(USART_TypeDef *instance, uint8_t idx) {
    uint32_t sr = instance->SR;
    uint32_t cr1 = instance->CR1;

    // DMA
    // IDLE line detection
    if ((sr & USART_SR_IDLE) && (cr1 & USART_CR1_IDLEIE)) {
        usart_t *usart = usart_handler[idx];
        // reset IDLE flag
        volatile uint32_t dummy = USART2->SR;
        dummy = USART2->DR;
        (void)dummy;
        // calculate the number of bytes received
        uint16_t dma_head = RING_BUF_DMA_SIZE_RX - usart->dma_stream_rx->NDTR;  // get current head
        uint16_t num_byte = 0U;
        if(dma_head != usart->dma_rx_tail) { // if buffer not empty
            if(dma_head > usart->dma_rx_tail) {
                num_byte = dma_head - usart->dma_rx_tail;
            } else {
                num_byte = (RING_BUF_DMA_SIZE_RX - usart->dma_rx_tail) + dma_head;
            }
            // push into ring buffer
            for (uint16_t i = 0; i < num_byte; i++) {
                uint8_t byte = dma_rx_storage[usart->dma_rx_tail];
                ring_buffer_push(usart->rx_buffer, &byte);
                usart->dma_rx_tail++;
                if (usart->dma_rx_tail >= RING_BUF_DMA_SIZE_RX) {
                    usart->dma_rx_tail = 0;
                }
            }

            // user callback
            usart->callback(usart, num_byte);

            // update tail after read
            usart->dma_rx_tail = dma_head;
        }
        
    }
    // error flags
    if(sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t dummy;
        dummy = instance->SR;       // read SR
        dummy = instance->DR;       // read DR (reset error flags)
        (void)dummy;
    }

    // Interrupts
    // If data is ready to read i.e. RXNE = 1 AND RXNEIE (Enabled interrupt)
    if ((sr & USART_SR_RXNE) && (cr1 & USART_CR1_RXNEIE)) {
        uint8_t received_byte = usart_receive_byte(instance);
        // TODO add overflow lost data printf
        ring_buffer_push(&rx_handler[idx], &received_byte);
    // If DR register is empty i.e. TXE = 1 AND TXEIE
    } else if((sr & USART_SR_TXE) && (cr1 & USART_CR1_TXEIE)) {
        // send byte
        uint8_t out_data;
        if(ring_buffer_pop(&tx_handler[idx], &out_data)) {
            instance->DR = out_data;
        } else {
            instance->CR1 &= ~USART_CR1_TXEIE; // buffer is empty, reset TXEIE flag
        }
    }
}

// DMA TX interrupt

void DMA1_Stream6_IRQHandler(void) {
    // check flag TCIF6
    if(DMA1->HISR & DMA_HISR_TCIF6) {
        // reset flag
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        // disable irq in DMA
        usart_handler[1]->instance->CR3 &= ~USART_CR3_DMAT;
    }
}

// IRQ Handlers
void USART1_IRQHandler(void) { generic_usart_irq_handler(USART1, 0); }
void USART2_IRQHandler(void) { generic_usart_irq_handler(USART2, 1); }
void USART3_IRQHandler(void) { generic_usart_irq_handler(USART3, 2); }
void USART6_IRQHandler(void) { generic_usart_irq_handler(USART6, 3); }