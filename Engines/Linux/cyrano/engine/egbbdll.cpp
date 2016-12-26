#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egbbdll.h"
#include "common.h"

static UBMP8 K_TR[64] = {
	0, 1, 2, 3, 3, 2, 1, 0,
	1, 4, 5, 6, 6, 5, 4, 1,
	2, 5, 7, 8, 8, 7, 5, 2,
	3, 6, 8, 9, 9, 8, 6, 3,
	3, 6, 8, 9, 9, 8, 6, 3,
	2, 5, 7, 8, 8, 7, 5, 2,
	1, 4, 5, 6, 6, 5, 4, 1,
	0, 1, 2, 3, 3, 2, 1, 0
};

static UBMP8 K1_TR[64] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	1, 8, 9,10,11,12,13,14,
	2, 9,15,16,17,18,19,20,
	3,10,16,21,22,23,24,25,
	4,11,17,22,26,27,28,29,
	5,12,18,23,27,30,31,32,
	6,13,19,24,28,31,33,34,
	7,14,20,25,29,32,34,35
};

static int KK_WP_index[4096];
static int KK_index[4096];
static int rotation[4096];

static const int rotF = 1;
static const int rotR = 2;
static const int rotD = 4;

static const int WIN           =   +1;
static const int DRAW          =    0;
static const int LOSS          =   -1;
static const int ILLEGAL       =   -2;
static const int DONT_KNOW     =   -3;
static const int WIN_SCORE     = 5000;

static const int VALUE[3] = {DRAW,WIN,LOSS};
enum {DECOMP_IN_RAM,DECOMP_IN_DISK,COMP_IN_RAM,COMP_IN_DISK};

#define is_in_disk(x)    ((x) & 1)
#define is_comp(x)       ((x) & 2)

static const char piece_name[] = "_kqrbnpkqrbnp_";

static SEARCHER searchers[8];
static LOCK searcher_lock;

/*
DECOMPRESSION
*/

#define _byte_1   0x00000000000000ff
#define _byte_all 0xffffffffffffffff
#define _byte_32  0xffffffff

#define addbits(x) {\
    while(x > length) {\
	   code = (code << 8) | *in++;\
	   length += 8;\
	}\
};

#define getbits(x,v) {\
    addbits(x)\
	length -= x;\
	v = UBMP32(code >> length) & (_byte_32 >> (32 - x));\
};

/*
Lempel Ziv
*/
static const int DISTANCE_BITS = 12;
static const int LENGTH_BITS = 8;
static const int PAIR_BITS = DISTANCE_BITS + LENGTH_BITS;
static const int LITERAL_BITS = 8;
static const int F_PAIR_BITS = PAIR_BITS + 1;
static const int F_LITERAL_BITS = LITERAL_BITS + 1;

static const int MIN_MATCH_LENGTH = (PAIR_BITS >> 3) + 1 + 1;
static const int MAX_MATCH_LENGTH = (1 << LENGTH_BITS) - 1 + MIN_MATCH_LENGTH;

static const int WINDOW_SIZE = (1 << DISTANCE_BITS);

static const int EOB_MARKER = (1 << LITERAL_BITS);

static const int LITERAL_CODES  = 513;
static const int DISTANCE_CODES =  24;

static const int extra_bits[DISTANCE_CODES] = {
	 0,  0,  0,  0,  1,  1,  2,  2,
	 3,  3,  4,  4,  5,  5,  6,  6,
	 7,  7,  8,  8,  9,  9, 10, 10
};

static const int base_dist[DISTANCE_CODES] = {
   0,     1,     2,     3,     4,     6,     8,    12,
  16,    24,    32,    48,    64,    96,   128,   192,
 256,   384,   512,   768,  1024,  1536,  2048,  3072
};

struct PAIR {
	int pos;
	int len;
	PAIR() {
		pos = 0;
		len = 0;
	}
};
/*
Huffman
*/
static const int    MAX_LEN  = 32;

struct CANN {
    int   symbol;
	UBMP32 code;
	UBMP32 mask;
	UBMP8  length;
	CANN() {
		symbol = INVALID;
		code = 0;
		length = 0;
	}
};

struct HUFFMAN {
	CANN*   cann;
	CANN*   pstart[MAX_LEN];
	UBMP8   min_length,
		    max_length;
	UBMP32  MAX_LEAFS;
	UBMP32  MAX_NODES;
	void build_cann_from_length();
};

void HUFFMAN::build_cann_from_length() {

	UBMP32 i,j;
	int temp;
	CANN tempcann;

	//sort by length
	for(i = 0;i < MAX_LEAFS;i++) {
		for(j = i;j < MAX_LEAFS;j++) {
			temp = cann[j].length - cann[i].length;
			if(temp == 0) {
				temp = cann[j].symbol - cann[i].symbol;
				temp = -temp;
			}
			if(temp < 0) {
				tempcann = cann[j];
				cann[j] = cann[i];
                cann[i] = tempcann;
			}
		}
	}

    //assign code
	UBMP32 code   = cann[MAX_LEAFS - 1].code;
	UBMP8  length = cann[MAX_LEAFS - 1].length;

	for(int k = MAX_LEAFS - 2; k >= 0; k--) {
        if (cann[k].length == 0) {
            break;
        }
		if(cann[k].length != length) {
			code >>= (length - cann[k].length);
			length = cann[k].length;
		}
		code++;
		cann[k].code = code;
    }

	//sort equal lengths lexically
	for(i = 0;i < MAX_LEAFS;i++) {
		for(j = i;j < MAX_LEAFS;j++) {
			temp = cann[j].length - cann[i].length;
			if(temp == 0) {
				temp = cann[j].symbol - cann[i].symbol;
			}
			if(temp < 0) {
				tempcann = cann[j];
				cann[j] = cann[i];
                cann[i] = tempcann;
			}
		}
	}

	//mark start of each length
    for (i = 0; i < MAX_LEN; i++) {
        pstart[i] = 0;
    }
	min_length = 32;
	max_length = 0;
	length = 0;
	for (i = 0; i < MAX_LEAFS; i++) {
        if (cann[i].length > length) {
			length = cann[i].length;
			pstart[length] = &cann[i];
			if(cann[i].length > max_length)
			    max_length = length;
			if(cann[i].length < min_length)
				min_length = cann[i].length;
        }
    }
}

/*
CACHE
*/
#define CACHE_HIT   1
#define CACHE_MISS  0

