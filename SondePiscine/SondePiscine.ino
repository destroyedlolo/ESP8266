/* SondePiscine
 *
 *	Simple sonde alimentée par des cellules photoélectriques 
 *	et qui revoie par MQTT la température de l'eau de la piscine.
 *
 *	provided by "Laurent Faillie"
 *	Licence : "Creative Commons Attribution-NonCommercial 3.0"
 */


	/*******
	* Paramétrage
	********/
#define DEV	// On est mode developpement

#ifdef DEV
#	define MQTT_CLIENT "SondePiscine-Dev"

	/* Message de Debug sur le tx.
	 * Pas de LED vu qu'il est sur le même PIN
	 */
#	define LED(x)	{ }
#	define SERIAL_ENABLED
#else
#	define MQTT_CLIENT "SondePiscine"

	/* La LED est allumée pendant la recherche du réseau et l'établissement
	 * du MQTT
	 */
#	define LED(x)	{ digitalWrite(LED_BUILTIN, x); }
#endif

#define ONE_WIRE_BUS 5

String MQTT_Topic = String(MQTT_CLIENT) + "/";	// Topic's root
String MQTT_Output = MQTT_Topic + "Message";
String MQTT_WiFi = MQTT_Topic + "WiFi";
String MQTT_MQTT = MQTT_Topic + "MQTT";
String MQTT_VCC = MQTT_Topic + "Vcc";
String MQTT_TempInterne = MQTT_Topic + "TempInterne";
String MQTT_TempPiscine = MQTT_Topic + "TempPiscine";
String MQTT_Command = MQTT_Topic + "Command";

	/* Paramètres par défaut */
	/* Durées (secondes) */
#define DEF_DUREE_SOMMEIL 30	// Sommeil entre 2 acquisitions
#define DEF_EVEILLE	60			// Durée où il faut rester éveillé après avoir recu une commande

	/******
	* Fin du paramétrage
	*******/


	/*******
	* Gestion des configurations
	********/

#include <KeepInRTC.h>
KeepInRTC kir;	// Gestionnaire de la RTC

class Config : public KeepInRTC::KeepMe {
	unsigned long consigne;		// Consigne à sauvegarder
	unsigned long prochaine;	// prochaine échéance

public:
	Config() : KeepInRTC::KeepMe( kir, (uint32_t *)&this->consigne, sizeof(this->consigne) ), prochaine(0) {}

	void setConsigne( unsigned long val ){	this->consigne = val; }
	unsigned long getConsigne( void ){ return this->consigne; }

	/* Réinitialise la consigne si nécessaire.
	 * -> val : valeur par défaut
	 * <- true si la RTC était invalide
	 */
	bool begin( unsigned long val ){
		if( !kir.isValid() ){
			this->setConsigne( val );
			this->save();
			return true;
		}
		return false;
	}

	void setProchain( unsigned long val ){	this->prochaine = val; }
	unsigned long getProchain( void ){ return this->prochaine; }
};

Config Sommeil;			// Sommeil entre 2 acquisitions
Config EveilInteractif;	// Temps pendant lequel il faut rester éveillé en attendant des ordres.


	/*******
	* Gestion de la communication
	********/
#include <ESP8266WiFi.h>
#include <Maison.h>		// Paramètres de mon réseau
#include "Duree.h"

extern "C" {
  #include "user_interface.h"
}
ADC_MODE(ADC_VCC);	// Nécessaire pour avoir la tension d'alimentation

#include <OWBus.h>
#include <OWBus/DS18B20.h>
OneWire oneWire(ONE_WIRE_BUS, true);	// Initialise la bibliothèque en activant le pullup (https://github.com/destroyedlolo/OneWire)
OWBus bus(&oneWire);

#include <PubSubClient.h>	// Il faut la version permettant un QoS > 0 (/ESP/TrucAstuces/MQTT_ESP8266/)
WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);

void Connexion_MQTT();

/* Publie un message MQTT
 * -> topic, msg : les informations à publier
 *		reconnect : indique s'il faut se reconnecter si la connexion est perdu
 *			(si false, on ignore le message si deconnecté)
 */
void publish( String &topic, const char *msg, bool reconnect=true ){
	if(!clientMQTT.connected()){
		if( reconnect )
			Connexion_MQTT();
		else
			return;
	}
	clientMQTT.publish( topic.c_str(), msg );
}

