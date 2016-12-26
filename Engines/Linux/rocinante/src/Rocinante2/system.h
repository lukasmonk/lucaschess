
// declares

extern long TimeElapsed();
extern void Wait();
extern void RunBStar(void *pArg);

struct DataRunBStar {
	char *fen;
	int MoveTime;
};