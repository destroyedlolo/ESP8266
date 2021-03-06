/* SondePiscine
 *
 *	Simple sonde alimentée par des cellules photoélectriques 
 *	et qui revoie par MQTT la température de l'eau de la piscine.
 *
 *	provided by "Laurent Faillie"
 *	Licence : "Creative Commons Attribution-NonCommercial 3.0"
 */


	/*******
	* Configurations
	********/

#if 1		
	/* La LED est allumée pendant la recherche du réseau et l'établissement
	 * du MQTT
	 */
#	define LED(x)	{ digitalWrite(LED_BUILTIN, x); }
#else
	/* Message de Debug sur le tx.
	 * Pas de LED vu qu'il est sur le même PIN
	 */
#	define LED(x)	{ }
#	define SERIAL_ENABLED
#endif

#define ONE_WIRE_BUS 5

#define MQTT_CLIENT "SondePiscine"
String MQTT_Topic("SondePiscine/");	// Topic's root
String MQTT_Output = MQTT_Topic + "Message";
String MQTT_WiFi = MQTT_Topic + "WiFi";
String MQTT_MQTT = MQTT_Topic + "MQTT";
String MQTT_VCC = MQTT_Topic + "Vcc";
String MQTT_TempInterne = MQTT_Topic + "TempInterne";
String MQTT_TempPiscine = MQTT_Topic + "TempPiscine";

#define DUREE_SOMMEIL 300e6	// 5 minutes entre chaque mesure

	/******
	* Fin des configurations
	*******/
#include <ESP8266WiFi.h>
#include <Maison.h>		// Paramettre de mon réseau
#include "Duree.h"

extern "C" {
  #include "user_interface.h"
}

ADC_MODE(ADC_VCC);	// Nécessaire pour avoir la tension d'alimentation

#include <OWBus.h>
#include <OWBus/DS18B20.h>
OneWire oneWire(ONE_WIRE_BUS, true);	// Initialize oneWire library with internal pullup (https://github.com/destroyedlolo/OneWire)
OWBus bus(&oneWire);

#include <PubSubClient.h>
WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

void Connexion_MQTT();

void publish( String &topic, const char *msg, bool reconnect=true ){
	if(!clientMQTT.connected()){
		if( reconnect )
			Connexion_MQTT();
		else
			return;
	}
	clientMQTT.publish( topic.c_str(), msg );
}

void publish( String &topic, const unsigned long val, bool reconnect=true ){
	publish( topic, String(val, DEC).c_str(), reconnect );
}

void Connexion_MQTT(){
	LED(LOW);
#ifdef SERIAL_ENABLED
	Serial.println("Connexion MQTT");
#endif

	Duree dMQTT;
	for( int i=0; i< 240; i++ ){
		if(clientMQTT.connect(MQTT_CLIENT)){
			LED(HIGH);
			dMQTT.Fini();

#ifdef SERIAL_ENABLED
	Serial.print("Duree connexion MQTT :");
	Serial.println( *dMQTT );
#endif
			publish( MQTT_MQTT, *dMQTT, false );
			return;
		} else {
			Serial.print("Echec, rc:");
			Serial.println(clientMQTT.state());
			delay(500);	// Test dans 1 seconde
		}
	}
	LED(HIGH);
#ifdef SERIAL_ENABLED
	Serial.println("Impossible de se connecter au MQTT");
#endif
	ESP.deepSleep(DUREE_SOMMEIL);	// On essaiera plus tard
}

void setup(){
#ifdef SERIAL_ENABLED
	Serial.begin(115200);
	delay(100);
#else
	pinMode(LED_BUILTIN, OUTPUT);
#endif

	Duree dwifi;
#if 1
	WiFi.begin( DOMO_SSID, DOMO_PASSWORD );	// Connexion à mon réseau domotique
#else
	WiFi.begin( WIFI_SSID, WIFI_PASSWORD );	// Connexion à mon réseau domestique
#endif

	for( int i=0; i< 240; i++ ){
		if(WiFi.status() == WL_CONNECTED){
#ifdef SERIAL_ENABLED
				Serial.println("ok");
#endif
				break;
			}
			delay(500);
#ifdef SERIAL_ENABLED
			Serial.print("=");
#endif
		}
		if(WiFi.status() != WL_CONNECTED){
#ifdef SERIAL_ENABLED
			Serial.println("Impossible de se connecter");
#endif
			ESP.deepSleep(DUREE_SOMMEIL);	// On essaiera plus tard
		}

	dwifi.Fini();
#ifdef SERIAL_ENABLED
	Serial.print("Duree connexion Wifi :");
	Serial.println( *dwifi );
#endif

	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);
	publish( MQTT_WiFi, *dwifi );
}

void loop(){
#ifdef SERIAL_ENABLED
	Serial.print("Tension d'alimentation :");
	Serial.println( ESP.getVcc() );
#endif
	publish( MQTT_VCC, ESP.getVcc() );

	DS18B20 SondeTempInterne(bus, 0x2882b25e09000015);
	DS18B20 SondePiscine( bus, 0x28ff8fbf711703c3);

	float temp =  SondeTempInterne.getTemperature( false );
	if( SondeTempInterne.isValidScratchpad() ){	// Verify this probe is here
#ifdef SERIAL_ENABLED
		Serial.print("Temperature Interne :");
		Serial.println( temp );
#endif
		publish( MQTT_TempInterne, String( temp ).c_str() );
	}

	temp = SondePiscine.getTemperature( false );
	if( SondePiscine.isValidScratchpad() ){	// Verify this probe is here
#ifdef SERIAL_ENABLED
		Serial.print("Temperature piscine :");
		Serial.println( temp );
#endif
		publish( MQTT_TempPiscine, String( temp ).c_str() );
	}

	ESP.deepSleep(DUREE_SOMMEIL);	// On essaiera plus tard
}
