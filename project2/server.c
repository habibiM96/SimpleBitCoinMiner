/* A simple server in the internet domain using TCP
The port number is passed as an argument


 To compile: gcc server.c -o server
*/

#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/select.h>


#define TRUE 1
#define FALSE 0
#define MAX_ERROR 64
#define MAX_CLIENT 100
#define MAX_BUFFER 256
#define PONG "PONG"
#define OKAY "OKAY"
#define PING "PING"


int buffer_analyser(char *buffer,int buffer_size, int socket_id);
int main(int argc, char **argv)
{
	int opt = TRUE;
	int listener_socket = -1, clilen = -1, new_sockfd = -1, client_socks[MAX_CLIENT],
	activity = 0, client_count = 0, value_read = -1, i = 0, server_portno, max_sd = 0;

	char buffer[MAX_BUFFER];
	char ERROR[MAX_ERROR];
	struct sockaddr_in serv_addr, cli_addr;

	fd_set client_read_fds; //set of sockets descriptors

	//initialise all client sockets to 0 so not checked
	for(i = 0; i < MAX_CLIENT; i++){
		client_socks[i] = 0;
	}

	//receiving the port number for server from user
	if (argc < 2){
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	 /* Create TCP socket */
	listener_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (listener_socket < 0){
		perror("ERROR opening socket");
		exit(1);
	}

	//set master socket to allow multiple connections
  if( setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
      perror("couldn't set multiple clients access for the server");
      exit(EXIT_FAILURE);
  }

	bzero((char *) &serv_addr, sizeof(serv_addr));

	server_portno = atoi(argv[1]);//server port number for server

	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for
	 this machine */

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(server_portno);  // store in machine-neutral format

	 /* Bind address to the socket */

	if (bind(listener_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("ERROR on binding");
		exit(1);
	}

	/* Listen on socket - means we're ready to accept connections -
	 incoming connection requests will be queued */

	 if (listen(listener_socket, 5) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
	 }

	clilen = sizeof(cli_addr);

	while(TRUE){
		//clear the client sockets reads-file descriptors
		FD_ZERO(&client_read_fds);

		//add listener socket to client sockets file descriptors set
		FD_SET(listener_socket, &client_read_fds);
		max_sd = listener_socket;

		//adding child socket to client_read_fds
		for(i = 0; i < MAX_CLIENT; i++){

			if(client_socks[i] > 0){
				//if the sock descriptor is set before
				FD_SET(client_socks[i], &client_read_fds);
			}

			if(client_socks[i] > max_sd){
				// keep a record of the highest socket descriptor
				max_sd = client_socks[i];
			}
		}

		//set up the select to monitor activity on the sockets, it waits indefinitely
		activity = select(max_sd + 1, &client_read_fds, NULL, NULL, NULL);

		if ((activity < 0) && (errno!=EINTR)){
			printf("select error");
		}

		if (FD_ISSET(listener_socket, &client_read_fds)){
			//receiving a new connection request
			if ((new_sockfd = accept( listener_socket, (struct sockaddr *) &cli_addr, &clilen)) < 0){
				perror("ERROR on accept");
				exit(EXIT_FAILURE);
			}

			for(i = 0; i < MAX_CLIENT; i++){
				//adding the new socket to the list
				if(client_socks[i] == 0){
					//found an empty one
					client_socks[i] = new_sockfd;
					break;
				}
			}
		}
		bzero(buffer,MAX_BUFFER);
		bzero(ERROR, MAX_ERROR);

		for(i = 0; i < MAX_CLIENT; i++){

			if (FD_ISSET(client_socks[i], &client_read_fds)){
				//check for any incoming message
				value_read = read(client_socks[i], buffer, MAX_BUFFER - 1);
				if( value_read == 0){
					//client disconnected
					printf("closing client \n");
					close(client_socks[i]);
					client_socks[i] = 0;
				} else if (value_read < 0){
					//receiving a message, return a response
					perror("ERROR reading from socket");
					exit(EXIT_FAILURE);
				} else {
					buffer[value_read] = '\0';
					//buffer_analyser(buffer, value_read);
					printf("Writing message back\n");
					buffer_analyser(buffer, value_read, client_socks[i]);
					//value_read = write(client_socks[i], buffer, value_read);
				}
			}
		}
	}

	return 0;
}

int buffer_analyser(char *buffer, int buffer_size, int sock_id){
	int value_read = 0;
	int i = 0;
	char *token;
	/*for(i = 0; i < buffer_size; i++){
		if(buffer[i] == '\r'){
			printf("Found CR || ");
		}
		if(buffer[i] == '\n'){
			printf("found NL\n");
		}
	}*/
    const char delim[3] = "\r\n";
    token = strtok(buffer, "\r\n");
    while(token){
        //strncpy(msg, token, 4);
		printf("token: %s && size: %d\n",token, strlen(token));
		//printf("msg: %s\n",msg);

        if((strncmp(token, PING, 4) == 0) && (strlen(token) == 4)){
			value_read = write(sock_id, "PONG\r\n", 6);
			printf("token1\n");
		} else if ((strncmp(token, PONG, 4) == 0) && (strlen(token) == 4)){
			printf("token2\n");
			value_read = write(sock_id, "ERRO Only Server can send PONG\r\n", 32);
		} else if ((strncmp(token, OKAY, 4) == 0) && (strlen(token) == 4)){
			printf("token3\n");
			value_read = write(sock_id, "ERRO Only Sever can send OKAY\r\n", 32);
		} else if ((strncmp(token, "ERRO", 4) == 0) && (strlen(token) == 4)){
			printf("token4\n");
            value_read = write(sock_id, "ERRO must not be sent to the server\r\n", 37);
		} else if(strncmp(token, "SOLN", 4) == 0){
			printf("SOlution Found\r\n");
		} else if(strncmp(token, "WORK", 4) == 0){
			printf("Work Found\r\n");
		} else if(strncmp(token, "ABRT", 4) == 0){
			printf("ABRT Found\r\n");
		} else {
			printf("ERRO Message not correct\r\n");
		}
		token = strtok(NULL, "\r\n");
		fflush(stdout);
    }
	return 0;
}
