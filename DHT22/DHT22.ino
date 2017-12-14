/* Test d'acquisition de température & d'humidité par DHT22
 *
 *	13/12/2017 - Première version
 *	14/12/2017 - Ajoute du code pour étaloner la sonde
 *
 *  Par Destroyedlolo, mis dans le Domaine Publique.
 *	Les explications sur le code : http://destroyedlolo.info/ESP/DHT22/
 */
#include <SimpleDHT.h>	// https://github.com/winlinvip/SimpleDHT

SimpleDHT22 DHT;
int pinDHT = 5;	// Broche sur laquelle est connecté le DHT22

#include <OWBus.h>
#include <OWBus/DS18B20.h>

OneWire oneWire(2);
OWBus bus(&oneWire);


void setup(){
	Serial.begin(115200);	// débuging
	delay(1000);	// Le temps que ça se stabilise
}

void loop(){
	float temperature = 0;
	float humidite = 0;
	int err;

	if((err = DHT.read2(pinDHT, &temperature, &humidite, NULL)) != SimpleDHTErrSuccess){
		Serial.print("\nErreur de lecture, err=");
		Serial.println(err);
	} else {
		Serial.print("\nSample OK: ");
		Serial.print(temperature); Serial.print(" *C, ");
		Serial.print(humidite); Serial.println(" RH%");
	}

	DS18B20 sonde( bus, 0x28A7A073070000AE );
	float t = sonde.getTemperature() - 0.0208544495;
	Serial.println( t );

	ESP.deepSleep(30 * 1e6);
}