struct CACHE {
	INFO   info;
	CACHE*  prev;
    CACHE*  next;
	CACHE() {
		prev = 0;
		next = 0;
	}
};
struct LRU_CACHE {
	
	CACHE* head;
	CACHE* tail;
	LRU_CACHE* lru_prev;
	LRU_CACHE* lru_next;

	LOCK lock;

	static CACHE* cache;
	static LRU_CACHE* lru_head;
	static LRU_CACHE* lru_tail;
	static LOCK lock_lru;
	static UBMP32 size;
	static UBMP32 used;

	LRU_CACHE() {
		tail = 0;
		head = 0;
		lru_next = 0;
		lru_prev = 0;
		l_create(lock);
	}
	
	void add(INFO*);
	int  get(UBMP32,UBMP32,UBMP8&);
	void bring_to_front();
	static void alloc(UBMP32);
};

CACHE* LRU_CACHE::cache;
LRU_CACHE* LRU_CACHE::lru_head;
LRU_CACHE* LRU_CACHE::lru_tail;
LOCK LRU_CACHE::lock_lru;
UBMP32 LRU_CACHE::size;
UBMP32 LRU_CACHE::used;

void LRU_CACHE::alloc(UBMP32 tsize) {

	size  = tsize / sizeof(CACHE);
	cache = new CACHE[size];
	used  = 0;
	lru_head = 0;
	lru_tail = 0;
	l_create(lock_lru);
	printf("Cache Size = %d Mb (%d entries)\n",tsize / (1024 * 1024),size);
	fflush(stdout);
}

/*
brings currently accessed lru cache to the front
of the list after add/get operations
NB: make sure that any bucket lock is not acquired before calling
this to avoid deadlocks
*/

void LRU_CACHE::bring_to_front() {
    l_lock(lock_lru);

	if(lru_head) {
		if(lru_head != this) {
			if(lru_tail == this)
				lru_tail = lru_tail->lru_prev;
			
			if(lru_prev)
				lru_prev->lru_next = lru_next;
			if(lru_next)
				lru_next->lru_prev = lru_prev;
			lru_next = 0;
			lru_prev = 0;
			
			LRU_CACHE *lru_temp = lru_head;
			lru_head = this;
			lru_head->lru_next = lru_temp;
			lru_temp->lru_prev = lru_head;
		}
	} else {
		lru_tail = this;
		lru_head = this;
	}

	l_unlock(lock_lru);
}

void LRU_CACHE::add(INFO* info) {

	
	l_lock(lock_lru);
    if(used < size) {
		CACHE* temph,*freec = cache + used;
		used++;
		l_unlock(lock_lru);

		/*insert info at the free space*/
	    l_lock(lock);
        temph = head;
		head = freec;
		memcpy(&head->info,info,sizeof(INFO));
		head->next = temph;
		if(temph) {
		    temph->prev = head;
		} else {
			tail = head;
		}
		l_unlock(lock);

	} else { 
		CACHE *temph,*tempt;

		/*find a tail to replace*/
		LRU_CACHE* lru_target = lru_tail;
		while(lru_target) {
			l_lock(lru_target->lock);
			if(lru_target->head != lru_target->tail) {
				tempt = lru_target->tail;
				lru_target->tail = lru_target->tail->prev;
				lru_target->tail->next = 0;
				l_unlock(lru_target->lock);
				break;
			}
            l_unlock(lru_target->lock);
			lru_target = lru_target->lru_prev;
		}
        
		l_unlock(lock_lru);

		/*insert it at the head and copy info*/
		l_lock(lock);
		temph = head;
		head = tempt;
		head->next = temph;
		head->prev = 0;
		if(temph) {
		    temph->prev = head;
		} else {
			tail = head;
        }
		memcpy(&head->info,info,sizeof(INFO));
		l_unlock(lock);
	}

	bring_to_front();
}

int LRU_CACHE::get(UBMP32 start_index,UBMP32 probe_index,UBMP8& value) {

	register CACHE* curr = head;

	l_lock(lock);
	while(curr) {
		if(curr->info.start_index == start_index) {
			/*
			update cache list and copy value
			*/
			if(curr != head) {
				if(curr == tail)
					tail = tail->prev;
				
				if(curr->prev)
					curr->prev->next = curr->next;
				if(curr->next)
					curr->next->prev = curr->prev;
				curr->next = 0;
				curr->prev = 0;
				
				CACHE *temp = head;
				head = curr;
				head->next = temp;
				temp->prev = head;
			}
			value = head->info.block[probe_index];
			l_unlock(lock);
			
			/*
			put the lru list at the front & return a cache hit
			*/
			bring_to_front();
			return CACHE_HIT;
		}
		curr = curr->next;
	}

	l_unlock(lock);
	return CACHE_MISS;
}
/*
EGBB
*/
struct EGBB {

	FILE* pf;
	UBMP32* index_table;
	UBMP8*  table;
	UBMP32  size,
		    orgsize,
			cmpsize,
			n_blocks,
			block_size,
			read_start;
	HUFFMAN huffman;
	HUFFMAN huffman_pos;


	int  state;
	bool is_loaded;
	bool use_search;
	char* name;
	LOCK lock;
	LRU_CACHE LRUcache;
	
	EGBB() {
		is_loaded = false;
		use_search = false;
		l_create(lock);
		huffman.MAX_LEAFS = LITERAL_CODES;
        huffman.MAX_NODES = 2 * huffman.MAX_LEAFS - 1;
		huffman_pos.MAX_LEAFS = DISTANCE_CODES;
        huffman_pos.MAX_NODES = 2 * huffman_pos.MAX_LEAFS - 1;
	}

	void open(char* name,int egbb_state,UBMP32 TB_SIZE);
	int decode(UBMP8*,UBMP8*,UBMP32);
	int get_score(UBMP32,PSEARCHER);
	UBMP64 read_bytes(int);
};

static EGBB egbb3piece[2 * 14];
static EGBB egbb4piece[2 * 14 * 14];
static EGBB egbb5piece[2 * 14 * 14 * 14];

/*
read n bytes assuming little endian byte order
*/
UBMP64 EGBB::read_bytes(int count) {
    UBMP64 x = 0;
	UBMP8* c = (UBMP8*) &x;
	for(int i = 0; i < count; i++) {
		c[i] = ((UBMP8) fgetc(pf));
	}
	return x;
}

