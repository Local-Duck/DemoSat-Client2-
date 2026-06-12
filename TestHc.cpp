#include <wiringPi.h>
#include <iostream>
#include <chrono>
#include <thread>
using namespace std;

// def des broches
#define TRIG 17
#define ECHO 27

float mesurerDistance() {
    // impilsion toutes les 10 us
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    // attente de l'écho
    while (digitalRead(ECHO) == LOW);

    // Démarrage du chrono
    auto start = chrono::high_resolution_clock::now();

    // attente de la fin de l'écho
    while (digitalRead(ECHO) == HIGH);

    // fin du chrono
    auto end = chrono::high_resolution_clock::now();

    // Calcul de la durée en microsecondes
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double dureeMicros = duration.count();

    // calcul distance : (durée * vitesse_son) / 2
    // vitesse_son = 340 m/s = 0.034 cm/µs
    float distance = (dureeMicros * 0.034) / 2;

    return distance;
}

int main() {
    // init de WiringPi
    if (wiringPiSetupGpio() == -1) {
        cout << "erreur d'initialisation de WiringPi" << endl;
        return 1;
    }
    // broches trig/echo
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    cout << "début de la mesure" << endl;

    // boucle de mesure
    while (true) {
        float distance = mesurerDistance();
        cout << "Distance : " << distance << " cm" << endl;
        delay(60); // delay d'1s entre chaque mesure
    }

    return 0;
}