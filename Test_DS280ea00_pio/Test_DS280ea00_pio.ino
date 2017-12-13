/* Test pour voir si le PIO.B est forcée en sortie avec un détecteur de lumière */
#include <OWBus.h>
#include <OWBus/DS28EA00.h>

#define ONE_WIRE_BUS 5			// Where 1-wire bus is connected to (GPIO-5)
OneWire oneWire(ONE_WIRE_BUS);	// Initialize oneWire library
OWBus bus(&oneWire);


void setup() {
	Serial.begin(115200);
	delay(100);
}

void status(DS28EA00 &p){
	uint8_t pios = p.readPIOs();
	
	Serial.print(pios, HEX);
	Serial.print(" (val, flipflop) PIO.A : ");
	Serial.print(p.getPIOA() ? "1,":"0,");
	Serial.print(p.getFlipFlopA() ? "1":"0");
	Serial.print(" / PIO.B : ");
	Serial.print(p.getPIOB() ? "1,":"0,");
	Serial.println(p.getFlipFlopB() ? "1":"0");
}

void loop() {
	DS28EA00 p( bus, 0x42886847000000bf );
	p.readPIOs();	// Get "initiale" value

/*
	Serial.print("Initiale : ");
	status(p);
	delay(1e3);
*/

	Serial.println("\nPIO.A = On (0)");
	p.writePIOs( p.getFlipFlopB() ? DS28EA00::PIObitsvalue::PIOBbit : 0 );	// garde la valeur du PIO.B
	status(p);
	delay(1e3);

	Serial.println("Retour état initial (PIO.A = 1)");
	p.writePIOs(
		DS28EA00::PIObitsvalue::PIOAbit | 
		( p.getFlipFlopB() ? DS28EA00::PIObitsvalue::PIOBbit : 0 )
	);
	status(p);
	delay(1e3);
}
