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
#include <stdint.h>
#include <inttypes.h>
//#include "uint256.h"
#include "sha256.h"
#include <assert.h>
#include <pthread.h>
#include "list.h"
//#include "datastruct.h"

#define TRUE 1
#define FALSE 0
#define MAX_ERROR 40
#define MAX_CLIENT 100
#define MAX_BUFFER 256
#define PONG "PONG"
#define OKAY "OKAY"
#define PING "PING"
#define LOG "log.txt"
#define MAX_WORK 10

list_t *work_queue;


int buffer_analyser(char *buffer,int buffer_size, int socket_id, FILE *log);
FILE* logger(FILE *fp, int to_do);
int SOLN_parser(char *line);
void print_uint26(BYTE *uint256, size_t length);
void uint_init(BYTE *uint320, size_t length);
int hash_checker(BYTE *solution, BYTE *target);
void WORK_parser(char *line, int client_sock);
void send_client_Message(work_t work);
void *work_func();

int main(int argc, char **argv){
	FILE *log;
	int opt = TRUE;
	int listener_socket = -1, clilen = -1, new_sockfd = -1, client_socks[MAX_CLIENT],
	activity = 0, value_read = -1, i = 0, server_portno, max_sd = 0;

	char buffer[MAX_BUFFER];
	char ERROR[MAX_ERROR];
	struct sockaddr_in serv_addr, cli_addr;

	fd_set client_read_fds; //set of sockets descriptors
	log = fopen(LOG, "w");
	fclose(log);
	//initialise all client sockets to 0 so not checked
	for(i = 0; i < MAX_CLIENT; i++){
		client_socks[i] = 0;
	}
	//receiving the port number for server from user
	if (argc < 2){
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	work_queue = make_empty_list();
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
			log = logger(log, 1);
			//printf("1\n");
			fprintf(log, "New Connection, Socket fd: %d , client IP:  %s\n" , new_sockfd , inet_ntoa(cli_addr.sin_addr));
			//printf("2\n");
			log = logger(log, 0);

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
					log = logger(log,1);
					///printf("3\n");
					fprintf(log, "Client socket: %d disconnected , Client IP: %s\n" ,client_socks[i], inet_ntoa(cli_addr.sin_addr));
					//printf("4\n");
					log = logger(log, 0);

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
					log = logger(log, 1);
					//printf("5\n");
					fprintf(log, "Socket fd: %d , client IP: %s\n" , client_socks[i] , inet_ntoa(cli_addr.sin_addr));
					//printf("6\n");
					buffer_analyser(buffer, value_read, client_socks[i], log);
					log = logger(log, 0);
					//buffer_analyser(buffer, value_read, client_socks[i], log);
					//value_read = write(client_socks[i], buffer, value_read);
				}
			}
		}
	}

	return 0;
}

