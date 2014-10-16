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

#define RX_BUFFER_SIZE		(100 * 1024)
static char *gBuffer = NULL;


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
Learner* Initialize(string path)
{
	bool	result = true;
	Learner	*learner = NULL;

	learner = new Learner(path);
	if( learner == NULL ) {
		cerr << "Unable to create learner object" << endl;
	}
	return learner;
}







bool HandleRequest(const int fd, Learner *learner)
{
	bool	result = true;
	int		bytesRx;


	bytesRx = recv(fd, gBuffer, RX_BUFFER_SIZE, 0);
	cout << "Read " << bytesRx << " bytes" << endl;
	if( bytesRx > 0 ) {
		result = learner->ParseCommand(fd, gBuffer, bytesRx);

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
	string  path = args.data_path_arg;

	if( status == 0 ) {

		// Daemonize the process
		Daemonize();
	}

	if( status == 0 ) {
		gBuffer = (char*)malloc(RX_BUFFER_SIZE);
		if( gBuffer == NULL ) {
			cerr << "Unable to allocate RX buffer" << endl;
			status = -1;
		}
	}

	if( status == 0 ) {
		// Setup Active learning objects
		learner = Initialize(path);
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

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			serv_addr.sin_port = htons(port);

			bind(listenFD, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
			listen(listenFD, 10);

			struct sockaddr_in	peer;
			int	len = sizeof(peer);

			// Event loop...
			while( !gDone ) {

				// Get a connection
				connFD = accept(listenFD, (struct sockaddr*)&peer, (socklen_t*)&len);

				cout << "Request from " << inet_ntoa(peer.sin_addr) << endl;

				HandleRequest(connFD, learner);
				close(connFD);
				sleep(1);
			}
		}

		if( gBuffer )
			free(gBuffer);
		if( learner )
			delete learner;
	}
	return status;
}

