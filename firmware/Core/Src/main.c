#include "stm32f4xx.h"
#include "clock.h"
#include "i2c_bus.h"
#include "pulse_timer.h"
#include "coincidence.h"
#include "bme280.h"
#include "uart_report.h"
#include <stdio.h>

#define REPORT_PERIOD_APPROX_LOOPS 2000000U
#define COINCIDENCE_WINDOW_US 100U /* voir docs/hardware.md pour la justification */
#define MAX_EVENTS_PER_DRAIN 64U

static void delay_loops(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) { }
}

int main(void) {
    clock_init_168mhz_hse8mhz();
    i2c1_init();
    uart_report_init(9600);
    pulse_timer_init();

    bool bme_ok = bme280_init();

    uint32_t total_a = 0, total_b = 0, total_coincidences = 0;
    uint32_t cycle = 0;

    while (1) {
        uint32_t times_a[MAX_EVENTS_PER_DRAIN];
        uint32_t times_b[MAX_EVENTS_PER_DRAIN];
        uint32_t n_a = pulse_timer_drain_a(times_a, MAX_EVENTS_PER_DRAIN);
        uint32_t n_b = pulse_timer_drain_b(times_b, MAX_EVENTS_PER_DRAIN);

        total_a += n_a;
        total_b += n_b;

        if (n_a > 0 && n_b > 0) {
            uint32_t matched_a[MAX_EVENTS_PER_DRAIN];
            uint32_t matched_b[MAX_EVENTS_PER_DRAIN];
            uint32_t n_coinc = mc_find_coincidences(
                times_a, n_a, times_b, n_b, COINCIDENCE_WINDOW_US,
                matched_a, matched_b, MAX_EVENTS_PER_DRAIN);
            total_coincidences += n_coinc;
        }

        cycle++;
        if (cycle % 50 == 0) { /* rapport périodique, pas à chaque cycle */
            bme280_sample_t sample = {0};
            bool sample_ok = bme_ok && bme280_read(&sample);

            char line[128];
            int len = snprintf(line, sizeof(line),
                "A=%lu B=%lu COINC=%lu P=%.1fhPa\r\n",
                (unsigned long)total_a, (unsigned long)total_b,
                (unsigned long)total_coincidences,
                sample_ok ? sample.pressure_hpa : 0.0);
            if (len > 0) {
                uart_report_send(line, (size_t)len);
            }
        }

        delay_loops(REPORT_PERIOD_APPROX_LOOPS);
    }
}