int buffer_analyser(char *buffer, int buffer_size, int sock_id, FILE *log){
	int value_read = 0;
	//int i = 0;
	char *token;
	//char message[5];
	//memset(message, '\0', 5);
	/*for(i = 0; i < buffer_size; i++){
		if(buffer[i] == '\r'){
			printf("Found CR || ");
		}
		if(buffer[i] == '\n'){
			printf("found NL\n");
		}
	}*/
    //const char delim[3] = "\r\n";
    token = strtok(buffer, "\r\n");
    while(token){
        //strncpy(token, token, 4);
		//printf("token: %s && size: %d\n",token, strlen(token));
		//printf("msg: %s\n",msg);

        if((strncmp(token, PING, 4) == 0) && (strlen(token) == 4)){
			value_read = write(sock_id, "PONG\r\n", 6);
			fprintf(log, "SSTP message: %s\n", token);
			printf("token1\n");

		} else if ((strncmp(token, PONG, 4) == 0) && (strlen(token) == 4)){
			printf("token2\n");
			value_read = write(sock_id, "ERRO Only Server can send PONG\r\n", 32);
			fprintf(log, "SSTP message: %s\n", token);

		} else if ((strncmp(token, OKAY, 4) == 0) && (strlen(token) == 4)){
			printf("token3\n");
			value_read = write(sock_id, "ERRO Only Sever can send OKAY\r\n", 31);
			fprintf(log, "SSTP message: %s\n", token);

		} else if ((strncmp(token, "ERRO", 4) == 0) && (strlen(token) == 4)){
			printf("token4\n");
            value_read = write(sock_id, "ERRO must not be sent to the server\r\n", 37);
			fprintf(log, "SSTP message: %s\n", token);

		} else if(strncmp(token, "SOLN", 4) == 0){
			fprintf(log, "SSTP message: %s\n", token);
			if(strlen(token) <= 5){
				printf("worng message\n");
				value_read = write(sock_id, "ERRO message is not complete\r\n", 30);
				token = strtok(NULL, "\r\n");
				continue;
			}
			//printf("SOlution Found\r\n");
			if(SOLN_parser(token)){
				value_read = write(sock_id, "OKAY\r\n", 6);
			} else {
				value_read = write(sock_id, "ERRO The proof of work was incorrect\r\n", 38);
			}
			//fprintf(log, "SSTP message: %s\n", token);

		} else if(strncmp(token, "WORK", 4) == 0){
			fprintf(log, "SSTP message: %s\n", token);
			//printf("Work Found\r\n");
			if(strlen(token) <= 5){
				printf("worng message\n");
				value_read = write(sock_id, "ERRO message is not complete\r\n", 30);
				token = strtok(NULL, "\r\n");
				continue;
			}
			if(work_queue->foot && work_queue->foot->queue_no >= 10){
				write(sock_id, "ERRO work_queue is full\r\n",strlen("ERRO work_queue is full\r\n"));
			}else {
				WORK_parser(token, sock_id);
			}

		} else if(strncmp(token, "ABRT", 4) == 0){
			fprintf(log, "SSTP message: %s\n", token);
			printf("ABRT Found\r\n");
			value_read = write(sock_id, "OKAY\r\n", 6);

		} else {
			fprintf(log, "SSTP message: %s\n", token);
			value_read = write(sock_id, "ERRO Message was not correct\r\n", 30);
			printf("ERRO Message not correct\r\n");
		}
		token = strtok(NULL, "\r\n");
		fflush(stdout);
    }
	return 0;
}


FILE* logger(FILE *fp, int to_do){
	//printf("Hello\n");
	if(to_do){
		//open file to write
		//printf("hel\n");
		fp = fopen(LOG, "a");
		//printf("1\n");
		if(!fp){
			printf("Couldn't log\n");
			exit(EXIT_FAILURE);
		}
		//printf("56\n");
		time_t rawtime;
		struct tm *info;
		//char buffer[80];

		time( &rawtime );
		info = localtime( &rawtime );
		fprintf(fp,"Time: %s", asctime(info));
	} else {
		fprintf(fp,"\n-----------------------------\n\n");
		fclose(fp);
	}
	return fp;
}


