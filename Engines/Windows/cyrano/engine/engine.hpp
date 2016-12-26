
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include <stdio.h>
#include <string.h>
#include "config.hpp"


#define VERSION "0.6b17"


#ifdef _DEBUG
#define DEBUG
void VAssertFailed(const char * szMod, int iLine);
#define	Assert(cond)		if (!(cond)) VAssertFailed(s_aszModule, __LINE__)
#else
#define	Assert(cond)
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	filA		0
#define	filB		1
#define	filC		2
#define	filD		3
#define	filE		4
#define	filF		5
#define	filG		6
#define	filH		7

#define	filMAX		8		// Number of files on the board.

#define	filIBD		16		// The number of files on the internal board.  The
							//  internal board is 16 files by 8 ranks.

#define	rnk1		0
#define	rnk2		1
#define	rnk3		2
#define	rnk4		3
#define	rnk5		4
#define	rnk6		5
#define	rnk7		6
#define	rnk8		7

#define	rnkMAX		8		// Number of ranks on the board.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	coEMPTY             -1

#define	coWHITE             1   // Piece colors.
#define	coBLACK             0
#define	coMAX               2

#define FlipColor(co)       ((co)^1)
#define AnyColor(piece)     ((piece)|1)
#define ColorOf(piece)      ((piece) & 1)

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
#define	csqMAX	(rnkMAX * filIBD)

#define	cfNONE          0x0000  // These flags indicate castling legality.
#define	cfE1G1          0x0001
#define	cfE1C1          0x0002
#define	cfE8G8          0x0004
#define	cfE8C8          0x0008

#define	cfMAX           16      // The number of different combinations of flags.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	valPAWN         100     // Values in centi-pawns of the pieces.
#define	valKNIGHT       324
#define	valBISHOP       325
#define	valROOK         500
#define	valQUEEN        975
#define	valKING         10000   // This is just a big number.

#define	valMIN          -40000  // These are aways away from valMATE in order
#define	valMAX          40000   //  to avoid unforeseen problems.

#define	valMATE         32767   // -Mate is -32767.  Mate in 1 is 32766. -Mate
                                //  in 1 is -32765.  Mate in 2 is 32764.  Etc.
#define valMATEscore    32000

#define mateValue(depth)    (-valMATE + (depth) - 1)


//	Candidate move.  The move generator emits records of this type.

class CM {
public:
    bool operator ==(const CM &cm) const  {
        return (m == cm.m);
    }
    CM(U32 _m, unsigned _cmk) : m(_m), cmk(_cmk) {}
    CM() {}
    U32         m;          // packed move format
	unsigned	cmk;		// Sort key.
};

typedef CM * PCM;

#define	ccmMAX          8192    // Number of moves in the candidate move array.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
class HASH;
typedef HASH *PHASH;

typedef	U64	HASHK,				// Hash key.  This will be used for the hash
	* PHASHK;					//  table, repetition check, etc.


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	There will be an array of these structures.  At the root of the search,
//	a pointer is set to the first element in the array, and the fields in the
//	element will be set based upon current context.  When "MakeMove" is
//	called, the fields in the next element will be appropriately set, and a
//	pointer to this new element will be passed into the recursed search
//	function.  This saves me from having to pass a lot of parameters around,
//	and saves me some other headache.

//	So this is a stack.  "STE" stands for "stack element", which is something
//	historical from my othe chess program.  Sorry.

#define	csteMAX	128			// This is just a sufficiently large number.