/*
Decode compressed block
*/
int EGBB::decode(
				    UBMP8* in_table,
		            UBMP8* out_table,
					UBMP32 size
				 ) {

	UBMP8* in = in_table;
	UBMP8* ine = in_table + size;
	UBMP8* out = out_table;
	UBMP8* ptr;
	PAIR pair;

	UBMP8  length = 0,j,extra;
	UBMP64 code = 0;

	UBMP32 v,temp;
	int diff;
	CANN  *pcann;
	HUFFMAN* phuf;

#define HUFFMAN_DECODE(huf,x) {\
	phuf = &huf;\
	addbits(phuf->max_length);\
	for(j = phuf->min_length; j <= phuf->max_length;j++) {\
		if(!(pcann = phuf->pstart[j])) \
			continue;\
		diff = UBMP32((code >> (length - j)) & pcann->mask) - pcann->code;\
		if(diff >= 0) {\
			x = (pcann + diff)->symbol;\
			length -= j;\
			break;\
		}\
	}\
};

	while(in < ine) {

		HUFFMAN_DECODE(huffman,v);

		if(v == EOB_MARKER)
			break;

        if(v > EOB_MARKER) {

			pair.len = v - 257;
			pair.len += MIN_MATCH_LENGTH;

			HUFFMAN_DECODE(huffman_pos,v);

			pair.pos = base_dist[v];
			extra = extra_bits[v];
			if(extra != 0) {
				getbits(extra,temp);
				pair.pos += temp;
			}

			ptr = out - pair.pos;
			for(int i = 0; i < pair.len;i++) {
				*out++ = *ptr++;
			}
		} else {
			*out++ = (UBMP8)v;
		}
	}

	return (out - out_table);
}

int EGBB::get_score(UBMP32 index,PSEARCHER psearcher) {
	
    int score;
	if(state == DECOMP_IN_RAM) {

		score = VALUE[(table[index >> 2] >> (2 * (index & 3))) & 3];

	} else if(state == DECOMP_IN_DISK) {

		UBMP8 value;
		UBMP32 block_start = ((index >> 2) / BLOCK_SIZE) * BLOCK_SIZE;
		UBMP32 probe_index = (index >> 2) - block_start;
		
		psearcher->info.start_index = block_start;
		
        if(LRUcache.get(psearcher->info.start_index,probe_index,value) == CACHE_HIT) {
		} else {
			l_lock(lock);
			fseek(pf,block_start,SEEK_SET);
			fread(&psearcher->info.block,BLOCK_SIZE,1,pf);
			l_unlock(lock);
			value = psearcher->info.block[probe_index];
            LRUcache.add(&psearcher->info);
		}

		score = VALUE[(value >> (2 * (index & 3))) & 3];

	} else if(state == COMP_IN_RAM) {

		UBMP32 block_size;
		UBMP32 n_blk = ((index >> 2) / BLOCK_SIZE);
		UBMP32 probe_index = (index >> 2) - (n_blk * BLOCK_SIZE);
		UBMP8 value;
		
		psearcher->info.start_index = index_table[n_blk];
		
		if(LRUcache.get(psearcher->info.start_index,probe_index,value) == CACHE_HIT) {
		} else {
			block_size = index_table[n_blk + 1] - index_table[n_blk];
			block_size = decode(&table[psearcher->info.start_index],psearcher->info.block,block_size);
			value = psearcher->info.block[probe_index];
			LRUcache.add(&psearcher->info);
		}

		score = VALUE[(value >> (2 * (index & 3))) & 3];

	}  else if(state == COMP_IN_DISK) {

		UBMP32 block_size;
		UBMP32 n_blk = ((index >> 2) / BLOCK_SIZE);
		UBMP32 probe_index = (index >> 2) - (n_blk * BLOCK_SIZE);
		UBMP8 value;

		psearcher->info.start_index = index_table[n_blk];
		
		if(LRUcache.get(psearcher->info.start_index,probe_index,value) == CACHE_HIT) {
		} else {
			
			block_size = index_table[n_blk + 1] - index_table[n_blk];

			l_lock(lock);
			fseek(pf,read_start + psearcher->info.start_index,SEEK_SET);
			fread(psearcher->block,block_size,1,pf);
			l_unlock(lock);

        	block_size = decode(psearcher->block,psearcher->info.block,block_size);

			value = psearcher->info.block[probe_index];
			LRUcache.add(&psearcher->info);
		}

		score = VALUE[(value >> (2 * (index & 3))) & 3];
	}

    return score;
}
/*
Open EGBB
*/
void EGBB::open(char* egbb_name,int egbb_state,UBMP32 TB_SIZE) {

    UBMP32 i;
	
	//open file
	pf = fopen(egbb_name,"rb");
	if(!pf) {
		return;
	}

    //load file according to egbb_state	
	name = new char[256];
	strcpy(name,egbb_name);
	is_loaded = true;
	state = egbb_state;

	if(!is_comp(state)) {  //Decompresed in ram/disk
		if(state == DECOMP_IN_RAM) {
			table = new UBMP8[TB_SIZE];
			ASSERT(table);
			
			for(i = 0;i < TB_SIZE;i++) {
				table[i] = fgetc(pf);
			}
		}
	} else {              //compresed in ram/disk
		
		huffman.cann = new CANN[huffman.MAX_LEAFS];
		huffman_pos.cann = new CANN[huffman_pos.MAX_LEAFS];
		
		
		//read counts
		orgsize = (UBMP32) read_bytes(4);
		cmpsize = (UBMP32) read_bytes(4);
		n_blocks = (UBMP32) read_bytes(4);
		block_size = (UBMP32) read_bytes(4);
		
		//read reserve bytes
		for(i = 0;i < 10;i++) {
			read_bytes(4);
		}
		
		//read length
		for(i = 0; i < huffman.MAX_LEAFS; i++) {
			fread(&huffman.cann[i].length,1,1,pf);
			huffman.cann[i].symbol = i;
			huffman.cann[i].code = 0;
			huffman.cann[i].mask = (1 << huffman.cann[i].length) - 1;
		}
		
		//read length
		for(i = 0; i < huffman_pos.MAX_LEAFS; i++) {
			fread(&huffman_pos.cann[i].length,1,1,pf);
			huffman_pos.cann[i].symbol = i;
			huffman_pos.cann[i].code = 0;
			huffman_pos.cann[i].mask = (1 << huffman_pos.cann[i].length) - 1;
		}
		
		//read index table
		index_table = new UBMP32[n_blocks + 1];
		for(i = 0; i < int(n_blocks + 1);i++) {
			index_table[i] = (UBMP32) read_bytes(4);
		}
		
		read_start = ftell(pf);
		
		huffman.build_cann_from_length();
		huffman_pos.build_cann_from_length();
		
		
		//compressed files in RAM
		if(state == COMP_IN_RAM) {
			table = new UBMP8[cmpsize];
			ASSERT(table);
			
			for(i = 0;i < cmpsize;i++) {
				table[i] = fgetc(pf);
			}
		}
	}

}

