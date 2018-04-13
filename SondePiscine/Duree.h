/* Classe pour mesurer une durée
 *
 * 25/10/2017 First version
 * 13/04/2018 Francisation
 */
#ifndef DUREE_H
#define DUREE_H

class Duree {
	unsigned long int debut, fin;

	public:
		Duree( void ) { reInit(); }
		void reInit( void ) { debut = millis(); fin = 0; }
		unsigned long int Fini( void ){ fin = millis(); return (fin - debut); }
		unsigned long int operator *( void ){ return( (fin ? fin : millis()) - debut ); }
};

#endif
