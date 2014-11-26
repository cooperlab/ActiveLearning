#include <fstream>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

#include <jansson.h>
#include <libconfig.h++>

#include "learner.h"
#include "logger.h"

#include "base_config.h"




using namespace std;
using namespace libconfig;



bool	gDone = false;		// Sig handler will set this to true
EvtLogger	*gLogger = NULL;

#define RX_BUFFER_SIZE		(100 * 1024)
static char *gBuffer = NULL;


//
// Change the process's session and direct all output to
// a log file.
//
bool Daemonize(void)
{
	bool	result = true;
	pid_t	pid, sid;

	// Fork child process
	pid = fork();
	if( pid < 0 ) {
		// fork error
		result = false;
	}

	if( result && pid > 0 ) {
		// Kill parent process
		exit(EXIT_SUCCESS);
	}

	// Change umask
	if( result ) {
		umask(0);
		sid = setsid();
		if( sid < 0 ) {
			result = false;
		}
	}

	// Change to root directory
	if( result ) {
		if( chdir("/") < 0 ) {
			result = false;
		}
	}

	// Close stdio
	if( result ) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	return result;
}





// Create the needed objects
//
Learner* Initialize(string dataPath, string outPath)
{
	bool	result = true;
	Learner	*learner = NULL;

	learner = new Learner(dataPath, outPath);
	if( learner == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create learner object");
	}
	return learner;
}







bool HandleRequest(const int fd, Learner *learner)
{
	bool	result = true;
	int		bytesRx;


	bytesRx = recv(fd, gBuffer, RX_BUFFER_SIZE, 0);

	if( bytesRx > 0 ) {
		result = learner->ParseCommand(fd, gBuffer, bytesRx);

	} else {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid request");
	}
	return result;
}






bool ReadConfig(string& dataPath, string& outPath, short& port, string& interface)
{
	bool	result = true;
	Config	config;
	int		readPort;

	// Load & parse config file
	try{
		config.readFile("/etc/al_server/al_server.conf");
	} catch( const FileIOException &ioex ) {
		result = false;;
	}
	
	// Data path
	try {
		 dataPath = (const char*)config.lookup("data_path");
	} catch( const SettingNotFoundException &nfEx ) {
		result = false;
	}

	// Out path
	try {
		 outPath = (const char*)config.lookup("out_path");
	} catch( const SettingNotFoundException &nfEx ) {
		result = false;
	}

	// Server port
	try {
		config.lookupValue("port", readPort);
	} catch(const SettingNotFoundException &nfEx) {
		result = false;
	}
	port = (short)readPort;
	
	// interface address
	try { 
		interface = (const char*)config.lookup("interface"); 
	} catch( const SettingNotFoundException &nfEx ) {
		// Default to localhost if not in config file.
		interface = "127.0.0.1";
	}
	
	return result;
}








int main(int argc, char *argv[])
{
	int status = 0;
	Learner	*learner = NULL;
	short 	port;
	string 	dataPath, outPath, interface;

	gLogger = new EvtLogger("/var/log/al_server.log");
	if( gLogger == NULL ) {
		status = -1;
	}

	if( status == 0 ) {
		if( !ReadConfig(dataPath, outPath, port, interface) ) {
			status = -1;
		} else {
			char portBuff[10];
			snprintf(portBuff, 10, "%d", port);
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Listening on interface: " + interface + 
												 " port: " + string(portBuff));
		}
	}

	if( status == 0 ) {
		Daemonize();
	}

	if( status == 0 ) {
		gBuffer = (char*)malloc(RX_BUFFER_SIZE);
		if( gBuffer == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to allocate RX buffer");;
			status = -1;
		}
	}

	if( status == 0 ) {
		// Setup Active learning objects
		learner = Initialize(dataPath, outPath);
		if( learner == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to initialize server");
			status = -1;
		}

		if( status == 0 ) {

			int listenFD = 0, connFD = 0;
			struct sockaddr_in serv_addr, test;

			// Setup socket
			listenFD = socket(AF_INET, SOCK_STREAM, 0);
			memset(&serv_addr, 0, sizeof(struct sockaddr_in));

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = inet_addr(interface.c_str());
			serv_addr.sin_port = htons(port);

			bind(listenFD, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
			listen(listenFD, 10);

			struct sockaddr_in	peer;
			int	len = sizeof(peer);
			char msg[100];
			// Event loop...
			while( !gDone ) {

				// Get a connection
				connFD = accept(listenFD, (struct sockaddr*)&peer, (socklen_t*)&len);

				snprintf(msg, 100, "Request from %s", inet_ntoa(peer.sin_addr));
				gLogger->LogMsg(EvtLogger::Evt_INFO, msg);

				if( HandleRequest(connFD, learner) == false ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "Request failed");
				}
				close(connFD);
				sleep(1);
			}
		}

		if( gBuffer )
			free(gBuffer);
		if( learner )
			delete learner;
	}
	if( gLogger )
		delete gLogger;

	return status;
}

