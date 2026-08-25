#include "uart_report.h"
#include "stm32f4xx.h"

void uart_report_init(unsigned int baudrate) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

    /* PB10 = USART3_TX, AF7 */
    GPIOB->MODER &= ~GPIO_MODER_MODER10;
    GPIOB->MODER |= (2UL << GPIO_MODER_MODER10_Pos);
    GPIOB->AFR[1] |= (7UL << GPIO_AFRH_AFSEL10_Pos);

    /* APB1 = 42 MHz. Formule mantisse/fraction (RM0090 §19.3.4), même
     * approche que le pilote I2C de ce dépôt (F405, ancien périphérique
     * USART, différent de celui du STM32L0 côté ballon-sonde). */
    uint32_t usartdiv_x16 = (42000000UL * 2U) / baudrate;
    uint32_t mantissa = usartdiv_x16 / 32U;
    uint32_t fraction = (usartdiv_x16 / 2U) % 16U;
    USART3->BRR = (mantissa << 4) | (fraction & 0xFU);

    USART3->CR1 = USART_CR1_TE;
    USART3->CR1 |= USART_CR1_UE;
}

void uart_report_send(const char *text, size_t len) {
    for (size_t i = 0; i < len; i++) {
        while ((USART3->SR & USART_SR_TXE) == 0) { }
        USART3->DR = (uint8_t)text[i];
    }
}
