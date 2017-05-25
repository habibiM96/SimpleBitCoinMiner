/* Project 2,
By Ashkan Habibi,
student ID: 758744
I used GeeksForGeeks multi client server as a reference
*/

/******************************* libraries ***********************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdint.h>
#include <inttypes.h>
#include "sha256.h"
#include <assert.h>
#include <pthread.h>
#include "list.h"

/******************************* CONSTANTS ***********************/
#define TRUE 1
#define SOLN_LENGTH 98
#define FALSE 0
#define UINT256 32
#define HEADER_LEN 5
#define MAX_ERROR 40
#define MAX_CLIENT 100
#define MAX_BUFFER 256
#define MAX_WORK 10
#define PONG "PONG\r\n"
#define OKAY "OKAY\r\n"
#define PING "PING"
#define ABRT "ABRT"
#define SOLN "SOLN"
#define WORK "WORK"
#define LOG "log.txt"
#define CRLF "\r\n"


/******************************* Global Queue ***********************/
//the work message queue that is used by the work thread.
//Holds up to 11 works, 1 working on adn 10 waiting

// I apologise for the offensive use of a global variable,
//I couldn't come up with a way to share the work with both
//threads real-time and constantly
list_t *work_queue;


/******************************* FUnction Prototypes ***********************/
int buffer_analyser(char *buffer,int buffer_size, int socket_id, FILE *log);
FILE* logger(FILE *fp, int to_do);
int SOLN_parser(char *line);
void uint_init(BYTE *uint320, size_t length);
int hash_checker(BYTE *solution, BYTE *target);
void WORK_parser(char *line, int client_sock);
void send_client_Message(work_t work);
void *work_func();
int crlfConfirm(char *msg);
void registerBuffer(buffer_t *buffers, int client_sock);
void deregisterBuffer(buffer_t *buffers, int client_sock);
void doCache(buffer_t *buffers, int client_sock, char *buffer, FILE *fp);

/******************************* Functions ***********************/
int main(int argc, char **argv){
	pthread_t tid; // work thread ID
	FILE *log;
	int opt = TRUE;
	int listener_socket = -1, clilen = -1, new_sockfd = -1, client_socks[MAX_CLIENT],
	activity = 0, value_read = -1, i = 0, server_portno, max_sd = 0;
	int client_count = 0;
	buffer_t buffers[MAX_CLIENT];
	char buffer[MAX_BUFFER]; //buffer where  clients write their message in
	struct sockaddr_in serv_addr, cli_addr;

	fd_set client_read_fds; //set of sockets descriptors
	log = fopen(LOG, "w");//creating and erasing the log file
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
	for(i = 0; i < MAX_CLIENT; i++){
		buffers[i].client_id = -1;
		memset(buffers[i].buff, '\0', MAX_BUFFER);
	}
	//spinning up the work thread
	pthread_create(&tid, NULL, work_func, NULL);

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
			if ((new_sockfd = accept( listener_socket,
					(struct sockaddr *) &cli_addr, &clilen)) < 0){
				perror("ERROR on accept");
				exit(EXIT_FAILURE);
			}
			log = logger(log, 1);
			fprintf(log, "New Connection, Socket fd: %d , client IP:  %s\n" ,
					new_sockfd , inet_ntoa(cli_addr.sin_addr));
			log = logger(log, 0);
			registerBuffer(buffers, new_sockfd);
			for(i = 0; i < MAX_CLIENT; i++){
				//adding the new socket to the list
				if(client_socks[i] == 0){
					//found an empty one
					client_socks[i] = new_sockfd;
					break;
				}
			}
		}

		bzero(buffer,MAX_BUFFER);//initialising the buffer before reading anything from it

		for(i = 0; i < MAX_CLIENT; i++){

			if (FD_ISSET(client_socks[i], &client_read_fds)){
				//check for any incoming message
				value_read = read(client_socks[i], buffer, MAX_BUFFER - 1);
				if( value_read == 0){
					//client disconnected
					printf("closing client \n");

					log = logger(log,1);
					fprintf(log, "Client socket: %d disconnected , Client IP: %s\n"
								,client_socks[i], inet_ntoa(cli_addr.sin_addr));
					log = logger(log, 0);
					deregisterBuffer(buffers, client_socks[i]);
					set_work_abrt(work_queue, client_socks[i]);
					close(client_socks[i]);
					client_socks[i] = 0;

				} else if (value_read < 0){
					//could not read anything from the buffer
					perror("ERROR reading from socket");
					exit(EXIT_FAILURE);

				} else {
					//received a message, will respond according to the protocol
					buffer[value_read] = '\0';
					printf("received message\n");
					log = logger(log, 1);
					fprintf(log, "Socket fd: %d , client IP: %s\n" ,
					client_socks[i] , inet_ntoa(cli_addr.sin_addr));
					doCache(buffers, client_socks[i], buffer, log);
					//buffer_analyser(buffer, value_read, client_socks[i], log);
					log = logger(log, 0);
				}
			}
		}
	}

	return 0;
}


