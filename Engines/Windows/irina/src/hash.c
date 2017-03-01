#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "protos.h"
#include "globals.h"
#include "hash.h"


Bitmap HASH_keys[64][16];
Bitmap HASH_ep[64];
Bitmap HASH_wk;
Bitmap HASH_wq;
Bitmap HASH_bk;
Bitmap HASH_bq;
Bitmap HASH_side;

Bitmap register_max;

HASH_reg * registers;

void init_registers(int max_mb)
{
    register_max = 1024L * 1024L * max_mb / sizeof (HASH_reg);
    registers = (HASH_reg *)calloc(register_max, sizeof(HASH_reg));
}

void set_hash(char * value)
{
    int mb;
    sscanf(value, "%d", &mb);
    if( mb >= 2 && mb <= 1024 ) {
        free(registers);
        init_registers(mb);
    }

}


void hash_save(int val)
{
    HASH_reg * x;
    x = &registers[board.hashkey%register_max];
    x->hashkey = board.hashkey;
    x->val = val;
}

bool hash_probe(int * val)
{
    HASH_reg * x;
    x = &registers[board.hashkey%register_max];
    if(x->hashkey == board.hashkey) {
        *val = x->val;
        if (repetitions() >= 3) *val = DRAWSCORE;
        return true;
    }
    return false;
}



Bitmap rand64()
{
    return rand() ^ ((Bitmap) rand() << 15) ^ ((Bitmap) rand() << 30) ^ ((Bitmap) rand() << 45) ^ ((Bitmap) rand() << 60);
}

void init_hash()
{
    int i, j;

    srand((unsigned int) get_ms());

    for (i = 0; i < 64; i++)
    {
        HASH_ep[i] = rand64();
        for (j = 0; j < 16; j++)
        {
            HASH_keys[i][j] = rand64();
        }
    }
    HASH_side = rand64();
    HASH_wk = rand64();
    HASH_wq = rand64();
    HASH_bk = rand64();
    HASH_bq = rand64();

    init_registers(HASH_DEFAULT);

}