int SOLN_parser(char *line){
	uint32_t diff;
	char *lineStart = line + 5; // where the difficulty starts

	//reading in the difficulty
	sscanf(lineStart, "%x\n", &diff);
	//printf("Number: %" PRIu32 "\n",difficulty);

	//getting alpha
	uint32_t copy = diff;
	uint32_t alpha = (copy >> 24); //getting the first 8 bits
	//printf("alpha; %d\n\n", alpha);


	/*BYTE alpha1[1];
	sscanf(lineStart, "%2hhx", &alpha1[0]);
	print_uint2(alpha1);*/

	//getting beta
	BYTE beta1[32];
	uint256_init(beta1);
	lineStart += 2;
	int j = 29;
	while(j < 32){
		sscanf(lineStart, "%2hhx", &beta1[j]);
		lineStart += 2;
		j++;
	}
	/*printf("beta: ");
	print_uint25(beta1);
	printf("\n");*/

	//target calculation
	BYTE base[32];
	uint256_init(base);

	base[31] = 0x2;

	BYTE res[32];
	uint256_init(res);
	uint32_t expo = 8*(alpha - 3);
	uint256_exp(res, base, expo);
	//print_uint256(res);

	BYTE target[32];
	uint256_init(target);

	uint256_mul(target, beta1, res);
	/*printf("target: ");
	//print_uint256(target);
	printf("\n");*/


	//seed parsing
	lineStart = line + 14; //where the seed starts
	BYTE seed[32];
	int i = 0;
	while(i < 32){
		sscanf(lineStart, "%2hhx", &seed[i]);
		lineStart += 2;
		i++;
	}
	//print_uint256(seed);

	//getting the nonce for the prrof of work
	uint64_t nonce;
	sscanf((lineStart+1), "%lx", &nonce);
	//printf("NONCE: %lx \n",nonce);

	BYTE NONCE[32];
	uint256_init(NONCE);
	//uint_init(NONCE, 8);
	i = 24;
	lineStart += 1;
	while(i < 32){
		sscanf(lineStart, "%2hhx", &NONCE[i]);
		lineStart += 2;
		i++;
	}
	//print_uint256(NONCE);

	//concatenation of seed and nonce
	BYTE solution[40];
	uint_init(solution, 40);
	for(i = 0; i < 40; i++){
		if(i < 32){
			solution[i] = seed[i];
		} else {
			solution[i] = NONCE[i - 8];
		}
	}
	//printf("solution: \n");
	//print_uint26(solution, 40);

	return hash_checker(solution, target);
}

int hash_checker(BYTE *solution, BYTE *target){
	BYTE hash1[32];
	uint256_init(hash1);

	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, solution, 40);
	sha256_final(&ctx, hash1);
	//printf("1st hash: \n");
	//print_uint256(hash1);

	BYTE hash2[32];
	uint256_init(hash2);
	sha256_init(&ctx);
	sha256_update(&ctx, hash1, 32);
	sha256_final(&ctx, hash2);

	//print_uint256(target);
	//print_uint256(hash2);
	if(sha256_compare(target, hash2) == 1){
		//printf("\nhooray!\n");
		return 1;
	} else {
		//printf("nope\n");
		return 0;
	}
}
void print_uint26 (BYTE *uint256, size_t length) {
    printf ("0x");
    size_t i = 0;
    for (i = 0; i < length; i++) {
        printf ("%02x", uint256[i]);
    }
    printf("\n");
}

void uint_init(BYTE *uint320, size_t length){
	if (uint320 == NULL) {
        return;
    }

	size_t i = 0;
	for (i = 0; i < length; i++){
		uint320[i] = 0;
	}
}

void WORK_parser(char *line, int client_sock){
	work_t *work = NULL;
	work = (void *) malloc (sizeof(work_t));
	assert(work != NULL);

	char *lineStart = line + 5; // where the difficulty starts
	sscanf(lineStart, "%x", &(work->difficulty));
	//printf("Difficulty: %x\n", work->difficulty);

	work->alpha = work->difficulty >> 24;
	//printf("Difficulty: %x\n", work->alpha);

	//beta processing
	uint256_init(work->beta);
	int i = 29;
	lineStart += 2;
	while(i < 32){
		sscanf(lineStart, "%2hhx", &(work->beta[i]));
		i++;
		lineStart += 2;
	}
	//printf("beta: \n");
	//print_uint256(work->beta);

	//processing target
	BYTE base[32];
	uint256_init(base);
	base[31] = 0x2;

	BYTE res[32];
	uint256_init(res);
	uint32_t expo = 8*(work->alpha - 3);
	uint256_exp(res, base, expo);

	uint256_init(work->target);
	uint256_mul(work->target, work->beta, res);
	//printf("target: \n");
	//print_uint256(work->target);

	//seed processing
	uint256_init(work->seed);
	i = 0;
	lineStart = 14 + line;
	while(i < 32){
		sscanf(lineStart, "%2hhx", &(work->seed[i]));
		lineStart += 2;
		i++;
	}
	//printf("seed: \n");
	//print_uint256(work->seed);

	//processing nonce
	uint256_init(work->start_nonce);
	i = 24;
	lineStart += 1;
	while(i < 32){
		sscanf(lineStart, "%2hhx", &(work->start_nonce[i]));
		lineStart += 2;
		i++;
	}
	//printf("nonce: \n");
	//print_uint256(work->start_nonce);

	//getting number of threads on the work
	lineStart++;
	sscanf(lineStart, "%d", &work->work_no);
	//printf("work no: %d\n", work->work_no);
	work->client_sock_id = client_sock;
	work->abrt = 0;
	insert_at_foot(work_queue, *work);
	update_queue_no(work_queue);
	//printf("1\n");
	work_func();
	//update_queue_no(work_queue);
}