/*
* buffer analyser tokenises a message based on CRLF (carriage return and newline),
* and will respond will the appropriate messages
*/
int buffer_analyser(char *buffer, int buffer_size, int sock_id, FILE *log){
	int value_read = 0, length_p = 0;
	BYTE ERRO [MAX_ERROR];
	memset(ERRO, '\0', MAX_ERROR);
	if(!crlfConfirm(buffer)){
		fprintf(log, "SSTP message: %s\n", buffer);
		strncpy(ERRO, "ERRO message did not end with crlf\r\n",
				strlen("ERRO message did not end with crlf\r\n"));

		value_read = write(sock_id, ERRO, strlen(ERRO));
		return 0;
	}
	/*i keep a unchanged copy of the buffer, because strtok changes the
	* entire buffer*/
	char bufferCPY[MAX_BUFFER], *cpy;
	strncpy(bufferCPY, buffer, strlen(buffer));
	cpy = &(bufferCPY);

	char *token;
    token = strtok(buffer, CRLF);
    while(token){

        if((strncmp(token, PING, strlen(PING)) == 0) && (strlen(token) == strlen(PING))){
			//received a PING, respond with a PONG
			value_read = write(sock_id, PONG, strlen(PONG));
			fprintf(log, "SSTP message: %s\n", token);

		} else if ((strncmp(token, PONG, strlen(PING)) == 0) && (strlen(token) == strlen(PING))){
			//received a PONG, reply with an error
			strncpy(ERRO, "ERRO Only Server can send PONG\r\n",
				strlen("ERRO Only Server can send PONG\r\n"));

			value_read = write(sock_id, ERRO, strlen(ERRO));
			fprintf(log, "SSTP message: %s\n", token);

		} else if ((strncmp(token, OKAY, strlen(PING)) == 0) && (strlen(token) == strlen(PING))){
			//received an OKAY, will respond with an Error
			strncpy(ERRO, "ERRO Only Server can send OKAY\r\n",
					strlen("ERRO Only Server can send OKAY\r\n"));

			value_read = write(sock_id, ERRO, strlen(ERRO));
			fprintf(log, "SSTP message: %s\n", token);

		} else if ((strncmp(token, "ERRO", strlen(PING)) == 0) && (strlen(token) == strlen(PING))){
			//received an ERRO, will respond with an Error
			strncpy(ERRO, "ERRO Only Server can send ERRO\r\n",
					strlen("ERRO Only Server can send ERRO\r\n"));

			value_read = write(sock_id, ERRO, strlen(ERRO));
			fprintf(log, "SSTP message: %s\n", token);

		} else if(strncmp(token, SOLN, strlen(PING)) == 0){
			//received an SOLN, will respond with with a proof of work verification
			fprintf(log, "SSTP message: %s\n", token);
			if(strlen(token) <= (SOLN_LENGTH - 4)){
				//incomplete SOLN message
				strncpy(ERRO, "ERRO incomplete SOLN message\r\n",
						strlen("ERRO incomplete SOLN message\r\n"));

				value_read = write(sock_id, ERRO, strlen(ERRO));
				token = strtok(NULL, CRLF);
				continue;
			}

			if(SOLN_parser(token) == 1){
				//parse solution message and verify it
				value_read = write(sock_id, OKAY, strlen(OKAY));

			} else {
				//wrong proof of work
				strncpy(ERRO, "ERRO The proof of work was incorrect\r\n",
						strlen("ERRO The proof of work was incorrect\r\n"));

				value_read = write(sock_id, ERRO, strlen(ERRO));
			}

		} else if(strncmp(token, WORK, strlen(PING)) == 0){
			//received an WORK, will respond with with a proof of work
			fprintf(log, "SSTP message: %s\n", token);
			if(strlen(token) <= (SOLN_LENGTH - 4)){
				//work message is incomplete
				strncpy(ERRO, "ERRO incomplete WORK message\r\n",
						strlen("ERRO incomplete WORK message\r\n"));
				value_read = write(sock_id, ERRO, strlen(ERRO));
				token = strtok(NULL, CRLF);
				continue;
			}

			if(work_queue->foot && work_queue->foot->queue_no >= MAX_WORK){
				//queue is at full capacity
				strncpy(ERRO, "ERRO work_queue is full\r\n", strlen("ERRO work_queue is full\r\n"));
				value_read = write(sock_id, ERRO, strlen(ERRO));
			}else {
				//have room for the work message, parse and add it to the queue
				WORK_parser(token, sock_id);
			}

		} else if(strncmp(token, ABRT, strlen(ABRT)) == 0){
			//client requests abort for all its work messages
			fprintf(log, "SSTP message: %s\n", token);
			value_read = write(sock_id, OKAY, strlen(OKAY));
			set_work_abrt(work_queue, sock_id);

		} else {
			//all other messages are of the wrong type
			fprintf(log, "SSTP message: %s\n", token);
			strncpy(ERRO, "ERRO message was not recognised\r\n",
					strlen("ERRO message was not recognised\r\n"));

			value_read = write(sock_id, ERRO, strlen(ERRO));
		}
		token = strtok(NULL, CRLF);
		fflush(stdout);

		if(token){
			//if more tokens, make sure they end with the delimiter, do this check on the original unchanged message
			length_p += strlen(token) + 2;
			if(length_p < (strlen(bufferCPY) - 2) && !crlfConfirm(cpy + length_p)){
				fprintf(log, "SSTP message: %s\n", buffer);
				strncpy(ERRO, "ERRO message did not end with crlf\r\n",
						strlen("ERRO message did not end with crlf\r\n"));

				value_read = write(sock_id, ERRO, strlen(ERRO));
				return 0;
			}
		}

    }
	return 0;
}


