#include "bme280.h"
#include <iostream>
#include <chrono>
#include <thread>
using namespace std;

int main() {

	while (true) {
		BME280 Capteur;
		Capteur.init_capteur();
		bme280_raw_data raw;
		bme280_calib_data cal;
		int32_t t_fine;
		Capteur.readCalibrationData(&cal);
		Capteur.getRawData(&raw);
		t_fine = Capteur.getTemperatureCalibration(&cal, raw.temperature);
		cout << "mesure temperature\n" << Capteur.compensateTemperature(t_fine) << endl;
		this_thread::sleep_for(chrono::milliseconds(1000));
	}

	return 0;
};