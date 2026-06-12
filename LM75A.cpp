#include "LM75A.h"
#include <wiringPiI2C.h>
#include <iostream>


LM75A::LM75A(bool A0_value, bool A1_value, bool A2_value) {
    _i2c_device_address = 0x48;
    if (A0_value) _i2c_device_address += 1;
    if (A1_value) _i2c_device_address += 2;
    if (A2_value) _i2c_device_address += 4;

    _fd = wiringPiI2CSetup(_i2c_device_address);


}

// Lecture de la température en Degrés Celsius

float LM75A::getTemperatureInDegrees() const {
    if (_fd == -1) return -1000.0f;

    // Lecture du registre 0x00 (Température) - Renvoie 16 bits
    int raw = wiringPiI2CReadReg16(_fd, 0x00);

    if (raw < 0) {
        return -1000.0f;
    }

    // Changer l'ordre d'envoie des Octets
    // Le LM75 envoie MSB puis LSB. WiringPi inverse l'ordre sur ARM
    // On replace le MSB à gauche et le LSB à droite
    uint16_t swapped = ((raw << 8) & 0xFF00) | ((raw >> 8) & 0x00FF);

    // Le LM75A utilise 11 bits alignés à gauche (MSB)
    // On décale de 5 bits vers la droite pour obtenir la valeur réelle
    // On utilise int16_t pour conserver le signe (températures négatives)
    int16_t value = (int16_t)swapped >> 5;

    // Le pas de résolution est de 0.125°C par unité
    return (float)value * 0.125f;
}

// Lecture de la température en Fahrenheit
 
float LM75A::getTemperatureInFahrenheit() const {
    float tempC = getTemperatureInDegrees();
    if (tempC == -1000.0f) return -1000.0f;
    return degreesToFahrenheit(tempC);
}

// Fonctions de conversion statiques

float LM75A::fahrenheitToDegrees(float f) {
    return (f - 32.0f) * 5.0f / 9.0f;
}

float LM75A::degreesToFahrenheit(float c) {
    return (c * 9.0f / 5.0f) + 32.0f;
}

