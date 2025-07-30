#ifndef __STM32F10X_H // 
#define __STM32F10X_H

#include <stdint.h>

// =====================
// Peripheral Base Addresses
// =====================
#define PERIPH_BASE           ((uint32_t)0x40000000)
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x10000)

#define GPIOA_BASE            (APB2PERIPH_BASE + 0x0800)
#define GPIOB_BASE            (APB2PERIPH_BASE + 0x0C00)
#define GPIOC_BASE            (APB2PERIPH_BASE + 0x1000)
#define GPIOD_BASE            (APB2PERIPH_BASE + 0x1400)
#define GPIOE_BASE            (APB2PERIPH_BASE + 0x1800)
#define AFIO_BASE             (APB2PERIPH_BASE + 0x0000)
#define USART1_BASE           (APB2PERIPH_BASE + 0x3800)
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400)
#define USART3_BASE           (APB1PERIPH_BASE + 0x4800)
#define SPI1_BASE             (APB2PERIPH_BASE + 0x3000)
#define SPI2_BASE             (APB1PERIPH_BASE + 0x3800)
#define I2C1_BASE             (APB1PERIPH_BASE + 0x5400)
#define I2C2_BASE             (APB1PERIPH_BASE + 0x5800)
#define CAN1_BASE             (APB1PERIPH_BASE + 0x6400)
#define CAN2_BASE             (APB1PERIPH_BASE + 0x6800)
#define RCC_BASE              (PERIPH_BASE + 0x21000)

// =====================
// GPIO Register Offsets
// =====================
#define GPIO_CRL_OFFSET       0x00
#define GPIO_CRH_OFFSET       0x04
#define GPIO_IDR_OFFSET       0x08
#define GPIO_ODR_OFFSET       0x0C
#define GPIO_BSRR_OFFSET      0x10
#define GPIO_BRR_OFFSET       0x14
#define GPIO_LCKR_OFFSET      0x18

// =====================
// RCC Register Macros
// =====================
#define RCC_APB2ENR           (*(volatile uint32_t*)(RCC_BASE + 0x18))
#define RCC_APB1ENR           (*(volatile uint32_t*)(RCC_BASE + 0x1C))
#define RCC_APB2ENR_IOPAEN    ((uint32_t)0x00000004)
#define RCC_APB2ENR_IOPBEN    ((uint32_t)0x00000008)
#define RCC_APB2ENR_IOPCEN    ((uint32_t)0x00000010)
#define RCC_APB2ENR_AFIOEN    ((uint32_t)0x00000001)
#define RCC_APB2ENR_USART1EN  ((uint32_t)0x00004000)
#define RCC_APB2ENR_SPI1EN    ((uint32_t)0x00001000)
#define RCC_APB1ENR_USART2EN  ((uint32_t)0x00020000)
#define RCC_APB1ENR_USART3EN  ((uint32_t)0x00040000)
#define RCC_APB1ENR_SPI2EN    ((uint32_t)0x00004000)
#define RCC_APB1ENR_I2C1EN    ((uint32_t)0x00200000)
#define RCC_APB1ENR_I2C2EN    ((uint32_t)0x00400000)
#define RCC_APB1ENR_CAN1EN    ((uint32_t)0x02000000)
#define RCC_APB1ENR_CAN2EN    ((uint32_t)0x04000000)

// =====================
// SCB Register Macros
// =====================
#define SCB_BASE            (0xE000ED00UL)
#define SCB_SHPR3           (*(volatile uint32_t *)(SCB_BASE + 0x20))  // System Handler Priority Register 3
#define PENDSV_PRIO_SHIFT   16
#define PENDSV_PRIO_MASK    (0xFF << PENDSV_PRIO_SHIFT)
#define SCB_ICSR           (*(volatile uint32_t *)(0xE000ED04UL))  // Interrupt Control and State Register
#define SCB_ICSR_PENDSVSET (1 << 28)                               // Set-pending PendSV bit

#endif // __STM32F10X_H
