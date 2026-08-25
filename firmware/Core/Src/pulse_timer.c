#include "pulse_timer.h"
#include "stm32f4xx.h"

static volatile uint32_t buf_a[PULSE_BUF_SIZE];
static volatile uint32_t head_a = 0, tail_a = 0;
static volatile uint32_t buf_b[PULSE_BUF_SIZE];
static volatile uint32_t head_b = 0, tail_b = 0;

static void push(volatile uint32_t *buf, volatile uint32_t *head, uint32_t tail, uint32_t value) {
    uint32_t next = (*head + 1) % PULSE_BUF_SIZE;
    if (next == tail) {
        return; /* buffer plein : on perd l'événement plutôt que d'écraser
                  * un horodatage pas encore lu — signalé comme limite
                  * connue dans le README, pas silencieusement "résolu" */
    }
    buf[*head] = value;
    *head = next;
}

void pulse_timer_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA0=TIM2_CH1, PA1=TIM2_CH2, AF1 */
    GPIOA->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1);
    GPIOA->MODER |= (2UL << GPIO_MODER_MODER0_Pos) | (2UL << GPIO_MODER_MODER1_Pos);
    GPIOA->AFR[0] |= (1UL << GPIO_AFRL_AFSEL0_Pos) | (1UL << GPIO_AFRL_AFSEL1_Pos);
    /* pull-down : la sortie du comparateur est active haute sur détection,
     * repos bas — évite un déclenchement flottant si rien n'est branché */
    GPIOA->PUPDR |= (2UL << GPIO_PUPDR_PUPD0_Pos) | (2UL << GPIO_PUPDR_PUPD1_Pos);

    /* APB1 timer clock = 84 MHz (APB1 42MHz x2, RM0090 §7.2).
     * PSC=83 -> 84MHz/84 = 1MHz -> résolution 1µs par tick, cohérent
     * avec coincidence.h (fenêtres en microsecondes). */
    TIM2->PSC = 83;
    TIM2->ARR = 0xFFFFFFFFUL; /* TIM2 est le seul timer 32 bits du F405 */

    /* CC1S=01, CC2S=01 : IC1 <- TI1, IC2 <- TI2 (pas de mapping croisé) */
    TIM2->CCMR1 = (1U << TIM_CCMR1_CC1S_Pos) | (1U << TIM_CCMR1_CC2S_Pos);
    /* front montant sur les deux voies (CCxP=0, comportement par défaut) */
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;

    TIM2->DIER = TIM_DIER_CC1IE | TIM_DIER_CC2IE;
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_CC1IF) {
        uint32_t t = TIM2->CCR1; /* la lecture acquitte CC1IF (RM0090 §18.4.5) */
        push(buf_a, &head_a, tail_a, t);
    }
    if (TIM2->SR & TIM_SR_CC2IF) {
        uint32_t t = TIM2->CCR2;
        push(buf_b, &head_b, tail_b, t);
    }
}

static uint32_t drain(volatile uint32_t *buf, volatile uint32_t *tail, uint32_t head,
                       uint32_t *out, uint32_t max_out) {
    uint32_t n = 0;
    while (*tail != head && n < max_out) {
        out[n++] = buf[*tail];
        *tail = (*tail + 1) % PULSE_BUF_SIZE;
    }
    return n;
}

uint32_t pulse_timer_drain_a(uint32_t *out, uint32_t max_out) {
    return drain(buf_a, &tail_a, head_a, out, max_out);
}

uint32_t pulse_timer_drain_b(uint32_t *out, uint32_t max_out) {
    return drain(buf_b, &tail_b, head_b, out, max_out);
}
