#include <iostream>
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

#include <jansson.h>

#include "learner.h"

#include "cmdline.h"
#include "base_config.h"




using namespace std;


bool	gDone = false;		// Sig handler will set this to true



//
// Change the process's session and direct all output to
// a log file.
//
bool Daemonize(void)
{
	bool	result = true;

	// TODO - Daemonize the process and convert all output to
	//		  stdio and stderr to log file output

	return result;
}





// Create the needed objects
//
Learner* Initialize(void)
{
	bool	result = true;
	Learner	*learner = NULL;

	learner = new Learner();
	if( learner == NULL ) {
		cerr << "Unable to create learner object" << endl;
	}
	return learner;
}







bool HandleRequest(const int fd, Learner *learner)
{
	bool	result = true;
	char 	buffer[8192];
	int		bytesRx;


	bytesRx = recv(fd, buffer, 8192, 0);
	if( bytesRx > 0 ) {
		result = learner->ParseCommand(fd, buffer, bytesRx);

	} else {
		cout << "Invalid request" << endl;
	}
	return result;
}






int main(int argc, char *argv[])
{
	int status = 0;
	gengetopt_args_info		args;
	Learner	*learner = NULL;


	status = cmdline_parser(argc, argv, &args);

	if( status == 0 ) {

		// Daemonize the process
		Daemonize();

		// Setup Active learning objects
		learner = Initialize();
		if( learner == NULL ) {
			cerr << "Unable to initialize server" << endl;
			status = -1;
		}

		if( status == 0 ) {

			int listenFD = 0, connFD = 0;
			struct sockaddr_in serv_addr, test;
			short	port = args.port_arg;

			// Setup socket
			listenFD = socket(AF_INET, SOCK_STREAM, 0);
			memset(&serv_addr, 0, sizeof(struct sockaddr_in));

			cout << "Starting on port " << port << endl;
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			serv_addr.sin_port = htons(port);

			cout << "After htons " << serv_addr.sin_port << endl;

			bind(listenFD, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
			listen(listenFD, 10);

			// Event loop...
			while( !gDone ) {

				// Get a connection
				connFD = accept(listenFD, (struct sockaddr*)NULL, NULL);

				HandleRequest(connFD, learner);
				close(connFD);
				sleep(1);
			}
		}

		if( learner )
			delete learner;
	}
	return status;
}