FILE* logger(FILE *fp, int to_do){
	if(to_do){
		//open file to write
		fp = fopen(LOG, "a");
		if(!fp){
			printf("Couldn't log\n");
			exit(EXIT_FAILURE);
		}
		time_t rawtime;
		struct tm *info;

		time( &rawtime );
		info = localtime( &rawtime );
		fprintf(fp,"Time: %s", asctime(info));
	} else {
		//close the file
		fprintf(fp,"\n-----------------------------\n\n");
		fclose(fp);
	}
	return fp;
}



/*
* SOLN_parser takes a solution message as a string and extracts difficulty,
* seed and the nonce, and will verifies the soluton/ proof of work through hash_checker
* and returns a boolean depending on the correctness of the proof of work
*/
int SOLN_parser(char *line){
	uint32_t diff;
	char *lineStart = line + HEADER_LEN; // where the difficulty starts
	sscanf(lineStart, "%x\n", &diff);

	//getting alpha
	uint32_t copy = diff;
	uint32_t alpha = (copy >> 24); //getting the first 8 bits

	//getting beta
	BYTE beta1[UINT256];
	uint256_init(beta1);

	 //skipping the first byte of the difficulty to the where beta starts
	lineStart += 2;
	int j = 29;//beta only occupies the last bytes of the array, i.e 29, 30, 31
	while(j < UINT256){
		sscanf(lineStart, "%2hhx", &beta1[j]);
		lineStart += 2;
		j++;
	}

	/*target calculation*/

	//making a base of 2
	BYTE base[UINT256];
	uint256_init(base);
	base[31] = 0x2;

	BYTE res[UINT256];
	uint256_init(res);

	//the given formula uses 8 and 3
	uint32_t expo = 8*(alpha - 3);
	uint256_exp(res, base, expo);

	BYTE target[UINT256];
	uint256_init(target);

	uint256_mul(target, beta1, res);

	//seed parsing
	lineStart = line + 14; //where the seed starts
	BYTE seed[UINT256];
	int i = 0;
	while(i < UINT256){
		sscanf(lineStart, "%2hhx", &seed[i]);
		lineStart += 2;
		i++;
	}

	//getting the nonce for the prrof of work
	BYTE NONCE[UINT256];
	uint256_init(NONCE);
	i = 24;//nonce is only 8 bytes long, so it only occupies from byte 24-31
	lineStart += 1;
	while(i < UINT256){
		sscanf(lineStart, "%2hhx", &NONCE[i]);
		lineStart += 2;
		i++;
	}


	//concatenation of seed and nonce
	//40 is the length of the seed (32) + length of the nonce (8)
	BYTE solution[40];
	uint_init(solution, 40);
	for(i = 0; i < 40; i++){
		if(i < UINT256){
			//the first 32 cells are occupised by the seed
			solution[i] = seed[i];
		} else {
			//the last 8 bytes are occupised by the nonce
			solution[i] = NONCE[i - 8];
		}
	}

	return hash_checker(solution, target);
}