void publish( String &topic, const unsigned long val, bool reconnect=true ){
	publish( topic, String(val, DEC).c_str(), reconnect );
}

inline void logmsg( const char *msg ){
	publish( MQTT_Output, msg );
#	ifdef SERIAL_ENABLED
	Serial.println( msg );
#endif
}
inline void logmsg( String &msg ){ logmsg( msg.c_str() ); }



/***********
 * Gestion des commandes MQTT
 ***********/
bool func_status( const String & ){
	String msg = "Délai acquisition : ";
	msg += Sommeil.getConsigne();
	msg += "\nEveil suite à commande : ";
	msg += EveilInteractif.getConsigne();

	logmsg( msg );
	return true;
}

bool func_delai( const String &arg ){
	int n = arg.toInt();

	if( n > 0){
		Sommeil.setConsigne( n );
		String msg = "Délai changé à ";
		msg += n;
		logmsg( msg );
	} else
		logmsg( "Argument invalide : Délai inchangé" );

	return true;
}

bool func_att( const String &arg ){
	int n = arg.toInt();

	if( n > 0){
		EveilInteractif.setConsigne( n );
		String msg = "Attente changée à ";
		msg += n;
		logmsg( msg );
	} else
		logmsg( "Argument invalide : Attente inchangée" );

	return true;
}

bool func_dodo( const String & ){
	EveilInteractif.setProchain( 0 );
	return false;
}

bool func_reste( const String &arg ){
	int n = arg.toInt();
	EveilInteractif.setProchain( millis() + n*1e3 );

	String msg = "Reste encore éveillé pendant ";
	msg += n;
	msg += " seconde";
	if( n > 1 )
		msg += 's';
	
	logmsg( msg );
	return false;
}

const struct _command {
	const char *nom;
	const char *desc;
	bool (*func)( const String & );	// true : raz du timer d'éveil
} commands[] = {
	{ "status", "Configuration courante", func_status },
	{ "delai", "Délai entre chaque échantillons (secondes)", func_delai },
	{ "attente", "Attend <n> secondes l'arrivée de nouvelles commandes", func_att },
	{ "dodo", "Sort du mode interactif et place l'ESP en sommeil", func_dodo },
	{ "reste", "Reste encore <n> secondes en mode interactif", func_reste },
	{ NULL, NULL, NULL }
};

void handleMQTT(char* topic, byte* payload, unsigned int length){
	String ordre;
	for(unsigned int i=0;i<length;i++)
		ordre += (char)payload[i];

#	ifdef SERIAL_ENABLED
	Serial.print( "Message [" );
	Serial.print( topic );
	Serial.print( "] : '" );
	Serial.print( ordre );
	Serial.println( "'" );
#	endif

		/* Extrait la commande et son argument */
	const int idx = ordre.indexOf(' ');
	String arg;
	if(idx != -1){
		arg = ordre.substring(idx + 1);
		ordre = ordre.substring(0, idx);
	}

	bool raz = true;	// Faut-il faire un raz du timer interactif
	if( ordre == "?" ){	// Liste des commandes connues
		String rep;
		if( arg.length() ) {
			rep = arg + " : ";

			for( const struct _command *cmd = commands; cmd->nom; cmd++ ){
				if( arg == cmd->nom && cmd->desc ){
					rep += cmd->desc;
					break;	// Pas besoin de continuer la commande a été trouvée
				}
			}
		} else {
			rep = "Liste des commandes reconnues :";

			for( const struct _command *cmd = commands; cmd->nom; cmd++ ){
				rep += ' ';
				rep += cmd->nom;
			}
		}

		logmsg( rep );
	} else {
		for( const struct _command *cmd = commands; cmd->nom; cmd++ ){
			if( ordre == cmd->nom && cmd->func ){
				raz = cmd->func( arg );
				break;
			}
		}
	}

	if( raz )		// Raz du timer d'éveil
		EveilInteractif.setProchain( millis() + EveilInteractif.getConsigne() * 1e3 );
}

