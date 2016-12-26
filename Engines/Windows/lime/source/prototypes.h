#ifndef PROTOTYPES_H
#define PROTOTYPES_H

extern void  init_castlebits();
extern int   fileranktosquare(const int f, const int r);
extern int   chartofile (const char file);
extern int   chartorank(const char rank);
extern char  ranktochar(int rank);
extern char  filetochar(int file);
extern char  piece(int piece);
extern void  init_piecelists();
extern void  clearboard();
extern void  setboard(const char fen[]);
extern void  init_hash_tables();
extern void  clearhash();
extern void  fullhashkey();
extern void  testhashkey();
extern void  debugevalsee();
extern void  printboard();
extern int  myparse(char move[]);
extern char  *returnsquare(int from);
extern void  debugpiecelists();
extern void  movegen();
extern void  movegen_legal();
extern int makelegalmove(s_Move *m);
extern int isdrawnp();
extern void xboard();
extern void  capgen();
extern int   isattacked(int sq, int side);
extern void  printnums();
extern int   canreduce(s_Move *m);
extern void  debugatt();
extern void  takemove();
extern int extradepth(s_Move *m);
extern int   makemove(s_Move *m);
extern void histgood(s_Move *move, int depth);
extern void histbad(s_Move *move, int depth);
extern void hisreset();
extern int   understandmove(char move[], int *prom);
extern void  perft(int depth);
extern char  *returnmove(s_Move move);
extern void  perftfile();
extern void  midgameeval();
extern int   gameeval();
extern void  debugeval();
extern void  debuginiteval();
extern void init_distancetable();
extern double allocatetime();
extern void parse_option(char string[]);
extern void initsearchparam();
extern void think();
extern void parse_go(char string[]);
extern void parse_position(char string[]);
extern void uci_mode();
extern int Bioskey(void);
extern void checkup();
extern int checkinput();
extern int timeup();
extern void calc();
extern void initsearch();
extern int  search(int alpha, int beta, int depth, int null);
extern int    root_search(int alpha, int beta, int depth);
extern int firstquies(int alpha,int beta);
extern int  quies(int alpha, int beta);
extern void pick(int from);
extern void printpv(int score);
extern int  isrep();
extern int time_check();
extern void debugorder();
extern void store_hash(int depth, int score, int flag, bool null, s_Move *move);
extern int  probe_hash_table(int depth, s_Move *move, int *null, int *score, int beta);
extern void stats();
extern void debugnps();
extern int order(s_Move *hm);
extern void update_killers (s_Move move, int score);
extern void update_history(s_Move move, int depth);
extern int nopvmove(char *move);
extern s_Move findhashmove(s_Move m);
extern int plyok();
extern int moveok(int move);
extern void root_move_list();
extern void root_init();
extern void score_root_moves();
extern void root_sort();

extern void writeply(int ply);
extern void openlog();
extern void closelog();
extern void writemove(s_Move our);
extern void writescore(int score);
extern void writefpv(bool fpv);
extern void writestring(const char *s);
extern void writespace();
extern void writenewline();
extern void writeint(int i);
extern void writedouble(double i);
extern void writeboard();
extern void writegamehist();
extern char   *returncastle();
extern void debugprofile();
extern int qorder(s_Move *hm);


extern int book_init();
extern void parseopeningline(char string[]);
extern void makehash();
extern int wfindhashbookmove();
extern void book_close();
extern void store();
extern double pondertime();







#endif