DLLExport void load_egbb_into_ram(int side,int piece1,int piece2,int piece3) {
	EGBB* pegbb;
	if(piece3) {
		pegbb = &egbb5piece[side * 14 * 14 * 14 + piece1 * 14 * 14 + piece2 * 14 + piece3];
	} else if(piece2) {
		pegbb = &egbb4piece[side * 14 * 14 + piece1 * 14 + piece2];
	} else {
		pegbb = &egbb3piece[side * 14 + piece1];
	}

	if(pegbb->state != COMP_IN_RAM) {
		pegbb->table = new UBMP8[pegbb->cmpsize];
		ASSERT(pegbb->table);
		for(UBMP32 i = 0;i < pegbb->cmpsize;i++) {
			pegbb->table[i] = fgetc(pegbb->pf);
		}
		pegbb->state = COMP_IN_RAM;
	}
}

DLLExport void unload_egbb_from_ram(int side,int piece1,int piece2,int piece3) {

	EGBB* pegbb;
	if(piece3) {
		pegbb = &egbb5piece[side * 14 * 14 * 14 + piece1 * 14 * 14 + piece2 * 14 + piece3];
	} else if(piece2) {
		pegbb = &egbb4piece[side * 14 * 14 + piece1 * 14 + piece2];
	} else {
		pegbb = &egbb3piece[side * 14 + piece1];
	}

    if(pegbb->state == COMP_IN_RAM) {
		pegbb->state = COMP_IN_DISK;
		delete[] (pegbb->table);
	}
}

static void open_egbb(char* path,int state,int piece1,int piece2 = 0,int piece3 = 0) {

	EGBB* pegbb[2];
	UBMP32 TB_SIZE;
	char name[256];

	if(piece3) { //5 piece
		char name1[20],name2[20];

		if(is_comp(state)) {
			strcpy(name1,"kxxkx.x.cmp");
			strcpy(name2,"kxxxk.x.cmp");
		} else {
			strcpy(name1,"kxxkx.x.bin");
			strcpy(name2,"kxxxk.x.bin");
		}

		if(PIECE(piece1) == pawn) {
			if(PIECE(piece3) == pawn)
				TB_SIZE = 3612 * 24 * 47 * 46;
			else
				TB_SIZE = 3612 * 24 * 47 * 62;
		} else if(PIECE(piece2) == pawn) {
			if(PIECE(piece3) == pawn)
				TB_SIZE = 3612 * 24 * 47 * 62;
			else
				TB_SIZE = 3612 * 24 * 62 * 61;
		} else if(PIECE(piece3) == pawn) {
			TB_SIZE = 3612 * 24 * 62 * 61;
		} else {
			TB_SIZE = 462 * 62 * 61 * 60;
		}
		TB_SIZE /= 4;

		for(int side = 0; side < 2;side++) {
			pegbb[side] = &egbb5piece[side * 14 * 14 * 14 + piece1 * 14 * 14 + piece2 * 14 + piece3];
			strcpy(name,path);

			if(COLOR(piece1) != COLOR(piece3)) {
				name1[1] = piece_name[piece1];
				name1[2] = piece_name[piece2];
				name1[4] = piece_name[piece3];
				name1[6] = (side == white) ? 'w':'b';
				strcat(name,name1);
				
			} else {
				name2[1] = piece_name[piece1];
				name2[2] = piece_name[piece2];
				name2[3] = piece_name[piece3];
				name2[6] = (side == white) ? 'w':'b';
				strcat(name,name2);
			}

			pegbb[side]->open(name,state,TB_SIZE);
		}
	} else if(piece2) { //4 piece

		char name1[20],name2[20];

		if(is_comp(state)) {
			strcpy(name1,"kxkx.x.cmp");
			strcpy(name2,"kxxk.x.cmp");
		} else {
			strcpy(name1,"kxkx.x.bin");
			strcpy(name2,"kxxk.x.bin");
		}

		if(PIECE(piece1) == pawn) {
			TB_SIZE = 3612 * 24 * 47;
		} else if(PIECE(piece2) == pawn) {
			TB_SIZE = 3612 * 24 * 62;
		} else {
			TB_SIZE = 462 * 62 * 61;
		}
		TB_SIZE /= 4;

		for(int side = 0; side < 2;side++) {
			pegbb[side] = &egbb4piece[side * 14 * 14 + piece1 * 14 + piece2];
		    strcpy(name,path);

			if(COLOR(piece1) != COLOR(piece2)) {
				name1[1] = piece_name[piece1];
				name1[3] = piece_name[piece2];
				name1[5] = (side == white) ? 'w':'b';
				strcat(name,name1);
			} else {
				name2[1] = piece_name[piece1];
				name2[2] = piece_name[piece2];
				name2[5] = (side == white) ? 'w':'b';
				strcat(name,name2);
			}

			pegbb[side]->open(name,state,TB_SIZE);
		}
	} else if(piece1) { //3 piece
		char name1[20];

		if(is_comp(state)) {
			strcpy(name1,"kxk.x.cmp");
		} else {
			strcpy(name1,"kxk.x.bin");
		}

		if(PIECE(piece1) == pawn)
			TB_SIZE = 3612 * 24;
		else
			TB_SIZE = 462 * 62;
		TB_SIZE /= 4;

		for(int side = 0; side < 2;side++) {
			pegbb[side] = &egbb3piece[side * 14 + piece1];
			strcpy(name,path);

			name1[1] = piece_name[piece1];
			name1[4] = (side == white) ? 'w':'b';
			strcat(name,name1);
			pegbb[side]->open(name,state,TB_SIZE);
		}
	}

	//one side only bitbases
	if(pegbb[0]->is_loaded) {
		if(!pegbb[1]->is_loaded) {
			pegbb[1]->use_search = true;
		}
	} else if(pegbb[1]->is_loaded) {
		pegbb[0]->use_search = true;
	}
}

