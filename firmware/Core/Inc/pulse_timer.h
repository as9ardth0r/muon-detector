/**
 * pulse_timer.h — horodatage des impulsions sur 2 voies (sortie des
 * comparateurs des deux détecteurs) via TIM2 en capture d'entrée.
 * TIM2 est le seul timer 32 bits du STM32F405 accessible en LQFP64 —
 * choix déjà fait pour permettre un comptage continu sur plusieurs
 * heures sans débordement fréquent (le nanodrone et le ballon-sonde
 * utilisaient des timers 16 bits pour du PWM, pas pour du comptage
 * long — besoin différent ici).
 *
 * PA0 = TIM2_CH1 (voie A), PA1 = TIM2_CH2 (voie B), AF1. Résolution
 * 1 µs (PSC réglé pour une horloge timer à 1 MHz depuis les 84 MHz
 * d'APB1×2 — voir clock.c).
 */
#ifndef PULSE_TIMER_H
#define PULSE_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#define PULSE_BUF_SIZE 256

void pulse_timer_init(void);

/* Retire jusqu'à `max_out` horodatages accumulés (µs, depuis le
 * démarrage du timer) de la voie A/B dans `out`, les retire du buffer
 * interne. Retourne le nombre effectivement retiré. Non bloquant. */
uint32_t pulse_timer_drain_a(uint32_t *out, uint32_t max_out);
uint32_t pulse_timer_drain_b(uint32_t *out, uint32_t max_out);

#endif /* PULSE_TIMER_H */
