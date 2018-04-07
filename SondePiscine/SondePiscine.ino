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

#if 0		
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

#define MQTT_CLIENT "SondePiscine"
String MQTT_Topic("SondePiscine");	// Topic's root
String MQTT_Output = MQTT_Topic + "Message";


	/******
	* Fin des configurations
	*******/
#include <ESP8266WiFi.h>
#include <Maison.h>		// My very own environment (WIFI_*, MQTT_*, ...)

extern "C" {
  #include "user_interface.h"
}

#include <PubSubClient.h>
WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

#include "Contexte.h"

void setup(){
#ifdef SERIAL_ENABLED
	Serial.begin(115200);
	delay(100);
#else
	pinMode(LED_BUILTIN, OUTPUT);
#endif
}

void loop(){
}