/*
Initialize index tables
*/
static void init_indices() {
	int temp[640];
	int index;
	int i,j;
	int u1,u2;
	int rot;

	for( i = 0; i < 4096; i++) {
		KK_index[i] = ILLEGAL;
		KK_WP_index[i] = ILLEGAL;
	}

	for( i = 0; i < 640; i++) {
		temp[i] = ILLEGAL;
	}

	index = 0;
	for(i = 0;i < 64;i++) {
		for(j = 0;j < 64;j++) {
			if(distance(SQ6488(i),SQ6488(j)) <= 1)
				continue;
            u1 = i;
			u2 = j;
			rot = 0;
			if(file64(u1) > FILED) {
				u1 = MIRRORF64(u1);
				u2 = MIRRORF64(u2);
				rot ^= rotF;
			}
			if(rank64(u1) > RANK4) {
				u1 = MIRRORR64(u1);
				u2 = MIRRORR64(u2);
				rot ^= rotR;
			}
			if(rank64(u1) > file64(u1)) {
				u1 = MIRRORD64(u1);
				u2 = MIRRORD64(u2);
				rot ^= rotD;
			}
			if(file64(u1) == rank64(u1)) {
				if(rank64(u2) > file64(u2)) {
					u1 = MIRRORD64(u1);
				    u2 = MIRRORD64(u2);
					rot ^= rotD;
				}
				u1 = K_TR[u1];
				u2 = K1_TR[u2];
			} else {
				u1 = K_TR[u1];
				u2 = u2;
			}
			if(temp[u1 * 64 + u2] == ILLEGAL) {
				temp[u1 * 64 + u2] = index;
				KK_index[i * 64 + j] = index;
				rotation[i * 64 + j] = rot;
				index++;
			} else {
				KK_index[i * 64 + j] = temp[u1 * 64 + u2];
				rotation[i * 64 + j] = rot;
			}
		}
	}

	index = 0;
	for( i = 0;i < 64;i++) {
		for( j = 0; j < 64;j++) {
			if(distance(SQ6488(i),SQ6488(j)) <= 1)
				continue;
			KK_WP_index[i * 64 + j] = index++;
		}
	}

}
/*
Open EGBB files and allocate cache
*/
void load_egbb_xxx(char* path,int cache_size,int load_options) {
	
	int piece1,piece2,piece3,state1,state2;

	if(load_options == LOAD_NONE) {
        state1 = COMP_IN_DISK;
		state2 = COMP_IN_DISK;
	} else if(load_options == LOAD_4MEN) {
		state1 = COMP_IN_RAM;
		state2 = COMP_IN_DISK;
	} else if(load_options == SMART_LOAD) {
		state1 = COMP_IN_RAM;
		state2 = COMP_IN_DISK;
	} else if(load_options == LOAD_5MEN) {
		state1 = COMP_IN_RAM;
		state2 = COMP_IN_RAM;
	}

	printf("EgbbProbe 3.1 by Daniel Shawul\n");
	fflush(stdout);

	init_sqatt();
	init_indices();
	LRU_CACHE::alloc( cache_size );
    l_create(searcher_lock);

	printf("Loading egbbs....");
	fflush(stdout);

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		open_egbb(path,state1,piece1);
	}

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			open_egbb(path,state1,piece1,piece2);
		}
	}

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = COMBINE(black,PIECE(piece1)); piece2 <= bpawn; piece2++) {
			open_egbb(path,state1,piece1,piece2);
		}
	}
	
	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = piece2; piece3 <= wpawn; piece3++) {
				open_egbb(path,state2,piece1,piece2,piece3);
			}
		}
	}
	
	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = bqueen; piece3 <= bpawn; piece3++) {
			    open_egbb(path,state2,piece1,piece2,piece3);
			}
		}
	}

	printf("\rEgbbs loaded !      \n");
	fflush(stdout);

}
/*
get indices after doing necessary rotations
*/
#define swap(x,y) {\
		temp = x;\
		x = y;\
		y = temp;\
};

