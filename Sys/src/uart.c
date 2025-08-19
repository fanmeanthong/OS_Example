#include "uart.h"

#define GPIOA_CRH   (*(volatile uint32_t*)(GPIOA_BASE + GPIO_CRH_OFFSET))
#define USART1_BRR  (*(volatile uint32_t*)(USART1_BASE + 0x08))
#define USART1_CR1  (*(volatile uint32_t*)(USART1_BASE + 0x0C))
#define USART1_SR   (*(volatile uint32_t*)(USART1_BASE + 0x00))
#define USART1_DR   (*(volatile uint32_t*)(USART1_BASE + 0x04))

/**
 * @brief  Initialize USART1: PA9 = TX, PA10 = RX, baud 9600
 */
void UART1_Init(void) {
    /* Enable GPIOA and USART1 clocks */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* Configure PA9 (TX) as AF Output Push-Pull, 50MHz */
    GPIOA_CRH &= ~(0xF << 4);         
    GPIOA_CRH |=  (0xB << 4);         

    /* Configure PA10 (RX) as Input Floating */
    GPIOA_CRH &= ~(0xF << 8);         
    GPIOA_CRH |=  (0x4 << 8);         

    /* Baudrate = 9600, PCLK2 = 72MHz */
    USART1_BRR = (468 << 4) | 12;     

    /* Enable USART1, TX, RX */
    USART1_CR1 |= (1 << 13); // UE
    USART1_CR1 |= (1 << 3);  // TE
    USART1_CR1 |= (1 << 2);  // RE
}

/**
 * @brief  Send one char over USART1
 */
void UART1_SendChar(char c) {
    while (!(USART1_SR & (1 << 7))); // TXE=1 ?
    USART1_DR = c;
}

/**
 * @brief  Send string over USART1
 */
void UART1_SendString(const char *str) {
    while (*str) {
        UART1_SendChar(*str++);
    }
}

/**
 * @brief  Retarget printf to USART1
 */
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            UART1_SendChar('\r');
        }
        UART1_SendChar(ptr[i]);
    }
    return len;
}

/**
 * @brief  Dump 16 registers from task stack (for debug)
 */
static void uart_send_hex32(uint32_t value) {
    UART1_SendString("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        UART1_SendChar(c);
    }
    UART1_SendString("\r\n");
}

void log_stack(uint32_t *sp) {
    UART1_SendString("=== Stack Dump ===\r\n");
    const char *labels[] = {
        " R4:  ", " R5:  ", " R6:  ", " R7:  ",
        " R8:  ", " R9:  ", "R10:  ", "R11:  ",
        " R0:  ", " R1:  ", " R2:  ", " R3:  ",
        "R12:  ", " LR:  ", " PC:  ", "xPSR: "
    };
    for (int i = 0; i < 16; i++) {
        UART1_SendString(labels[i]);
        uart_send_hex32(sp[i]);
    }
    UART1_SendString("==================\r\n");
}

void print_str(const char *s) {
    UART1_SendString(s);
}

void print_dec(int value) {
    char buf[12];   // đủ cho -2,147,483,648
    int i = 0;
    if (value < 0) {
        UART1_SendChar('-');
        value = -value;
    }
    do {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    while (i--) {
        UART1_SendChar(buf[i]);
    }
}

void print_hex(uint32_t value) {
    UART1_SendString("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        UART1_SendChar(c);
    }
}

