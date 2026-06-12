#include <wiringPiI2C.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define INA219_ADDR       0x40
#define REG_CONFIG        0x00
#define REG_SHUNT_VOLTAGE 0x01
#define REG_BUS_VOLTAGE   0x02
#define REG_CURRENT       0x04
#define REG_CALIBRATION   0x05

// Calibration pour shunt 0.1 ohm, max 3.2A
#define CALIBRATION_VALUE 4096
#define CURRENT_LSB       0.0000976f  // A par bit

#define BATTERY_MAH       4000.0f
#define BATTERY_VOLTAGE   3.7f

int fd;

int16_t readReg(int reg) {
    int val = wiringPiI2CReadReg16(fd, reg);
    return (int16_t)((val << 8) | ((val >> 8) & 0xFF));
}

int main() {
    fd = wiringPiI2CSetup(INA219_ADDR);
    if (fd < 0) { fprintf(stderr, "Erreur I2C\n"); return 1; }

    // Config: 32V bus, gain /8, 12bit, continu
    wiringPiI2CWriteReg16(fd, REG_CONFIG,
        (uint16_t)(0x219F << 8) | (0x219F >> 8));
    wiringPiI2CWriteReg16(fd, REG_CALIBRATION,
        (uint16_t)(CALIBRATION_VALUE << 8) | (CALIBRATION_VALUE >> 8));

    printf("%-12s %-12s %-12s %-10s\n",
        "Tension(V)", "Courant(mA)", "Puissance(W)", "Charge(%)");
    printf("----------------------------------------------------\n");

   while(true) {
        int16_t raw_bus = readReg(REG_BUS_VOLTAGE);
        int16_t raw_shunt = readReg(REG_SHUNT_VOLTAGE);
        int16_t raw_cur = readReg(REG_CURRENT);

        float voltage = ((raw_bus >> 3) * 4) / 1000.0f;      // mV -> V
        float current = raw_cur * CURRENT_LSB * 1000.0f;      // A -> mA
        float power = voltage * current;                     // mW

        // Charge estimée (linéaire 3.0V=0% → 4.2V=100%)
        float charge = (voltage - 3.0f) / (4.2f - 3.0f) * 100.0f;
        if (charge < 0)   charge = 0;
        if (charge > 100) charge = 100;

        printf("%-12.3f %-12.1f %-12.1f %-10.1f\n",
            voltage, current, power, charge);

        sleep(1);
    }
    return 0;
}