static void get_index(UBMP32& pos_index,UBMP32& tab_index,
						 int player, int w_king, int b_king,
						 int piece1 = 0, int square1 = INVALID,
						 int piece2 = 0, int square2 = INVALID,
						 int piece3 = 0, int square3 = INVALID
						 ) {

	
    int p1,p2,p3,p4,p5;
	int t3,t4,t5;
	int rot,temp;
	int side;
	int kki,p3i,p4i,p5i;
	
	side = player;
	p1 = w_king;
	p2 = b_king;
	
	if(piece3) {

		t3 = piece1;
		t4 = piece2;
        t5 = piece3;
		p3 = square1;
		p4 = square2;
		p5 = square3;

		//1.sort white pieces
		//2.sort black pieces
		if(t3 > t4) {
			swap(t3,t4);
            swap(p3,p4);
		}
		if(t3 > t5) {
			swap(t3,t5);
            swap(p3,p5);
		}
		if(t4 > t5) {
			swap(t4,t5);
            swap(p4,p5);
		}

		if(COLOR(t4) == white
			&& COLOR(t5) == black
			) {
		} else if(COLOR(t3) == white
			&& COLOR(t4) == black
			) {
			swap(t3,t4);
			swap(p3,p4);
			swap(t4,t5);
			swap(p4,p5);
		} else {
			if(PIECE(t3) > PIECE(t4)) {
				swap(t3,t4);
				swap(p3,p4);
				swap(t4,t5);
				swap(p4,p5);
			} else if(PIECE(t3) > PIECE(t5)) {
				swap(t3,t5);
				swap(p3,p5);
				swap(t5,t4);
				swap(p5,p4);
			}
		}
		//first piece should be white
		if(COLOR(t3) != white) {
			t3 = COMBINE(invert(COLOR(t3)),PIECE(t3));
			t4 = COMBINE(invert(COLOR(t4)),PIECE(t4));
			t5 = COMBINE(invert(COLOR(t5)),PIECE(t5));
			side = invert(side);

			p3 = MIRRORR64(p3);
			p4 = MIRRORR64(p4);
			p5 = MIRRORR64(p5);

			temp = p1;
			p1 = MIRRORR64(p2);
			p2 = MIRRORR64(temp);
		}
		if(PIECE(t3) == pawn) {
			
			if(file64(p3) > FILED) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
				p5 = MIRRORF64(p5);

			}
			if(PIECE(t5) == pawn) {
				kki = KK_WP_index[p1 * 64 + p2];
				p3i = SQ6424(p3);
				p4i = SQ6448(p4) - (p4 > p3);
				p5i = SQ6448(p5) - (p5 > p3) - (p5 > p4);
				
				pos_index = kki * 24 * 47 * 46 + p3i * 47 * 46 + p4i * 46 + p5i;
				tab_index = side * 14 * 14 * 14 + t3 * 14 * 14 + t4 * 14 + t5;
            } else {
				kki = KK_WP_index[p1 * 64 + p2];
				p3i = SQ6424(p3);
				p4i = SQ6448(p4) - (p4 > p3);
				p5i = p5 - (p5 > p1) - (p5 > p2);
				
				pos_index = kki * 24 * 47 * 62 + p3i * 47 * 62 + p4i * 62 + p5i;
				tab_index = side * 14 * 14 * 14 + t3 * 14 * 14 + t4 * 14 + t5;
			}
		} else if(PIECE(t4) == pawn) {
			
			if(file64(p4) > FILED) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
                p5 = MIRRORF64(p5);
			}
			
			if(PIECE(t5) == pawn) {
				kki = KK_WP_index[p1 * 64 + p2];
				p3i = SQ6424(p4);
				p4i = SQ6448(p5) - (p5 > p4);
				p5i = p3 - (p3 > p1) - (p3 > p2);
				
				pos_index = kki * 24 * 47 * 62 + p3i * 47 * 62 + p4i * 62 + p5i;
				tab_index = side * 14 * 14 * 14 + t3 * 14 * 14 + t4 * 14 + t5;
			} else {
				kki = KK_WP_index[p1 * 64 + p2];
				p3i = SQ6424(p4);
				p4i = p3 - (p3 > p1) - (p3 > p2);
				p5i = p5 - (p5 > p1) - (p5 > p2) - (p5 > p3);
				
				pos_index = kki * 24 * 62 * 61 + p3i * 62 * 61 + p4i * 61 + p5i;
				tab_index = side * 14 * 14 * 14 + t3 * 14 * 14 + t4 * 14 + t5;
			}
        } else if(PIECE(t5) == pawn) {
			
			if(file64(p5) > FILED) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
                p5 = MIRRORF64(p5);
			}
			
			kki = KK_WP_index[p1 * 64 + p2];
			p3i = SQ6424(p5);
			p4i = p3 - (p3 > p1) - (p3 > p2);
			p5i = p4 - (p4 > p1) - (p4 > p2) - (p4 > p3);

			pos_index = kki * 24 * 62 * 61 + p3i * 62 * 61 + p4i * 61 + p5i;
			tab_index = side * 14 * 14 * 14 + t3 * 14 * 14 + t4 * 14 + t5;

		} else {
			
			rot = rotation[p1 * 64 + p2];
			if(rot & rotF) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
                p5 = MIRRORF64(p5);
			}
			if(rot & rotR) {
				p1 = MIRRORR64(p1);
				p2 = MIRRORR64(p2);
				p3 = MIRRORR64(p3);
				p4 = MIRRORR64(p4);
				p5 = MIRRORR64(p5);
			}
			if(rot & rotD) {
				p1 = MIRRORD64(p1);
				p2 = MIRRORD64(p2);
				p3 = MIRRORD64(p3);
				p4 = MIRRORD64(p4);
				p5 = MIRRORD64(p5);
			}
			
			kki = KK_index[p1 * 64 + p2];
			p3i = p3 - (p3 > p1) - (p3 > p2);
			p4i = p4 - (p4 > p1) - (p4 > p2) - (p4 > p3);
			p5i = p5 - (p5 > p1) - (p5 > p2) - (p5 > p3) - (p5 > p4);

			pos_index = kki * 62 * 61 * 60 + p3i * 61 * 60 + p4i * 60 + p5i;
			tab_index = side * 14 * 14 * 14 + t3 * 14 * 14 + t4 * 14 + t5;

		}

	} else if(piece2) {

		t3 = piece1;
		t4 = piece2;
		p3 = square1;
		p4 = square2;

		//1.sort white pieces
		//2.sort black pieces
		//3.put the one with largest piece in front
		if(t3 > t4) {
			swap(t3,t4);
            swap(p3,p4);
		}

		if(PIECE(t3) > PIECE(t4)) {
			swap(t3,t4);
            swap(p3,p4);
		}

		//first piece should be white
		if(COLOR(t3) != white) {
			t3 = COMBINE(invert(COLOR(t3)),PIECE(t3));
			t4 = COMBINE(invert(COLOR(t4)),PIECE(t4));
			side = invert(side);

			p3 = MIRRORR64(p3);
			p4 = MIRRORR64(p4);
			temp = p1;
			p1 = MIRRORR64(p2);
			p2 = MIRRORR64(temp);
		}
		
		if(PIECE(t3) == pawn) {
			
			if(file64(p3) > FILED) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
			}
			
			kki = KK_WP_index[p1 * 64 + p2];
			p3i = SQ6424(p3);
			p4i = SQ6448(p4) - (p4 > p3);

			pos_index = kki * 24 * 47 + p3i * 47 + p4i;
			tab_index = side * 14 * 14 + t3 * 14 + t4;
			

		} else if(PIECE(t4) == pawn) {
			
			if(file64(p4) > FILED) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
			}
			
			kki = KK_WP_index[p1 * 64 + p2];
			p3i = SQ6424(p4);
			p4i = p3 - (p3 > p1) - (p3 > p2);

			pos_index = kki * 24 * 62 + p3i * 62 + p4i;
			tab_index = side * 14 * 14 + t3 * 14 + t4;

		} else {
			
			rot = rotation[p1 * 64 + p2];
			if(rot & rotF) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
				p4 = MIRRORF64(p4);
			}
			if(rot & rotR) {
				p1 = MIRRORR64(p1);
				p2 = MIRRORR64(p2);
				p3 = MIRRORR64(p3);
				p4 = MIRRORR64(p4);
			}
			if(rot & rotD) {
				p1 = MIRRORD64(p1);
				p2 = MIRRORD64(p2);
				p3 = MIRRORD64(p3);
				p4 = MIRRORD64(p4);
			}
			
			kki = KK_index[p1 * 64 + p2];
			p3i = p3 - (p3 > p1) - (p3 > p2);
			p4i = p4 - (p4 > p1) - (p4 > p2) - (p4 > p3);

			pos_index = kki * 62 * 61 + p3i * 61 + p4i;
			tab_index = side * 14 * 14 + t3 * 14 + t4;

		}

	} else if(piece1) {

		t3 = piece1;
		p3 = square1;

		//first piece should be white
		if(COLOR(t3) != white) {
			t3 = COMBINE(invert(COLOR(t3)),PIECE(t3));
			side = invert(side);

			p3 = MIRRORR64(p3);
			temp = p1;
			p1 = MIRRORR64(p2);
			p2 = MIRRORR64(temp);
		}

		if(PIECE(t3) == pawn) {
			
			if(file64(p3) > FILED) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
			}
			
			kki = KK_WP_index[p1 * 64 + p2];
			p3i = SQ6424(p3);

			pos_index = kki * 24 + p3i;
			tab_index = side * 14 + t3;

		} else {
			
			rot = rotation[p1 * 64 + p2];
			if(rot & rotF) {
				p1 = MIRRORF64(p1);
				p2 = MIRRORF64(p2);
				p3 = MIRRORF64(p3);
			}
			if(rot & rotR) {
				p1 = MIRRORR64(p1);
				p2 = MIRRORR64(p2);
				p3 = MIRRORR64(p3);
			}
			if(rot & rotD) {
				p1 = MIRRORD64(p1);
				p2 = MIRRORD64(p2);
				p3 = MIRRORD64(p3);
			}
			
			kki = KK_index[p1 * 64 + p2];
			p3i = p3 - (p3 > p1) - (p3 > p2);

			pos_index = kki * 62 + p3i;
			tab_index = side * 14 + t3;

		}
	}
}