typedef	struct	tagSTE {
	HASHK	hashkPc;		// Hash key for this position.  Castling and
							//  en-passant aren't accounted for.  This is all
							//  pieces and all pawns.
	HASHK	hashkPn;		// Hash key for the pawns in this position.
// Pins for the side to move
	U64 pinned;
	U64 pinner;
    int side, xside;        // side to move
	int	plyRem;				// Number of plies of full-width left to do.
	int	valPcUs;			// The material value of all of our non-pawns.
	int	valPcThem;			// The material value of all of their non-pawns.
	int	valPnUs;			// The material value of all of our pawns.
	int	valPnThem;			// The material value of all of their pawns.
    CM  *mlist_start;       // move list start

	int	plyFifty;			// Fifty-move counter (100 (2 * 50) is a draw).

    //int prevCaptValue;      // value of the capture piece on previous ply
    int prevCaptSqr;        // where on board was the capture
	CM	argcmPv[csteMAX];	// This is the partial PV constructed by a search
							//  that returns a value between alpha and beta.
	int     ccmPv;          // The number of moves in "argcmPv".
    U32 lastMove;           // to display the current variation
    int kingsafety[2];
    int threat[2];
    int     val;            // evaluation
    int     bound;          // recognizer bound
    U32 threatMove;
    U8  threatTarget;
	U8      ep_t;
	U8      castle;   // castling flag
	U8      checkf;   // The side to move is in check.
    bool    evalislazy;     // true if we did not do a real eval
    bool    evaluated;      // true if this position was evaluated
	bool    fNull;			// A null-move is allowed.
    bool    reduced;        // set if branch depth was reduced
//    bool    danger;
    bool    checkdone[2];
    bool    ttmove;
    U8      c_ttpair;
    U8      c_altMoves;

}	STE, * PSTE;

// If "cmPv.isqFrom" is not isqNIL, if this move
//  is found in the move list, it should be
//  searched first.  These elements are primed at
//  the start of each call to the search function.
//  They are primed with the PV returned by the
//  previous call to the search function.
extern CM lastPV[];

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	unsigned long	TM;	// Time value (in milliseconds).

#define	tmANALYZE	(TM)-1	// This is just a flag passed to "VThink" to make
							//  it go into analyze mode and think forever.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Time control types.

#define	tctFIXED_TIME	0
#define	tctTOURNEY		1
#define	tctINCREMENT	2	// Also handles sudden-death controls (incr=0).

typedef	struct	tagTIME_CONTROL {
	int	tct;		// Type of time control.
	int	cMoves;		// Moves in tournament control.
	TM	tmBase;		// Base for fischer control, or total time for a
					//  tournament time control.
	TM	tmIncr;		// Increment for fischer control.
	int	plyDepth;	// Depth in fixed depth time control.  Fixed depth time
					//  control is in addition to the regular time control
					//  It will do the regular time control until it runs out
					//  of depth.  "plyDepth" of <= 0 is ignored.
    U64 nodes;      // search limited by node count
}	TIME_CONTROL, * PTIME_CONTROL;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This maintains a record of previous moves in this game.  It's use to prime
//	the repetition hash, and in case I'm told to back up a move or more.

typedef	struct tagGAME_CONTEXT {
	char	aszFen[256];	// Starting fen.
	CM      argcm[2048];	// List of moves made so far.
	HASHK	arghashk[2049];	// Hash keys *before* each move made.  This causes
							//  some nasty problems, because after a move is
							//  made, this one more element in this array than
							//  is in the "argcm" array.  So loops through
							//  this array look kind of weird.
	int	ccm;				// Number of moves made so far.
	int	movStart;			// First game move (this is also in the FEN).
	int	coStart;			// First-moving color (this is also in the FEN).
}	GAME_CONTEXT, * PGAME_CONTEXT;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This contains information about the ongoing search.

