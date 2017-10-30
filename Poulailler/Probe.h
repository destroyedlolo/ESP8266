/* 1-wire Probe definition
 *
 * 25/10/2017 First version
 */

#include <OneWire.h>
#include <DallasTemperature.h>	// Because of DeviceAddress (stupid)

class Probe {
	DeviceAddress &addr;
	const char *name;

	public :
		Probe(const char *aname, DeviceAddress &aaddr) : addr( aaddr ) {
			name = aname;
		}

	const char *getName() { return name; };
	DeviceAddress &getAddr() { return addr; };

	static String AddressString( DeviceAddress &addr ){
		String str;
		for(int i=0; i<8; i++){
			if(addr[i]<16) str += '0';	// Padding si necessaire
			str += String(addr[i], HEX);
		}
		return str;
	}

	String AddressString(){
		return this->AddressString(addr);
	}
};