/*
* checks a proof of work using sha256
*/
int hash_checker(BYTE *solution, BYTE *target){
	BYTE hash1[UINT256];
	uint256_init(hash1);

	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, solution, 40);
	sha256_final(&ctx, hash1);

	BYTE hash2[32];
	uint256_init(hash2);
	sha256_init(&ctx);
	sha256_update(&ctx, hash1, 32);
	sha256_final(&ctx, hash2);

	if(sha256_compare(target, hash2) == 1){
		return 1;
	} else {
		return 0;
	}
}

/*
* initialises a byte array of specified size
*/
void uint_init(BYTE *uint320, size_t length){
	if (uint320 == NULL) {
        return;
    }

	size_t i = 0;
	for (i = 0; i < length; i++){
		uint320[i] = 0;
	}
}

/*
* WORK_parser works almost the same way as SOLN_parser works,
* However, once it parse a work it adds it to the queue for work thread to work on it.
* I tried making a general purpose function but it kept failing and due to lack of time had to
* rely on two different function to do the job
*/
void WORK_parser(char *line, int client_sock){
	work_t *work = NULL;
	work = (void *) malloc (sizeof(work_t));
	assert(work != NULL);

	char *lineStart = line + HEADER_LEN; // where the difficulty starts
	sscanf(lineStart, "%x", &(work->difficulty));

	work->alpha = work->difficulty >> 24;

	//beta processing
	uint256_init(work->beta);
	int i = 29; //beta only occupies the last bytes of the array, i.e 29, 30, 31
	lineStart += 2; //skipping the first byte of diffciulty
	while(i < UINT256){
		sscanf(lineStart, "%2hhx", &(work->beta[i]));
		i++;
		lineStart += 2;
	}

	/*processing target*/

	BYTE base[UINT256];
	uint256_init(base);
	base[31] = 0x2;

	BYTE res[UINT256];
	uint256_init(res);
	//the formula given from the specs
	uint32_t expo = 8*(work->alpha - 3);
	uint256_exp(res, base, expo);

	uint256_init(work->target);
	uint256_mul(work->target, work->beta, res);

	//seed processing
	uint256_init(work->seed);
	i = 0;
	//where the seed starts from the beginning of the message line,
	//skipping the previous 14 characters in the message
	lineStart = 14 + line;
	while(i < UINT256){
		sscanf(lineStart, "%2hhx", &(work->seed[i]));
		lineStart += 2;
		i++;
	}
	//printf("seed: \n");
	//print_uint256(work->seed);

	//processing nonce
	uint256_init(work->start_nonce);
	i = 24; //nonce only occupies the last 8 bytes of the array, i.e 24-31
	lineStart += 1;
	while(i < UINT256){
		sscanf(lineStart, "%2hhx", &(work->start_nonce[i]));
		lineStart += 2;
		i++;
	}

	//getting number of threads on the work
	lineStart++;
	sscanf(lineStart, "%d", &work->work_no);

	work->client_sock_id = client_sock;
	work->abrt = 0;

	//add to the queue, update the number of the works in the queue
	insert_at_foot(work_queue, *work);
	update_queue_no(work_queue);
}


/*
* this is the function that is passed to the thread,
* it constantly checks for new works  to process and it runs as long as the server is running
*/
void *work_func(){
	assert(work_queue != NULL);

	//increment the nonce by 1 every time
    BYTE add_1[32];
	uint256_init(add_1);
	add_1[31] = 0x1;

	//concatenation of seed + nonce, 32 of seed + 8 bytes of nonce
	BYTE solution[40];
	bzero(solution,40);

	work_t work;
	int sol_found = 0, i = 0;

	while(TRUE){
		//keeps the thread running waiting for new works to come.
		//ends when server shuts off

		if(!is_empty_list(work_queue)){
			//new work in the queue
			work = get_head(work_queue);

			if(work.abrt){
				//checks if the client has aborted the work or disconnected
				//removes the aborted work
				work_queue = get_tail(work_queue);
				continue;
			}
			uint256_init(solution);

			sol_found = 0;
			for(i = 0; i < UINT256; i++){
				//initialise the solution with the 32 bytes of seed
				solution[i] = work.seed[i];
			}

			while(!sol_found){
				//while a solution is not found, increment the nonce to find one
				if(work_queue->head->data.abrt){
					//client has aborted this work
					break;
				}
				for(i = UINT256; i < 40; i++){
					//fill the last 8 bytes of the solution with the new incremented nonce
					solution[i] = work.start_nonce[i - 8];
				}

				if(hash_checker(solution, work.target)){
					//if a proof of work is found, send a message to the client
					//update the queue
					send_client_Message(work);
					sol_found = 1;
					work_queue = get_tail(work_queue);
					update_queue_no(work_queue);
				} else {
					uint256_add(work.start_nonce, add_1, work.start_nonce);
				}
			}
		}
	}
}


