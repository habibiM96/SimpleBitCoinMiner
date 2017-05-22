#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "uint256.h"

#ifndef DATASTRCUT_H
#define DATASTRCUT_H

typedef struct{
    uint32_t difficulty;
	uint32_t alpha;
    BYTE beta[32];
	BYTE target[32];
    BYTE seed[32];
    BYTE start_nonce[32];
    int client_sock_id;
	int abrt;
	int work_no;
} work_t;

#endif
