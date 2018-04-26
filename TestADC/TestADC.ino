/* TestADC
 *
 *	Test la qualité du convertisseur ADC
 *
 *	provided by "Laurent Faillie"
 *	Licence : "Creative Commons Attribution-NonCommercial 3.0"
 */

extern "C" {
  #include "user_interface.h"
}

ADC_MODE(ADC_VCC);	// Nécessaire pour avoir la tension d'alimentation

void setup(){
	Serial.begin(115200);
	delay(100);
}

void loop(){
	Serial.println( ESP.getVcc() );

	delay(2000);
}

