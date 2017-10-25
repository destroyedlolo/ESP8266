/* 1-wire Temperature Probe definition
 *
 * 25/10/2017 First version
 */
#include "Probe.h"

class TemperatureProbe : public Probe {
	static 	DallasTemperature *bus;	// zeroized as per standard

	static void assertBus(){
		if(!bus)
			Serial.println("TemperatureProbe's bus not initialised");
	}

public:
	static void Init( OneWire &b ){
		bus = new DallasTemperature(&b);
		bus->begin();
	}

	TemperatureProbe(const char *aname, DeviceAddress &aaddr) : Probe(aname, aaddr){
		assertBus();
	}

	static void requestTemperatures(){
		assertBus();
		bus->requestTemperatures();
	}

	float getTempC(){
		return(bus->getTempC( getAddr() ));
	}

	uint8_t getResolution(){
		return(bus->getResolution( getAddr() ));
	}

	uint8_t setResolution( uint8_t r){
		return(bus->setResolution( getAddr(), r ));
	}
};