typedef	struct tagSEARCH_STATUS {
	U64	nodes;				// Number of nodes searched by the current search.
	U64	nodesNext;			// Nodes I must exceed before next callback to
							//  interface.
	int	ccmLegalMoves;		// Legal moves at the root.
	int	icmSearching;		// Which move I'm searching now.
	char aszSearching[16];	// Alegbraic of the move I'm searching now.
	char aszSearchSan[16];	// san of the move I'm searching now.
	int	plyDepth;			// Depth of current search.
	int	plyMaxDepth;		// Max depth reached.
	unsigned tmUs;			// Time on clock at start of this move.
	unsigned tmThem;		// Time on opponent's clock at start of this move.
	unsigned tmStart;		// Time I started thinking.
	unsigned tmEnd;			// Time I am scheduled to end.
//--
    unsigned tmEndLimit;    // Absolute limit
    bool    scoreDropped;   // set if new pv score has dropped
//    bool    dontStop;       // set if we do not want to stop now
    bool    dontStopPrev;   // from last iteration
    bool    easyMove;       // set if there is an easy move
    bool    rootHasChanged;
    bool    canStop;
//--

//	unsigned tmExtra;		// Time I have reserved for emergencies.
	int	val;				// The value of the search, as of the last time
							//  I dumped a PV.
	int	prsa;				// The search mode (normal, fail high, etc.).
    int lastVal;            // value of the previous search
//    bool tmExtend;          // set if we'd like to extend the time a bit
    bool failed;            // set when failling high to not stop the search early
    CM  currBestMove;
    int tbhits;             // bitbases hit count
}	SEARCH_STATUS, * PSEARCH_STATUS;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

// material signature for recognizers ( à la DT )

typedef struct {
    U8 msB : 1;
    U8 msN : 1;
    U8 msQ : 1;
    U8 msR : 1;
    U8 msP : 1;
} msigT;


//	Thinking modes.

#define	smodeTHINK		0	// The engine is thinking, and will eventually
							//  move.
#define	smodeANALYZE	1	// The engine is thinking forever.
#define	smodePONDER		2	// The engine has gotten one move ahead of the
							//  real game score, and is thinking about what
							//  to do if that move it's gotten ahead by really
							//  happens.
							// The engine can be put into "think" mode by
							//  simply changing the state, and telling it when
							//  to stop thinking.
#define	smodeIDLE		3	// Doing nothing.  A call to "Think" is not
							//  in the backtrace.

//	There will be approximately one of these active.  It represents the
//	board position, and also includes some current search state information.
//	When a move is made during the search, the board (argsq) in here is
//	messed with, the piece list (argpi) is messed with, and a new STE element
//	is initialized.  When a move is undone, some of the STE information is
//	used to unmake the move on the board and piece list, and the rest is just
//	popped off and discarded.

//	A parameter to this thing is passed around, a lot.  This is annoying, but
//	you'll thank me if you ever try to implement a parallel search.  If you
//	have two of these elements, you can have two searches running at the same
//	time, the only ill effect being hash table overwrite weirdness, since that
//	is the only real "global" non-constant data used by this engine.

//	This is a big structure, and you may not be able to put one of these on
//	the stack.

typedef	struct	tagCON {
    // game context for the bb part
    U8          pos[64];
    bitboard    pieces[17];
    U8          king[2];
	STE	argste[csteMAX];	// Search context stack.
    CM  move_list[ccmMAX];  // move list stack
	CM	argcm[ccmMAX];		// Move generator stack.

    msigT   msigBit[2];     // material signature for recognizer
    U8      msigCount[16];  // counter per material type and side

	char	aszPonder[16];	// Move the engine is pondering if in ponder mode.
	SEARCH_STATUS	ss;		// Information about search progress.
	GAME_CONTEXT	gc;		// A list of moves made in the game so far, etc.
	TIME_CONTROL	tc;		// Time control currently in use.
	int	smode;				// Engine search mode (think, ponder, etc.).

    bool    hashStore;
    bool    fAbort;			// This is set to TRUE if the engine should
							//  immediately end its search.  The engine should
							//  unmake all made moves as it is unwinding, but
							//  it shouldn't expect that a move is going to be
							//  made when it is done.
	bool    fTimeout;		// This is true if the engine is out of time and
							//  should end its search at its convenience.  In
							//  my implementation, it will finish the first
							//  ply before responding to this, otherwise it
							//  will respond immediately.
	bool    fPost;			// TRUE if variation information should be posted
							//  to the interface.
	bool    fPonder;		// TRUE if the engine should ponder (ever).
	bool    fLowPriority;	// TRUE if the engine should reduce its priority
							//  when searching.
    bool    isUCI;          // TRUE if using the uci protocol
	bool    fResign;		// TRUE if the program should resign if it is
							//  getting mauled.
	bool fDrawOffered;		// This will be true if the opponent offered a
							//  draw between my last move and now.

}	CON, * PCON;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A few utility functions.

