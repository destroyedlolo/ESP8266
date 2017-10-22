/* Test d'acquisition de température par 1-wire
 *  
 *  Par Destroyedlolo, mis dans le Domaine Publique.
 */

#include <OneWire.h>						// Gestion du bus 1-wire
#include <DallasTemperature.h>	// Sondes ds18b20

ADC_MODE(ADC_VCC);

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
	delay(10);	// Le temps que ça se stabilise

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
	Serial.print("Alimentation : ");
	Serial.println(ESP.getVcc());
	
	int duree = millis();
	DS18B20.requestTemperatures();
	Serial.print("La demande d'aquisition a durée ");
	Serial.print( millis() - duree );
	Serial.println(" milli-secondes");
	
	for(int i=0; i<nbr_sondes; i++){
		duree = millis();
		Serial.print("Température pour ");
		Serial.print(Adr2String( sondes_adr[i] ));
		Serial.print(" : ");
		Serial.print(DS18B20.getTempC( sondes_adr[i]));
		Serial.print("°C (");
		Serial.print(DS18B20.getResolution(sondes_adr[i]));
		Serial.print(" bits) en ");
		Serial.print( millis() - duree );
		Serial.println(" milli-secondes");
		yield();
	}

	delay(300e3);	// Prochaine mesure dans 5 minutes
}

