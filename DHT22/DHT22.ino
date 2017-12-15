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

#include <ESP8266WiFi.h>
#include <PubSubClient.h>	/* https://pubsubclient.knolleary.net/api.html */
#include "Maison.h"	// Paramètres de mon réseau (WIFI_*, MQTT_*, ...)
#define MQTT_CLIENT "TestDHT"
#define MQTT_Topic "TestDHT"
WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

void setup(){
	Serial.begin(115200);	// débuging
	delay(1000);	// Le temps que ça se stabilise

	Serial.println("Connexion WiFi");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while(WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
	Serial.print("ok : adresse ");
	Serial.println(WiFi.localIP());

	Serial.println("Connexion MQTT");
	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);
	while(!clientMQTT.connected()){
		if(clientMQTT.connect(MQTT_CLIENT)){
			Serial.println("connecté");
			break;
		} else {
			Serial.print("Echec, rc:");
			Serial.println(clientMQTT.state());
			delay(1000);	// Test dans 1 seconde
		}
	}
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

	String dt( temperature );
	dt += ',';
	dt += t;
	dt += ',';
	dt += humidite;
	Serial.println( dt.c_str() );
	clientMQTT.publish( MQTT_Topic, dt.c_str() );

	ESP.deepSleep(300 * 1e6);
}