#define Isq64FromRnkFil(rnk, fil)	((rnk) * filMAX + (fil))
#define IsqFromRnkFil(rnk, fil)		((rnk) * filIBD + (fil))

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

									//  ascii (p, N, k, etc.).
extern int	const s_argvalPc[];		// Array of "block of wood" values of the
									//  pieces.
extern int  const b_valPc[16];
extern int	const s_argvalPcOnly[];	// Same as "s_argvalPc", only king and
									//  pawn are zeroed.
extern int	const s_argvalPnOnly[];	// Same as "s_argvalPc", only all except
									//  pawn are zeroed.
extern char const * s_argszCo[];	// "White", "Black".
#if 0
extern int const *					// These are shared by the move generator
	s_argpvecPiece[];
extern int const * s_argpvecPawn[];	//  and the attack detection code.  They
									//  are documented in "move.c".
extern const int c_argisq64[];      // 128 to 64 conversion
#endif

extern char const s_aszFenDefault[];		// FEN for default chess start position.
extern char const * c_argszNoYes[];		// An array of "no" and "yes".



#ifdef _DEBUG
void VDumpBoard(PCON pcon);
void VDumpCm(PCON pcon, PSTE pste);
#endif

//	These are documented in the source modules.

bool FInitHashp(PCON pcon, int cbMaxPawnHash);
void VInitMiddlegamePawnTable(void);
int ValEval(PCON pcon, PSTE pste, int Alpha, int Beta);
int ValPawns(PCON pcon, PSTE pste);
void VThink(PCON pcon, TM tmUs, TM tmThem);
void VInitAttackData(void);
void VInitHash(void);
bool FAttacked(PCON pcon, int isqDef, int coAtk);
void VFixSte(PCON pcon);
bool FAdvance(PCON pcon, char * sz);
void VUndoMove(PCON pcon);
unsigned TmNow(void);
void VDumpPv(PCON pcon, int ply, int val, int prsa);
void OutDump(const char *txt);
void VDumpSearch(PCON pcon, PSTE psteBase, int val, int Alpha, int Beta);

void VSetTime(PCON pcon, bool wasPondering = false);
void VSetTimeControl(PCON pcon, int cMoves, TM tmBase, TM tmIncr);
void VSetFixedDepthTimeControl(PCON pcon, int plyDepth);
void VSetFixedNodeCount(PCON pcon, U64 nodeCount);
void VCheckTime(PCON pcon);
void VSetPv(PCON pcon);
void VDrawCheck(PCON pcon);
void VClearHashp(void);
bool FGenBook(PCON pcon, char * szFile, int cbMaxBook, int bdm);
bool FBookMove(PCON pcon, PCM pcmMove);
void VMates(int coWin);
bool FStalemated(PCON pcon);
PCON PconInitEngine(int argc, char * argv[]);
void VIncrementHashSeq(void);
void VStalemate(int coStalemated);
void VReseed(void);
void VGetProfileSz(const char * szKey, const char * szDefault,
	char * szOut, int cbOut);
void VLowPriority(void);
int IGetProfileInt(const char * szKey, int iDefault);
bool FBnHashSet(HASHK hashk);
void SortHash(PSTE pste, PHASH phash);
void SortPV(PSTE pste, int depth);
//void SortKillers(PSTE pste, int depth);
void VDumpLine(const CON *pcon, const STE *pste, const char *comment);