void *work_func(){
	assert(work_queue != NULL);
    BYTE add_1[32];
	uint256_init(add_1);
	add_1[31] = 0x1;
	//printf("2\n");
	BYTE solution[40];
	bzero(solution,40);

	work_t work;
	int sol_found = 0, i = 0;
	int j = 0;
	while(j < 10){
		//printf("3\n");
		j++;
		if(!is_empty_list(work_queue)){
			work = get_head(work_queue);

			if(work.abrt){
				//free(work);
				update_queue_no(work_queue);
				continue;
			}
			uint256_init(solution);

			sol_found = 0;
			for(i = 0; i < 32; i++){
				solution[i] = work.seed[i];
			}

			while(!sol_found){
				if(work_queue->head->data.abrt){
					work_queue = clear_queue(work_queue, work.client_sock_id);
					//work = NULL;
					break;
				}
				for(i = 32; i < 40; i++){
					solution[i] = work.start_nonce[i - 8];
				}
				if(hash_checker(solution, work.target)){
					send_client_Message(work);
					sol_found = 1;
					//printf("It did work\n");
					//print_uint256(work.start_nonce);
					//work_queue = clear_queue(work_queue);
					work_queue = get_tail(work_queue);
					update_queue_no(work_queue);
				} else {
					uint256_add(work.start_nonce, add_1, work.start_nonce);
				}
			}
		}
	}
}


void send_client_Message(work_t work){
	//assert(work != NULL);
	if(work.abrt){
		work_queue = clear_queue(work_queue, work.client_sock_id);
		//work = NULL;
		return;
	}
	BYTE MESSAGE[98];
	memset(MESSAGE, '\0', 98);
	BYTE *msg_ptr = MESSAGE;
	strncpy(msg_ptr, "SOLN ", strlen("SOLN "));
	msg_ptr += strlen("SOLN ");
	//write(work.client_sock_id, "IT does work!!!\r\n", 17);
	///write(work.client_sock_id, );
	sprintf(msg_ptr, "%x ", work.difficulty);
	msg_ptr += 9;
	int i = 0;
	while(i < 32){
		sprintf(msg_ptr, "%02x", work.seed[i]);
		i++;
		msg_ptr += 2;
	}
	//printf("char: %c", *msg_ptr);
	//msg_ptr += 1;
	*msg_ptr = ' ';
	msg_ptr++;
	i = 24;
	while(i < 32){
		sprintf(msg_ptr, "%02x", work.start_nonce[i]);
		i++;
		msg_ptr += 2;
	}
	*msg_ptr = '\r';
	*(msg_ptr + 1) = '\n';
	//MESSAGE[95] = '\r';
	//MESSAGE[96] = '\n';
	//printf("Message: %s", MESSAGE);
	write(work.client_sock_id, MESSAGE, strlen(MESSAGE));
}