void Connexion_MQTT(){
	LED(LOW);
#ifdef SERIAL_ENABLED
	Serial.println("Connexion MQTT");
#endif

	Duree dMQTT;
	for( int i=0; i< 240; i++ ){
		if(clientMQTT.connect(MQTT_CLIENT, false)){	// false prevent commands to be cleared
			LED(HIGH);
			dMQTT.Fini();

#ifdef SERIAL_ENABLED
	Serial.print("Duree connexion MQTT :");
	Serial.println( *dMQTT );
#endif
			publish( MQTT_MQTT, *dMQTT, false );
			clientMQTT.subscribe(MQTT_Command.c_str(), 1);
			return;
		} else {
			Serial.print("Echec, rc:");
			Serial.println(clientMQTT.state());
			delay(500);	// Test dans 1 seconde
		}
	}
	LED(HIGH);
#ifdef SERIAL_ENABLED
	Serial.println("Impossible de se connecter au MQTT");
#endif
	ESP.deepSleep( Sommeil.getConsigne() * 1e6 );	// On essaiera plus tard
}

void setup(){
#ifdef SERIAL_ENABLED
	Serial.begin(115200);
	delay(100);
	Serial.println("Réveil");
#else
	pinMode(LED_BUILTIN, OUTPUT);
#endif

	if( Sommeil.begin(DEF_DUREE_SOMMEIL) | EveilInteractif.begin(DEF_EVEILLE) ){	// ou logique sinon le begin() d'EveilInteractif ne sera jamais appelé
#ifdef SERIAL_ENABLED
		Serial.println("Valeur par défaut");
#endif
	}

	Duree dwifi;
#ifdef DEV
	WiFi.begin( WIFI_SSID, WIFI_PASSWORD );	// Connexion à mon réseau domestique
#else
	WiFi.begin( DOMO_SSID, DOMO_PASSWORD );	// Connexion à mon réseau domotique
#endif

	for( int i=0; i< 240; i++ ){
		if(WiFi.status() == WL_CONNECTED){
#ifdef SERIAL_ENABLED
				Serial.println("ok");
#endif
				break;
			}
			delay(500);
#ifdef SERIAL_ENABLED
			Serial.print("=");
#endif
		}
		if(WiFi.status() != WL_CONNECTED){
#ifdef SERIAL_ENABLED
			Serial.println("Impossible de se connecter");
#endif
			ESP.deepSleep( Sommeil.getConsigne() * 1e6 );	// On essaiera plus tard
		}

	dwifi.Fini();
#ifdef SERIAL_ENABLED
	Serial.print("Duree connexion Wifi :");
	Serial.println( *dwifi );
#endif

	clientMQTT.setServer(BROKER_HOST, BROKER_PORT);
	clientMQTT.setCallback( handleMQTT );
	publish( MQTT_WiFi, *dwifi );
}

void loop(){
	if( Sommeil.getProchain() < millis() ){	// il est temps de faire une nouvelle acquisition
#ifdef SERIAL_ENABLED
		Serial.print("Tension d'alimentation :");
		Serial.println( ESP.getVcc() );
#endif
		publish( MQTT_VCC, ESP.getVcc() );

		DS18B20 SondeTempInterne(bus, 0x2882b25e09000015);
		DS18B20 SondePiscine( bus, 0x28ff8fbf711703c3);

		float temp =  SondeTempInterne.getTemperature( false );
		if( SondeTempInterne.isValidScratchpad() ){	// On s'assure que la sonde est présente
#ifdef SERIAL_ENABLED
			Serial.print("Température Interne :");
			Serial.println( temp );
#endif
			publish( MQTT_TempInterne, String( temp ).c_str() );
		}

		temp = SondePiscine.getTemperature( false );
		if( SondePiscine.isValidScratchpad() ){	// On s'assure que la sonde est présente
#ifdef SERIAL_ENABLED
			Serial.print("Température piscine :");
			Serial.println( temp );
#endif
			publish( MQTT_TempPiscine, String( temp ).c_str() );
		}

		Sommeil.setProchain( millis() + Sommeil.getConsigne() * 1e3 );	// Calcul de la prochaine acquisition
	}

	if(clientMQTT.connected())
			clientMQTT.loop();

	if( EveilInteractif.getProchain() < millis() ){	// Pas de session interactive en cour
#ifdef SERIAL_ENABLED
			Serial.println( "Dodo ..." );
#endif
		ESP.deepSleep( Sommeil.getConsigne() * 1e6);
	} else 
		delay( 5000 );	// Attend 5s avant de vérifier à nouveau s'il y a des commandes MQTT
}