/*
* sends a message to the client with the correct proof of work
*/
void send_client_Message(work_t work){
	//assert(work != NULL);
	usleep(50);
	if(work.abrt){
		//checks if the client hasn't aborted or disconnected
		work_queue = clear_queue(work_queue, work.client_sock_id);
		return;
	}

	BYTE MESSAGE[SOLN_LENGTH];
	memset(MESSAGE, '\0', SOLN_LENGTH);
	BYTE *msg_ptr = MESSAGE;

	//writing the message header
	strncpy(msg_ptr, SOLN, strlen(SOLN));
	msg_ptr += strlen(SOLN);

	strncpy(msg_ptr, " ", strlen(" "));
	msg_ptr += strlen(" ");

	//write the difficulty
	sprintf(msg_ptr, "%x ", work.difficulty);
	msg_ptr += 9; // 9 = length of difficulty + space

	//writing the seed
	int i = 0;
	while(i < UINT256){
		sprintf(msg_ptr, "%02x", work.seed[i]);
		i++;
		msg_ptr += 2;
	}

	//writing a space between seed and nonce
	*msg_ptr = ' ';
	msg_ptr++;

	//writing nonce
	i = 24; // nonce only occupies 8 bytes, i.e cells 24-31
	while(i < UINT256){
		sprintf(msg_ptr, "%02x", work.start_nonce[i]);
		i++;
		msg_ptr += 2;
	}

	//writing crlf
	*msg_ptr = '\r';
	*(msg_ptr + 1) = '\n';
	if(work.abrt){
		//checks if the client hasn't aborted or disconnected
		work_queue = clear_queue(work_queue, work.client_sock_id);
		return;
	}
	write(work.client_sock_id, MESSAGE, strlen(MESSAGE));
}

int crlfConfirm(char *msg){
	//char *ch = msg;
	char *crPos = NULL, *nlPos = NULL;
	crPos = memchr(msg, '\r', strlen(msg));
	nlPos = memchr(msg, '\n', strlen(msg));

	if(crPos != NULL && nlPos != NULL){
		if(crPos == (nlPos - 1)){
			return 1;
		}
	}
	return 0;
}

void registerBuffer(buffer_t *buffers, int client_sock){
	int i = 0;
	for(i = 0; i < MAX_CLIENT; i++){
		if(buffers[i].client_id < 0){
			buffers[i].client_id = client_sock;
			memset(buffers[i].buff, '\0', MAX_BUFFER);
			break;
		}
	}
}

void deregisterBuffer(buffer_t *buffers, int client_sock){
	int i = 0;
	for(i = 0; i < MAX_CLIENT; i++){
		if(buffers[i].client_id  == client_sock){
			buffers[i].client_id = -1;
			memset(buffers[i].buff, '\0', MAX_BUFFER);
			break;
		}
	}
}

void doCache(buffer_t *buffers, int client_sock, char *buffer, FILE *fp){
	int i = 0, j = 0;

	for(i = 0; i < MAX_CLIENT; i++){
		if(buffers[i].client_id  == client_sock){
			int empty_size = MAX_BUFFER - strlen(buffers[i].buff);
			if(strlen(buffer) > (empty_size - 1)){
				strncat(buffers[i].buff, buffer, empty_size - 1);
			} else {
				strncat(buffers[i].buff, buffer, strlen(buffer));
			}
			break;
		}
	}
	if(crlfConfirm(buffers[i].buff)){
		buffer_analyser(buffers[i].buff, strlen(buffers[i].buff), buffers[i].client_id, fp);
		memset(buffers[i].buff, '\0', MAX_BUFFER);
	} else if(strlen(buffers[i].buff) == (MAX_BUFFER - 1)){
		buffer_analyser(buffers[i].buff, strlen(buffers[i].buff), buffers[i].client_id, fp);
		memset(buffers[i].buff, '\0', MAX_BUFFER);
	}
}
