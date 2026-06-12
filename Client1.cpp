#ifdef __INTELLISENSE__
#define _Float32  float
#define _Float64  double
#define _Float32x double
#define _Float64x long double
#define _Float128 long double
#endif

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "LM75A.h"
#include "bme280.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include <csignal>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

//              INA219
#define INA219_ADDR       0x40
#define REG_CONFIG        0x00
#define REG_SHUNT_VOLTAGE 0x01
#define REG_BUSVOLTAGE    0x02
#define REG_POWER         0x03
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

//              HC-SR04 
#define TRIG 17   // GPIO17
#define ECHO 27   // GPIO27
float mesurerDistance() {
    digitalWrite(TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    // Attendre le debut de l'echo (timeout 50 ms)
    auto t0 = std::chrono::high_resolution_clock::now();
    while (digitalRead(ECHO) == LOW) {
        auto dt = std::chrono::high_resolution_clock::now() - t0;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() > 50)
            return -1.0f;   // pas d'echo : on abandonne
    }

    auto start = std::chrono::high_resolution_clock::now();
    // Attendre la fin de l'echo (timeout 50 ms)
    while (digitalRead(ECHO) == HIGH) {
        auto dt = std::chrono::high_resolution_clock::now() - start;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() > 50)
            return -1.0f;   // echo trop long : on abandonne
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return (d.count() * 0.034f) / 2.0f;
}

//                      Envoi HTTP 
std::string envoyer_au_serveur(const std::string& host,
    const std::string& port,
    const std::string& body_content)
{
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);
    auto const results = resolver.resolve(host, port);
    stream.connect(results);

    http::request<http::string_body> req{ http::verb::post, "/", 11 };
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/x-www-form-urlencoded");
    req.body() = body_content;
    req.prepare_payload();

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    return res.body();
}

// Convertit un float en chaine decimales (0.00 par defaut)
std::string f2s(float value, int precision = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}
int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host_serveur> <port>" << std::endl;
        std::cerr << "Exemple: " << argv[0] << " 172.18.10.35 8080" << std::endl;
        return 1;
    }
    std::string host = argv[1];
    std::string port = argv[2];

    //                     Initialisations UNE SEULE FOIS (hors de la boucle) 
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "Erreur init WiringPi (GPIO)" << std::endl;
        return 1;
    }
    //                     init capteur temp
    
    //                     init LM75A
    LM75A capteur;
    //                     init BME280
    BME280 bme;
    bme.init_capteur();
    bme280_calib_data cal;
    bme.readCalibrationData(&cal);

    //                     init HC-SR04
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    //                      init INA219
    fd = wiringPiI2CSetup(INA219_ADDR);
    if (fd < 0) { fprintf(stderr, "Erreur I2C (INA219)\n"); return 1;}

    std::cout << "Demarrage de l'acquisition." << std::endl;

    //                       Boucle continue
    while (true) {
        //                   Capteur LM75A
        float temperature = capteur.getTemperatureInDegrees();

        //                   Capteur BME280
        bme280_raw_data raw;
        bme.getRawData(&raw);
        int32_t t_fine = bme.getTemperatureCalibration(&cal, raw.temperature);
        float temp2 = bme.compensateTemperature(t_fine);

        //                   Calcule de la redondance
        float redondance = std::fabs(temp2 - temperature);

        //                   Capteur HC-SR04 
        float distance = mesurerDistance();

        //                   Capteur INA219
        int16_t raw_bus = readReg(REG_BUSVOLTAGE);
        int16_t raw_shunt = readReg(REG_SHUNT_VOLTAGE);
        int16_t raw_cur = readReg(REG_CURRENT);

        //                   acquisition INA219
        float voltage = ((raw_bus >> 3) * 4) / 1000.0f;      // mV -> V
        float current = raw_cur * CURRENT_LSB * 1000.0f;      // A -> mA
        float power = voltage * current;                     // mW

        std::string body =
            "temperature1=" + f2s(temperature) + "C"
            + "&temperature2=" + f2s(temp2) + "C"
            + "&redondance=" + f2s(redondance) + "C"
            + "&distance=" + f2s(distance) + "cm"
            + "&tension=" + f2s(voltage) + "V"
            + "&courant=" + f2s(current) + "mA"
            + "&puissance=" + f2s(power) + "mW";

        try {
            std::string reponse = envoyer_au_serveur(host, port, body);
            std::cout << "Envoye  : " << body << std::endl;
            std::cout << "Reponse : " << reponse << std::endl;
        }
        catch (std::exception const& e) {
            std::cerr << "Envoi echoue: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));  // frequence d'envoi
    }
    return 0;
}
