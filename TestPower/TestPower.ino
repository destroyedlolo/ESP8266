/* Test de consomation
 *  
 *  Envoie toutes les DELAI secondes la tension d'alimentation par MQTT.
 *  Le but est de tester les différentes méthodes d'économies d'energie.
 *  
 *  Ecoutez le résultat par 
 *  	mosquitto_sub -v -h bpi.chez.moi -t 'TestESP/#'
 *  
 *  Par "Destroyedlolo", mis dans le Domaine Publique.
 *  
 *  It's in french as linked with tutorials on my website
 *  http://destroyedlolo.info/ESP/
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

	/****
	 * Parametrages
	 ****/

		/* Quel mode allons-nous tester ? */
#define MODE_AUCUN	0				// Fonctionnement sans optimisations
#define MODE_LIGHT_SLEEP 1	// Light Sleep
#define MODE_DEEP_SLEEP	 2	// Deep sleep (les broches RST et D0 doivent être connectées)

#define MODE MODE_AUCUN
#define LED		// Allume la LED lors de la recherche du réseau et du MQTT

#define MQTT_CLIENT "TestESP"
String MQTT_Topic("TestESP/");	// Racine des messages envoyés
#define DELAI 60	// Delai en secondes entre chaque boucle

	/****
	 * Paramêtres maison
	 ****/
#include "Maison.h"			// Paramètres de mon réseau (WIFI_*, MQTT_*, ...)
#define LED_BUILTIN 2		// La LED est sur le GPIO2 sur mon NodeMCU v0.1

	/* IP static pour éviter d'attendre le DHCP */
IPAddress adr_ip(192, 168, 0, 17);
IPAddress adr_gateway(192, 168, 0, 10);
IPAddress adr_dns(192, 168, 0, 3);

	/****
	 * Réseau
	 ****/

WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

unsigned long dwifi;	// Combien de temps à duré l'initialisation du Wifi

unsigned long connexion_WiFi(){
	unsigned long debut, fin;
	WiFi.persistent( false );	// Supprime la sauvegarde des info WiFi en Flash

#	ifdef LED
		digitalWrite(LED_BUILTIN, LOW);
#	endif

	Serial.println("Connexion WiFi");
	debut = millis();
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while(WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
	fin = millis();

	Serial.print("ok : adresse ");
	Serial.println(WiFi.localIP());

#	ifdef LED
		digitalWrite(LED_BUILTIN, HIGH);
#	endif
	return fin - debut;
}

void Connexion_MQTT(){
#	ifdef LED
		digitalWrite(LED_BUILTIN, LOW);
#	endif

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

#	ifdef LED
		digitalWrite(LED_BUILTIN, HIGH);
#	endif
}


	/***
	 * C'est parti
	 ***/

ADC_MODE(ADC_VCC);	// Nécessaire pour avoir la tension d'alimentation

void setup(){
#	ifdef LED
		pinMode(LED_BUILTIN, OUTPUT);
#	endif

		/* Console */
	Serial.begin(115200);
	delay(10);	// Le temps que ça se stabilise

		/* Configuration réseau */
	WiFi.config(adr_ip, adr_gateway, adr_dns);
	WiFi.mode(WIFI_STA);
	dwifi = connexion_WiFi();
	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);
}

void loop() {
	int debut, fin;
	if(!clientMQTT.connected()){
		debut = millis();
		Connexion_MQTT();
		fin = millis();

		Serial.print("La reconnexion a durée ");
		Serial.print( fin - debut );
		Serial.println(" milli-secondes");
		clientMQTT.publish( (MQTT_Topic + "reconnect").c_str(), String( fin - debut ).c_str() );
		if(dwifi){
			clientMQTT.publish( (MQTT_Topic + "Wifi").c_str(), String( dwifi ).c_str() );
			dwifi = 0;
		}
	}

	Serial.print("Alimentation : ");
	Serial.println(ESP.getVcc());
	debut = millis();
	clientMQTT.publish( (MQTT_Topic + "Alim").c_str(), String( ESP.getVcc() ).c_str() );
	fin = millis();
	Serial.print("Durée de l'envoi : ");
	Serial.print( fin - debut );
	Serial.println(" ms");
	clientMQTT.publish( (MQTT_Topic + "Alim/duree").c_str(), String( fin - debut ).c_str() );

	delay( DELAI * 1e3 );	
}