/*
Exported functions : probe_egbb & load_egbb
*/

int SEARCHER::get_score(
               int alpha,int beta,
			   int side, int w_king, int b_king,
			   int piece1, int square1,
			   int piece2, int square2,
			   int piece3, int square3
			   ) {

	int score;
	int move,from,to;
	int sq1,sq2,sq3,legal_moves;

	UBMP32 pos_index,tab_index;
	EGBB* pegbb;

	if(piece1 == 0)
		return DRAW;

	get_index(pos_index,tab_index,
		      side,w_king,b_king, 
		      piece1,square1,
			  piece2,square2,
			  piece3,square3);

	if(piece3) {
		pegbb = &egbb5piece[tab_index];
	} else if(piece2) {
		pegbb = &egbb4piece[tab_index];
	} else {
		pegbb = &egbb3piece[tab_index];
	}
	
	if(pegbb->is_loaded) {
		return pegbb->get_score(pos_index,this);
	} else if(pegbb->use_search) {
		if(piece3) {
			square1 = SQ6488(square1);
			square2 = SQ6488(square2);
			square3 = SQ6488(square3);
		} else if(piece2) {
			square1 = SQ6488(square1);
			square2 = SQ6488(square2);
		} else {
			square1 = SQ6488(square1);
		}

		//set up position if we are at the root
		if(ply == 0) {
			set_pos(side,SQ6488(w_king),SQ6488(b_king),
				piece1,square1,
				piece2,square2,
				piece3,square3);
		}

		//generate moves and search
		pstack->count = 0;
		gen_caps();
		gen_noncaps();

		legal_moves = 0;

		for(int i = 0;i < pstack->count; i++) {
			
			move = pstack->move_st[i];
			PUSH_MOVE(move);
			if(attacks(player,plist[COMBINE(opponent,king)]->sq)) {
				POP_MOVE(move);
				continue;
			}

			legal_moves++;

			from = m_from(move);
			to = m_to(move);

			//squares
			sq1 = square1;
			sq2 = square2;
			sq3 = square3;

			//remove captured piece
			if(m_capture(move)) {
				if(sq1 == to) {     
					sq1 = sq2;
					sq2 = sq3;
					sq3 = INVALID;
				} else if(sq2 == to) {
					sq2 = sq3;
					sq3 = INVALID;
				} else if(sq3 == to) {
					sq3 = INVALID;
				}
			}
			//move piece
			if(sq1 == from)     
				sq1 = to;
			else if(sq2 == from)
				sq2 = to;
			else if(sq3 == from)
				sq3 = to;
			
			//recursive call
			if(sq1 == INVALID) {
				score = -DRAW;
			} else if(sq2 == INVALID) {
				score = -get_score(-beta,-alpha,
					player,SQ8864(plist[wking]->sq),SQ8864(plist[bking]->sq),
					board[sq1],SQ8864(sq1));
			} else if(sq3 == INVALID) {
				score = -get_score(-beta,-alpha,
					player,SQ8864(plist[wking]->sq),SQ8864(plist[bking]->sq),
					board[sq1],SQ8864(sq1), 
					board[sq2],SQ8864(sq2));
			} else {
				score = -get_score(-beta,-alpha,
					player,SQ8864(plist[wking]->sq),SQ8864(plist[bking]->sq),
					board[sq1],SQ8864(sq1), 
					board[sq2],SQ8864(sq2),
					board[sq3],SQ8864(sq3));
			}

			POP_MOVE(move);

			//update alpha
			if(score > alpha) { 
				alpha = score;
				if(score >= beta) {
					return beta;
				}
			}
		}

		//mate/stalemate?
		if(legal_moves == 0) {
			if(attacks(opponent,plist[COMBINE(player,king)]->sq))
				return LOSS;
			else
				return DRAW;
		}

		return alpha;
	} else {
        return DONT_KNOW;
	}
}

static const int piece_cv[14] = {0,0,9,5,3,3,1,0,-9,-5,-3,-3,-1,0};
static const int  king_pcsq[64] = {
	-3,-2,-1,  0,  0,-1,-2,-3,  
	-2,-1,  0, 1, 1,  0,-1,-2,  
	-1,  0, 1, 2, 2, 1,  0,-1,  
	  0, 1, 2, 3, 3, 2, 1,  0,  
	  0, 1, 2, 3, 3, 2, 1,  0,  
	-1,  0, 1, 2, 2, 1,  0,-1,  
	-2,-1,  0, 1, 1,  0,-1,-2,  
	-3,-2,-1,  0,  0,-1,-2,-3
};

static const int  kbnk_pcsq[64] = {
	 7, 6, 5, 4, 3, 2, 1, 0,
     6, 7, 6, 5, 4, 3, 2, 1,  
     5, 6, 7, 6, 5, 4, 3, 2,  
     4, 5, 6, 7, 6, 5, 4, 3,  
     3, 4, 5, 6, 7, 6, 5, 4,  
     2, 3, 4, 5, 6, 7, 6, 5,  
	 1, 2, 3, 4, 5, 6, 7, 6,  
	 0, 1, 2, 3, 4, 5, 6, 7  
};

