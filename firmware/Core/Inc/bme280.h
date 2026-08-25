/**
 * bme280.h — pilote complet BME280 (pression, pour la correction
 * barométrique — voir sim/muon_sim/barometric.py). Même formule de
 * compensation que le projet ballon-sonde (registres/formule Bosch
 * documentés publiquement), adaptée ici à l'API i2c_bus.h de ce
 * dépôt (I2Cv1 du STM32F405, différente de l'I2Cv2 du STM32L0 utilisé
 * côté ballon).
 */
#ifndef BME280_H
#define BME280_H

#include <stdbool.h>

#define BME280_I2C_ADDR 0x76U

typedef struct {
    double temperature_c;
    double pressure_hpa;
    double humidity_pct;
} bme280_sample_t;

bool bme280_init(void);
bool bme280_read(bme280_sample_t *out);

#endif /* BME280_H */
