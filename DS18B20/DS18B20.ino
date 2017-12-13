/* Test d'acquisition de température par 1-wire
 *
 *	25/10/2017 - Première version
 *	05/11/2017 - Ménage dans le code, passe à OWBus
 *
 *  Par Destroyedlolo, mis dans le Domaine Publique.
 *	Les explications sur le code : http://destroyedlolo.info/ESP/Temperature/
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>	/* https://pubsubclient.knolleary.net/api.html */

#include <OWBus.h>
#include <OWBus/DS18B20.h>
#include <OWBus/DS28EA00.h>

extern "C" {
  #include "user_interface.h"
}

ADC_MODE(ADC_VCC);

/****
 * WiFi
 ****/
#include "Maison.h"	// Paramètres de mon réseau (WIFI_*, MQTT_*, ...)
#define LED_BUILTIN 2

void connexion_WiFi(){
	digitalWrite(LED_BUILTIN, LOW);
	Serial.println("Connexion WiFi");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while(WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}

	Serial.print("ok : adresse ");
	Serial.println(WiFi.localIP());
	digitalWrite(LED_BUILTIN, HIGH);
}

/*****
 * MQTT
 */
#define MQTT_CLIENT "TestTemp"
String MQTT_Topic("TestTemp/");

#if 0
	/* Pour accéléré la connexion, on peut se passer du DNS */
IPAddress adr_ip(192, 168, 0, 17);		// Duron (décommissionné)
IPAddress adr_gateway(192, 168, 0, 10);
IPAddress adr_dns(192, 168, 0, 3);
#endif

WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

void Connexion_MQTT(){
	digitalWrite(LED_BUILTIN, LOW);
	Serial.println("Connexion MQTT");
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
	digitalWrite(LED_BUILTIN, HIGH);
}

/******
 * 1-wire
 ******/
#define ONE_WIRE_BUS D1	// Où est connecté le bus
OneWire oneWire(ONE_WIRE_BUS);

	/* Gestion individualisée des sondes */
int nbr_sondes;	// Combien avons nous trouvé de sondes ?
DS18B20 **sondes;	// pour stocker les sondes
OWBus bus(&oneWire);

void setup(){
	Serial.begin(115200);	// débuging
	pinMode(LED_BUILTIN, OUTPUT);	// La LED est allumée pendant la recherche de WiFi et MQTT
	delay(10);	// Le temps que ça se stabilise

	/* Configuration réseau */
#if 0	// sans DNS
	WiFi.config(adr_ip, adr_gateway, adr_dns);
	WiFi.mode(WIFI_STA);	// Par sécurité
#endif
	wifi_set_sleep_type(LIGHT_SLEEP_T);	// Le mode le moins consommateur hormis d'utiliser le "deep sleep".
	connexion_WiFi();
	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);

	/* 1-wire */
	Serial.print("Bus 1-wire utilisé :");
	Serial.println(ONE_WIRE_BUS);

	Serial.print("Nombre de sondes :");
	Serial.println(nbr_sondes = bus.getDeviceCount());

	if(!(sondes = (DS18B20 **)malloc( sizeof(DS18B20 *) * nbr_sondes))){
		Serial.println("Impossible d'allouer les sondes.\nSTOP !");
		ESP.deepSleep(0);	// On s'arrete là
	}
	Serial.println("Le stockage des sondes est alloué");

	int duree = millis();
	OWBus::Address addr;
	bus.search_reset();
	int i = 0;
	while( bus.search_next( addr ) ){
		Serial.print( addr.toString().c_str() );
		Serial.print(" : ");
		if(!addr.isValid( &oneWire))
			Serial.println("Invalid address");
		else {
			if(i > nbr_sondes){
				Serial.println("Hum, plus le même nombre de sondes.\nStop !");
				ESP.deepSleep(0);	// On s'arrete là
			}
			switch(addr.getFamillyCode()){
			case DS18B20::FAMILLY_CODE:
				sondes[i] = new DS18B20( bus, addr );
				break;
			case DS28EA00::FAMILLY_CODE:
				sondes[i] = new DS28EA00( bus, addr );
				break;
			default:
				Serial.print( addr.getFamilly() );
				Serial.println(" n'est pas prise en compte");
				continue;	// on passe à la suivante
			}
			Serial.println( sondes[i]->getFamilly() );
			sondes[i]->setResolution( i ? 12:9 );

			i++;
		}
		yield();
	}
	nbr_sondes = i;

	Serial.print("La lecture des sondes à durée ");
	Serial.print( millis() - duree );
	Serial.println(" milli-secondes");
}

void loop(){
	int duree;
	if(!clientMQTT.connected()){
		duree = millis();
		Connexion_MQTT();
		int fin = millis();

		Serial.print("La reconnexion a durée ");
		Serial.print( fin - duree );
		Serial.println(" milli-secondes");
		clientMQTT.publish( (MQTT_Topic + "reconnect").c_str(), String( fin - duree ).c_str() );
	}

	Serial.print("Alimentation : ");
	Serial.println(ESP.getVcc());
	clientMQTT.publish( (MQTT_Topic + "Alim").c_str(), String( ESP.getVcc() ).c_str() );
	
	duree = millis();
	bus.launchTemperatureAcquisition();	// broadcaste une demande d'acquisition de température
	delay(750);	// Laissons le temps à la conversion de se faire

	Serial.print("La demande d'acquisition a durée ");
	Serial.print( millis() - duree );
	Serial.println(" milli-secondes");

	duree = millis();	// Ainsi, nous tentons de mesurer aussi les latences dues à la conversion
	for(int i=0; i<nbr_sondes; i++){
		float temp;

		Serial.print("Température pour ");
		Serial.print( sondes[i]->getAddress().toString() );
		Serial.print(" : ");
		Serial.print( temp = sondes[i]->readLastTemperature() );	
		Serial.print("°C (");
		Serial.print( sondes[i]->getResolution() );
		Serial.print(" bits) en ");
		Serial.print( millis() - duree );
		Serial.println(" milli-secondes");

		clientMQTT.publish( (MQTT_Topic + sondes[i]->getAddress().toString()).c_str(), String( temp ).c_str() );
		yield();
		duree = millis();
	}

	delay(300e3);	// Prochaine mesure dans 5 minutes
}

