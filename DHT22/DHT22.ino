/* Test d'acquisition de température & d'humidité par DHT22
 *
 *	13/12/2017 - Première version
 *
 *  Par Destroyedlolo, mis dans le Domaine Publique.
 *	Les explications sur le code : http://destroyedlolo.info/ESP/DHT22/
 */
#include <SimpleDHT.h>	// https://github.com/winlinvip/SimpleDHT

SimpleDHT22 DHT;
int pinDHT = 5;	// Broche sur laquelle est connecté le DHT22

void setup(){
	Serial.begin(115200);	// débuging
	delay(10);	// Le temps que ça se stabilise
}

void loop(){
	float temperature = 0;
	float humidite = 0;
	int err;

	if((err = DHT.read2(pinDHT, &temperature, &humidite, NULL)) != SimpleDHTErrSuccess){
		Serial.print("Erreur de lecture, err=");
		Serial.println(err);
	} else {
		Serial.print("Sample OK: ");
		Serial.print(temperature); Serial.print(" *C, ");
		Serial.print(humidite); Serial.println(" RH%");
	}

	delay(2500);
}
