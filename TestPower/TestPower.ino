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

extern "C" {
  #include "user_interface.h"
}

	/****
	 * Parametrages
	 ****/

		/* Quel mode allons-nous tester ? */
#define MODE_AUCUN	0				// Fonctionnement sans optimisations
#define MODE_LIGHT_SLEEP 1	// Light Sleep
#define MODE_DEEP_SLEEP	 2	// Deep sleep (les broches RST et D0 doivent être connectées)

#define MODE MODE_DEEP_SLEEP

#define SAVEWIFI	// essaie d'utiliser la sauvegarde faite dans la Flash de l'ESP.
#define LED				// Allume la LED lors de la recherche du réseau et du MQTT
#define AUTORECONNECT	// N'appelle pas explicitement WiFi.begin() : utilise l'auto-reconnection

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

#	ifdef LED
		digitalWrite(LED_BUILTIN, LOW);
#	endif

	Serial.println("Connexion WiFi");
	debut = millis();

#ifdef SAVEWIFI
		/* Les infos de la connexion sont écrites à chaque fois dans la Flash :
		 * on essaie dans un premier temps de les réutiliser avant d'en imposer des nouvelles. 
		 */
#	ifndef AUTORECONNECT
	WiFi.begin();
#	endif

	for( int i=0; i< 120; i++ ){	// On essaie de se connecter pendant 1 minute
		if(WiFi.status() == WL_CONNECTED)
			break;
		delay(500);
		Serial.print("-");
	}

	if(WiFi.status() != WL_CONNECTED){	// La connexion a échoué, on force les settings
		WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
		while(WiFi.status() != WL_CONNECTED){
			delay(500);
			Serial.print(".");
		}
	}
#else
	WiFi.persistent( false );	// Supprime la sauvegarde des info WiFi en Flash

	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while(WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
#endif

	fin = millis();

	Serial.println("ok");
	WiFi.printDiag( Serial );
	Serial.print("AutoConnect : ");
	Serial.println(WiFi.getAutoConnect() ? "Oui" : "Non");

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
#	if MODE == MODE_LIGHT_SLEEP
		wifi_set_sleep_type(LIGHT_SLEEP_T);
#	else
		wifi_set_sleep_type(MODEM_SLEEP_T);
#	endif
//	WiFi.mode(WIFI_STA);
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
		if(dwifi){
			clientMQTT.publish( (MQTT_Topic + "Wifi").c_str(), String( dwifi ).c_str() );
			dwifi = 0;
		}
		clientMQTT.publish( (MQTT_Topic + "reconnect").c_str(), String( fin - debut ).c_str() );
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

		/* En dessous d'une 20e de secondes, cette ligne permet
		 *  de conserver la connexion avec le broker.
		 *  Au delà, delà, la connexion est systématiquement réinitialisée
		 * Permet aussi de prendre en charge les messages entrant.
		 */
	clientMQTT.loop();

#if MODE == MODE_DEEP_SLEEP
	ESP.deepSleep(DELAI * 1e6);	// Attention, l'unité est la µs
#else	// Autres modes basés uniquement sur delay()
	delay( DELAI * 1e3 );
#endif
}

