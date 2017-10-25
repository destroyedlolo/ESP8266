/* 1-wire Probe definition
 *
 * 25/10/2017 First version
 */

#include <OneWire.h>

class Probe {
	DeviceAddress &addr;
	const char *name;

	public :
		Probe(const char *aname, DeviceAddress &aaddr) : addr( aaddr ) {
			name = aname;
		}

	const char *getName() { return name; };
	DeviceAddress &getAddr() { return addr; };
};