int probe_egbb_xxx(int player, int w_king, int b_king,
			   int piece1, int square1,
			   int piece2, int square2,
			   int piece3, int square3
			   ) {

    
	register int wdl_score,score,all_c,p_dist,material,ktable;

	//get score
	if(piece1 == _EMPTY)
		square1 = INVALID;
	if(piece2 == _EMPTY)
		square2 = INVALID;
	if(piece3 == _EMPTY)
		square3 = INVALID;

	PSEARCHER psearcher;
	l_lock(searcher_lock);
	for(int i = 0;i < 8;i++) {
		if(!searchers[i].used) {
			psearcher = &searchers[i];
			psearcher->used = 1;
			break;
		}
	}
	l_unlock(searcher_lock);
	
	wdl_score = psearcher->get_score(
		      LOSS,WIN,
		      player,w_king,b_king, 
		      piece1,square1,
			  piece2,square2,
			  piece3,square3);

    psearcher->used = 0;

	if(wdl_score == DONT_KNOW) {
		return _NOTFOUND;
	} else if(wdl_score == DRAW) {
		return 0;
	} else {

        /* 
		modify score to ensure progress
		*/

		//material
		material = piece_cv[piece1] + piece_cv[piece2] + piece_cv[piece3];
		if(player == black) material = -material;

		//king closeness to pawns
		if(piece3) {
			all_c = 5;
			int wp_dist = 0,bp_dist = 0;
			if(PIECE(piece1) == pawn) {
				wp_dist += distance(SQ6488(square1),SQ6488(w_king));
				bp_dist += distance(SQ6488(square1),SQ6488(b_king));
			}
			
			if(PIECE(piece2) == pawn) {
				wp_dist += distance(SQ6488(square2),SQ6488(w_king));
				bp_dist += distance(SQ6488(square2),SQ6488(b_king));
			}
			
			if(PIECE(piece3) == pawn) {
				wp_dist += distance(SQ6488(square3),SQ6488(w_king));
				bp_dist += distance(SQ6488(square3),SQ6488(b_king));
			}
			
			if(player == white) p_dist = 2 * wp_dist - bp_dist;
			else p_dist = 2 * bp_dist - wp_dist;
			
		} else if(piece2) {
			all_c = 4;
			p_dist = 0;
		} else {
			all_c = 3;
			p_dist = 0;
		}

		//king square table for kbnk and others
		if(all_c == 4 
			&& (((piece1 == wbishop && piece2 == wknight) ||
			     (piece1 == wknight && piece2 == wbishop))  
				 ||
				((piece1 == bbishop && piece2 == bknight) ||
			     (piece1 == bknight && piece2 == bbishop)))
			) {

			//get the king's & bishop's square
            int b_sq,loser_ksq,winner_ksq;
			if(COLOR(piece1) == white) {
				winner_ksq = w_king;
				loser_ksq = b_king;
				if(piece1 == wbishop) {
					b_sq = square1;
				} else {
					b_sq = square2;
				}
			} else {
				winner_ksq = b_king;
				loser_ksq = w_king;
				if(piece1 == bbishop) {
					b_sq = square1;
				} else {
					b_sq = square2;
				}
			}

			if(is_light64(b_sq));
			else {
				winner_ksq = MIRRORF64(winner_ksq); 
				loser_ksq = MIRRORF64(loser_ksq);
			}

            //score
			if(wdl_score == WIN) {
				ktable = king_pcsq[winner_ksq] - kbnk_pcsq[loser_ksq];
			} else {
				ktable = -king_pcsq[winner_ksq] + kbnk_pcsq[loser_ksq];
			}

		} else {
			if(player == white) {
				if(wdl_score == WIN) {
					ktable = - 2 * king_pcsq[b_king] + king_pcsq[w_king];
				} else {
					ktable = + 2 * king_pcsq[w_king] - king_pcsq[b_king];
				}
			} else {
				if(wdl_score == WIN) {
					ktable = - 2 * king_pcsq[w_king] + king_pcsq[b_king];
				} else {
					ktable = + 2 * king_pcsq[b_king] - king_pcsq[w_king];
				}
			}
		}

		//combine score
		if(player == white) {
			if(wdl_score == WIN) {
				score = 
					+ WIN_SCORE
					+ (5 - all_c) * 200 
					- distance(SQ6488(w_king),SQ6488(b_king)) * 2
                    + ktable * 5
					+ material * 50
					- p_dist * 7;
			} else {
				score = 
					- WIN_SCORE
					- (5 - all_c) * 200
					+ distance(SQ6488(w_king),SQ6488(b_king)) * 2
					+ ktable * 5
					+ material * 50
					- p_dist * 7;
			}
		} else {
			if(wdl_score == WIN) {
				score = 
					+ WIN_SCORE
					+ (5 - all_c) * 200
					- distance(SQ6488(w_king),SQ6488(b_king)) * 2
					+ ktable * 5
					+ material * 50
					- p_dist * 7;
			} else {
				score = 
					- WIN_SCORE
					- (5 - all_c) * 200
					+ distance(SQ6488(w_king),SQ6488(b_king)) * 2
					+ ktable * 5
					+ material * 50
					- p_dist * 7;
			}
		}
		return score;
	}
}

/*
old 
*/
DLLExport void CDECL load_egbb(char* path) {
	load_egbb_xxx(path,4194304,LOAD_4MEN);
}
DLLExport int CDECL probe_egbb(int player, int w_king, int b_king,
			    int piece1, int square1,
			    int piece2, int square2
			   ) {
	return probe_egbb_xxx(player,w_king,b_king,
		                  piece1,square1,
						  piece2,square2,
						  _EMPTY,0);
}
/*
new
*/
DLLExport void CDECL load_egbb_5men(char* path,int cache_size,int load_options) {
        load_egbb_xxx(path,cache_size,load_options);
}
DLLExport int  CDECL probe_egbb_5men(int player, int w_king, int b_king,
			    int piece1, int square1,
			    int piece2, int square2,
				int piece3, int square3
			   ) {
	return probe_egbb_xxx(player,w_king,b_king,
		                  piece1,square1,
						  piece2,square2,
						  piece3,square3);
}
