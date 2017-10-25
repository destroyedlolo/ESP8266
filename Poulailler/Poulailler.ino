/*	Poulailler
 * 	
 * 	This is my chicken coop automation
 *
 *	provided by "Laurent Faillie"
 *	Licence : "Creative Commons Attribution-NonCommercial 3.0"
 *	
 *	History :
 *	25/10/2017 - Temperature probe only
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

extern "C" {
  #include "user_interface.h"
}

	/******
	 * Parameters
	 ******/

	/* LED */
#define LED_BUILTIN 2	// On my ESP-12
#if 1					// Led is lightning during Wifi / Mqtt connection establishment
#	define LED(x)	{ digitalWrite(LED_BUILTIN, x); }
#else
#	define LED(x)
#endif

	/* Network */
#include "Maison.h"		// My very own environment (WIFI_*, MQTT_*, ...)
IPAddress adr_ip(192, 168, 0, 17);	// Static IP to avoid DHCP delay
IPAddress adr_gateway(192, 168, 0, 10);
IPAddress adr_dns(192, 168, 0, 3);

	/* MQTT */
#define MQTT_CLIENT "Poulailler"
String MQTT_Topic("Poulailler/");	// Topic root
#define DELAY	300	// Delay in seconds b/w samples

	/* 1-wire */
#define ONE_WIRE_BUS D1	// Where D1 bus is connected to

	/* Probes */
#include "Probe.h"

Probe ptemperatures[] = {
	Probe( "TestTemp", (DeviceAddress){ 0x28, 0x82, 0xb2, 0x5e, 0x09, 0x00, 0x00, 0x15 })	
};


	/* End of configuration area
	 * Let's go !
	 */
#include "Duration.h"

ADC_MODE(ADC_VCC);

WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DSTemp(&oneWire);

bool startWiFi( class Duration &d ){
	Serial.println("Starting WiFi");

	d.reInit();
	for( int i=0; i< 120; i++ ){	// Trying with autoconnect
		if(WiFi.status() == WL_CONNECTED)
			break;
		delay(500);
		Serial.print("-");
	}

	if(WiFi.status() != WL_CONNECTED){	// Failed, starting manually
		WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
		for( int i=0; i< 240; i++ ){
			if(WiFi.status() == WL_CONNECTED)
				break;
			delay(500);
			Serial.print(".");
		}
	}
	d.Finished();
	
	Serial.println("ok");
	WiFi.printDiag( Serial );
	Serial.print("AutoConnect : ");
	Serial.println(WiFi.getAutoConnect() ? "Oui" : "Non");

	return(WiFi.status() == WL_CONNECTED);
}

bool startMQTT( class Duration &d ){
	Serial.println("Starting MQTT");

	d.reInit();
	for( int i=0; i< 60; i++ ){
		if(clientMQTT.connect(MQTT_CLIENT)){
			Serial.println("Connected");
			break;
		} else {
			Serial.print("Failure, rc:");
			Serial.println(clientMQTT.state());
			delay(1000);	// New try in 1 second
		}
	}

	return(clientMQTT.connected());
}

void setup() {
	Duration dwifi, dmqtt;

	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);
	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);

	LED(LOW);
	startWiFi( dwifi );	// If false, have to return to sleep

	startMQTT( dmqtt );
	LED(HIGH);
	Serial.print("WiFi connection duration :");
	Serial.println( *dwifi );
	Serial.print("MQTT connection duration :");
	Serial.println( *dmqtt );
	clientMQTT.publish( (MQTT_Topic + "Wifi").c_str(), String( *dwifi ).c_str() );
	clientMQTT.publish( (MQTT_Topic + "MQTT").c_str(), String( *dmqtt ).c_str() );
}

void loop() {
  // put your main code here, to run repeatedly:

}
