/* Test d'acquisition de température par 1-wire
 *  
 *  Par Destroyedlolo, mis dans le Domaine Publique.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>	/* https://pubsubclient.knolleary.net/api.html */

#include <OneWire.h>						// Gestion du bus 1-wire
#include <DallasTemperature.h>	// Sondes ds18b20

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
	WiFi.persistent( false );	// Supprime la sauvegarde des info WiFi en Flash
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

	/* On n'utilise pas DHCP pour que ca aille plus vite */
IPAddress adr_ip(192, 168, 0, 17);		// Duron (décommissionné)
IPAddress adr_gateway(192, 168, 0, 10);
IPAddress adr_dns(192, 168, 0, 3);

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
DallasTemperature DS18B20(&oneWire);

	/* Gestion individualisée des sondes */
int nbr_sondes;	// Combien avons nous trouvé de sondes ?
DeviceAddress *sondes_adr;	// pour stocker les adresses des sondes

String Adr2String(DeviceAddress adr){	/* Convertie une adresse en chaine */
	String str;
	for(int i=0; i<8; i++){
		if(adr[i]<16) str += '0';	// Padding si necessaire
		str += String(adr[i], HEX);
	}
	return str;
}

void setup(){
	Serial.begin(115200);	// débuging
	pinMode(LED_BUILTIN, OUTPUT);	// La LED est allumée pendant la recherche de WiFi et MQTT
	delay(10);	// Le temps que ça se stabilise

	/* Configuration réseau */
	WiFi.config(adr_ip, adr_gateway, adr_dns);
	WiFi.mode(WIFI_STA);
	wifi_set_sleep_type(LIGHT_SLEEP_T);
	connexion_WiFi();
	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);

	/* 1-wire */
	Serial.print("Bus 1-wire utilisé :");
	Serial.println(ONE_WIRE_BUS);

	/* Sondes DS18B20 */
	DS18B20.begin();

	Serial.print("Nombre de sondes :");
	Serial.println(nbr_sondes = DS18B20.getDeviceCount());

	if(!(sondes_adr = (DeviceAddress *)malloc( sizeof(DeviceAddress) * nbr_sondes))){
		Serial.println("Impossible d'allouer les adresses des sondes.\nSTOP !");
		ESP.deepSleep(0);	// On s'arrete là
	}
	Serial.println("Le stockage des adresses des sondes est alloué");

	int duree = millis();
	for(int i=0; i<nbr_sondes; i++){
		if(DS18B20.getAddress(sondes_adr[i], i)){
			Serial.print("Sonde ");
			Serial.print(i);
			Serial.print(": ");
			Serial.println(Adr2String( sondes_adr[i] ));
			DS18B20.setResolution( sondes_adr[i], i ? 12:9);
		} else {
			Serial.print("Impossible de lire l'adresse de la Sonde ");
			Serial.println(i);
		}
		yield();
	}
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
	DS18B20.requestTemperatures();
	Serial.print("La demande d'aquisition a durée ");
	Serial.print( millis() - duree );
	Serial.println(" milli-secondes");

	duree = millis();	// Ainsi, nous tentons de mesurer aussi les latences dues à la conversion
	for(int i=0; i<nbr_sondes; i++){
		String adr = Adr2String( sondes_adr[i] );
		float temp;

		Serial.print("Température pour ");
		Serial.print( adr );
		Serial.print(" : ");
		Serial.print( temp = DS18B20.getTempC( sondes_adr[i]));	
		Serial.print("°C (");
		Serial.print(DS18B20.getResolution(sondes_adr[i]));
		Serial.print(" bits) en ");
		Serial.print( millis() - duree );
		Serial.println(" milli-secondes");

		clientMQTT.publish( (MQTT_Topic + adr).c_str(), String( temp ).c_str() );
		yield();
		duree = millis();
	}

	delay(300e3);	// Prochaine mesure dans 5 minutes
}

