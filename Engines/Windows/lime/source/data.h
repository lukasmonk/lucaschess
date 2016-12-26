#ifndef DATA_H
#define DATA_H

#include <cassert>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <malloc.h>

using namespace std;


extern FILE *log_file;
extern int logme;

extern jmp_buf stopped;

extern int his_table[MOVEBITS];
extern s_Position p[1];
extern s_EvalOptions eo[1];
extern int          castlebits[144];
extern const char   piecetochar[15];
extern const char   brdranks[8];
extern const char   brdfiles[8];

extern s_Hashelem   *TTable;
extern int          numelem;
extern s_Move pondermove;

extern int check[48];
extern int red[48];

extern unsigned __int64 hash_p[14][144];//piece/square
extern unsigned __int64 hash_s[2];//side
extern unsigned __int64 hash_ca[16];
extern unsigned __int64 hash_enp[144];//one for each square

extern const int	knightinc[];
extern const int	bishopinc[];
extern const int	rookinc[];
extern const int	queeninc[];
extern const int    wpawninc[];
extern const int    bpawninc[];
extern const int    *incarrays[13];
extern const int    ranks[144];
extern const int    files[144];

extern  const int vals[16];
extern char colours[2];

extern s_Hist hist[1024];
extern s_Move nomove;
extern s_Move best;

extern sEvalData eval[1];

extern s_SearchParam searchparam[1];


extern int stopsearch;
extern int itdepth;
extern s_Move pv[48][48];
extern int pvindex[48];
extern s_Move killer1[48];
extern s_Move killer2[48];
extern int killerscore[48];
extern int killerscore2[48];
extern s_Move matekiller[48];
extern int history[144][144];
extern int hisall[2][144][144];
extern int hisfh[2][144][144];
extern int donull;
extern int nodes;
extern int qnodes;
extern int followpv;
extern int histply;

extern float fhf;
extern float fh;
extern float nulltry;
extern float nullcut;
extern float hashprobe;
extern float hashhit;
extern float incheckext;
extern float wasincheck;
extern float matethrt;
extern float pawnfifth;
extern float pawnsix;
extern float prom;
extern float pvsh;
extern float pvs;
extern float reduct;
extern float single;
extern float resethis;


extern FILE *book_file;//the old book of text moves arranged in lines e2e4 e7e5 etc
extern FILE *bookfile;//used to write the binary keys when making a book
extern FILE *wbookfile;//used to read the white book binary keys
extern vector<string> vbooklines;//stores text lines e2e4 e7e5
extern vector<s_binentry> hashentries;//the hashentries determined for the book
extern FILE *hash_file;//hash keys from converted old book
extern vector<s_binentry> whitebook;

extern int movestotal;
extern int readtotal;

extern s_binentry binenter;
extern s_bookinfo bookdata;







#endif

