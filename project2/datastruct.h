/****************************************************************************
* Code written by Ashkan Habibi
* Student ID: 758744
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "uint256.h"

#ifndef DATASTRCUT_H
#define DATASTRCUT_H

typedef struct{
    uint32_t difficulty; //the difficulty
	uint32_t alpha; //the first byte of diffuclty
    BYTE beta[32]; // the next 3 bytes of diffuclty
	BYTE target[32]; //the target for a proof of work
    BYTE seed[32];
    BYTE start_nonce[32];
    int client_sock_id; // the socket fd of the client,
	int abrt; //abrt flag
	int work_no;
} work_t;

#endif
