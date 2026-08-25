#include "bme280.h"
#include "i2c_bus.h"

#define REG_CHIP_ID   0xD0U
#define REG_RESET     0xE0U
#define REG_CTRL_HUM  0xF2U
#define REG_CTRL_MEAS 0xF4U
#define REG_CONFIG    0xF5U
#define REG_PRESS_MSB 0xF7U
#define REG_CALIB_00  0x88U
#define REG_CALIB_H1  0xA1U
#define REG_CALIB_26  0xE1U

#define CHIP_ID_EXPECTED 0x60U

typedef struct {
    uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
    uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3; int16_t dig_P4; int16_t dig_P5;
    int16_t dig_P6; int16_t dig_P7; int16_t dig_P8; int16_t dig_P9;
    uint8_t dig_H1; int16_t dig_H2; uint8_t dig_H3; int16_t dig_H4; int16_t dig_H5; int8_t dig_H6;
} calib_t;

static calib_t cal;

static bool read_reg(uint8_t reg, uint8_t *data, uint32_t len) {
    return i2c1_write_read(BME280_I2C_ADDR, &reg, 1, data, len) == I2C_OK;
}

static bool write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c1_write(BME280_I2C_ADDR, buf, sizeof(buf)) == I2C_OK;
}

static int16_t s16(const uint8_t *p) { return (int16_t)((uint16_t)p[1] << 8 | p[0]); }
static uint16_t u16(const uint8_t *p) { return (uint16_t)((uint16_t)p[1] << 8 | p[0]); }

static bool read_calibration(void) {
    uint8_t buf1[26];
    if (!read_reg(REG_CALIB_00, buf1, sizeof(buf1))) return false;

    cal.dig_T1 = u16(&buf1[0]);  cal.dig_T2 = s16(&buf1[2]);  cal.dig_T3 = s16(&buf1[4]);
    cal.dig_P1 = u16(&buf1[6]);  cal.dig_P2 = s16(&buf1[8]);  cal.dig_P3 = s16(&buf1[10]);
    cal.dig_P4 = s16(&buf1[12]); cal.dig_P5 = s16(&buf1[14]); cal.dig_P6 = s16(&buf1[16]);
    cal.dig_P7 = s16(&buf1[18]); cal.dig_P8 = s16(&buf1[20]); cal.dig_P9 = s16(&buf1[22]);

    uint8_t h1;
    if (!read_reg(REG_CALIB_H1, &h1, 1)) return false;
    cal.dig_H1 = h1;

    uint8_t buf2[7];
    if (!read_reg(REG_CALIB_26, buf2, sizeof(buf2))) return false;
    cal.dig_H2 = s16(&buf2[0]);
    cal.dig_H3 = buf2[2];
    cal.dig_H4 = (int16_t)(((int16_t)(int8_t)buf2[3] << 4) | (buf2[4] & 0x0FU));
    cal.dig_H5 = (int16_t)(((int16_t)(int8_t)buf2[5] << 4) | (buf2[4] >> 4));
    cal.dig_H6 = (int8_t)buf2[6];
    return true;
}

bool bme280_init(void) {
    uint8_t chip_id = 0;
    if (!read_reg(REG_CHIP_ID, &chip_id, 1)) return false;
    if (chip_id != CHIP_ID_EXPECTED) return false;
    if (!write_reg(REG_RESET, 0xB6U)) return false;
    if (!read_calibration()) return false;
    if (!write_reg(REG_CTRL_HUM, 0x01U)) return false;
    if (!write_reg(REG_CTRL_MEAS, 0x27U)) return false;
    if (!write_reg(REG_CONFIG, 0x00U)) return false;
    return true;
}

bool bme280_read(bme280_sample_t *out) {
    uint8_t raw[8];
    if (!read_reg(REG_PRESS_MSB, raw, sizeof(raw))) return false;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

    double var1 = ((double)adc_T / 16384.0 - (double)cal.dig_T1 / 1024.0) * (double)cal.dig_T2;
    double var2 = (((double)adc_T / 131072.0 - (double)cal.dig_T1 / 8192.0)
                  * ((double)adc_T / 131072.0 - (double)cal.dig_T1 / 8192.0)) * (double)cal.dig_T3;
    double t_fine = var1 + var2;
    out->temperature_c = t_fine / 5120.0;

    var1 = t_fine / 2.0 - 64000.0;
    var2 = var1 * var1 * (double)cal.dig_P6 / 32768.0;
    var2 = var2 + var1 * (double)cal.dig_P5 * 2.0;
    var2 = var2 / 4.0 + (double)cal.dig_P4 * 65536.0;
    var1 = ((double)cal.dig_P3 * var1 * var1 / 524288.0 + (double)cal.dig_P2 * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * (double)cal.dig_P1;
    if (var1 == 0.0) {
        out->pressure_hpa = 0.0;
    } else {
        double p = 1048576.0 - (double)adc_P;
        p = (p - var2 / 4096.0) * 6250.0 / var1;
        var1 = (double)cal.dig_P9 * p * p / 2147483648.0;
        var2 = p * (double)cal.dig_P8 / 32768.0;
        p = p + (var1 + var2 + (double)cal.dig_P7) / 16.0;
        out->pressure_hpa = p / 100.0;
    }

    double var_h = t_fine - 76800.0;
    var_h = ((double)adc_H - ((double)cal.dig_H4 * 64.0 + (double)cal.dig_H5 / 16384.0 * var_h))
          * ((double)cal.dig_H2 / 65536.0 * (1.0 + (double)cal.dig_H6 / 67108864.0 * var_h
             * (1.0 + (double)cal.dig_H3 / 67108864.0 * var_h)));
    var_h = var_h * (1.0 - (double)cal.dig_H1 * var_h / 524288.0);
    if (var_h > 100.0) var_h = 100.0;
    if (var_h < 0.0) var_h = 0.0;
    out->humidity_pct = var_h;

    return true;
}
