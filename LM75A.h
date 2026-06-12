#ifndef LM75A_h
#define LM75A_h


#include <cstdint>

class LM75A
{
public:
    // Constructeur : A0/A1/A2 = configuration de l'adresse I2C
    LM75A(bool A0_value = false,
        bool A1_value = false,
        bool A2_value = false);

    // Lecture température
    float getTemperatureInDegrees() const;
    float getTemperatureInFahrenheit() const;


    // Conversions
    static float fahrenheitToDegrees(float temperature_in_fahrenheit);
    static float degreesToFahrenheit(float temperature_in_degrees);

private:
    int _i2c_device_address;  // Adresse I2C calculée
    int _fd;                  // File descriptor wiringPiI2C

    //static constexpr float INVALID_LM75A_TEMPERATURE = -1000.0f;
};

#endif //LM75A_h
