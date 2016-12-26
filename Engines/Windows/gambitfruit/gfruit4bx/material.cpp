
// material.cpp

// includes

#include <cstring>

#include "board.h"
#include "colour.h"
#include "hash.h"
#include "material.h"
#include "option.h"
#include "piece.h"
#include "protocol.h"
#include "square.h"
#include "util.h"



// constants

static const uint32 TableSize = 256; // 4kB

static const int PawnPhase   = 0;
static const int KnightPhase = 1;
static const int BishopPhase = 1;
static const int RookPhase   = 2;
static const int QueenPhase  = 4;

static const int TotalPhase = PawnPhase * 16 + KnightPhase * 4 + BishopPhase * 4 + RookPhase * 4 + QueenPhase * 2;

// constants and variables

#define ABS(x) ((x)<0?-(x):(x))

//static /* const */ int MaterialWeight = 256; // 100%

static /*const*/ int PawnOpening   = 80; // was 100
static /*const*/ int PawnEndgame   = 90; // was 100
static /*const*/ int KnightOpening = 320;
static /*const*/ int KnightEndgame = 320;
static /*const*/ int BishopOpening = 325;
static /*const*/ int BishopEndgame = 325;
static /*const*/ int RookOpening   = 500;
static /*const*/ int RookEndgame   = 500;
static /*const*/ int QueenOpening  = 975;
static /*const*/ int QueenEndgame  = 975;

static /*const*/ int BishopPairOpening = 50;
static /*const*/ int BishopPairEndgame = 50;

static /*const*/ int Queen_Knight_combo = 15; // with no rooks
static /*const*/ int Rook_Bishop_combo = 15;  // with no queens
static /*const*/ int bad_trade_value = 50; // not used like in crafty... (will be for 3 minors vs queen)

static int bitbase_pieces = 2;

static const int RookPawnPenalty[17] = { 15,15,13,11,9,7,5,3,1,-1,-3,-5,-7,-9,-11,-13,-15 };

// types

typedef material_info_t entry_t;

struct material_t {
    entry_t * table;
    uint32 size;
    uint32 mask;
    uint32 used;
    sint64 read_nb;
    sint64 read_hit;
    sint64 write_nb;
    sint64 write_collision;
};

// variables

static material_t Material[1];

// prototypes

static void material_comp_info (material_info_t * info, const board_t * board);

// functions

// material_init()

void material_init() {

    // UCI options

    bitbase_pieces =  (option_get_int("Bitbase pieces") -2);

    //MaterialWeight = (option_get_int("Material") * 256 + 50) / 100;
    bad_trade_value = option_get_int("Bad Trade Value");

    BishopPairOpening = option_get_int("Bishop Pair Opening");
    BishopPairEndgame = option_get_int("Bishop Pair Endgame");

    Queen_Knight_combo = option_get_int("Queen Knight combo");
    Rook_Bishop_combo = option_get_int("Rook Bishop combo");

    PawnOpening = option_get_int("Opening Pawn Value");
    KnightOpening = option_get_int("Opening Knight Value");
    BishopOpening = option_get_int("Opening Bishop Value");
    RookOpening = option_get_int("Opening Rook Value");
    QueenOpening = option_get_int("Opening Queen Value");

    PawnEndgame = option_get_int("Endgame Pawn Value");
    KnightEndgame = option_get_int("Endgame Knight Value");
    BishopEndgame = option_get_int("Endgame Bishop Value");
    RookEndgame = option_get_int("Endgame Rook Value");
    QueenEndgame = option_get_int("Endgame Queen Value");

    // material table

    Material->size = 0;
    Material->mask = 0;
    Material->table = NULL;
}

// material_alloc()

void material_alloc() {

    ASSERT(sizeof(entry_t)==16);


    Material->size = TableSize;
    Material->mask = TableSize - 1;
    Material->table = (entry_t *) my_malloc(Material->size*sizeof(entry_t));

    material_clear();

}

// material_clear()

void material_clear() {

    if (Material->table != NULL) {
        memset(Material->table,0,Material->size*sizeof(entry_t));
    }

    Material->used = 0;
    Material->read_nb = 0;
    Material->read_hit = 0;
    Material->write_nb = 0;
    Material->write_collision = 0;
}

// material_get_info()

void material_get_info(material_info_t * info, const board_t * board) {

    uint64 key;
    entry_t * entry;

    ASSERT(info!=NULL);
    ASSERT(board!=NULL);

    // probe


    Material->read_nb++;

    key = board->material_key;
    entry = &Material->table[KEY_INDEX(key)&Material->mask];

    if (entry->lock == KEY_LOCK(key)) {

        // found

        Material->read_hit++;

        *info = *entry;

        return;
    }


    // calculation

    material_comp_info(info,board);

    // store

    Material->write_nb++;

    if (entry->lock == 0) { // HACK: assume free entry
        Material->used++;
    } else {
        Material->write_collision++;
    }

    *entry = *info;
    entry->lock = KEY_LOCK(key);

}

// material_comp_info()

static void material_comp_info(material_info_t * info, const board_t * board) {

    int wp, wn, wb, wr, wq;
    int bp, bn, bb, br, bq;
    int wt, bt;
    int wm, bm;
    int colour;
    uint8 recog;
    uint8 flags;
    uint8 cflags[ColourNb];
    uint8 mul[ColourNb];
    sint16 phase;
    sint16 opening, endgame;
    int owf,obf,ewf,ebf; /* Thomas */
    int WhiteMinors,BlackMinors,WhiteMajors,BlackMajors;


    ASSERT(info!=NULL);
    ASSERT(board!=NULL);

    // init

    wp = board->number[WhitePawn12];
    wn = board->number[WhiteKnight12];
    wb = board->number[WhiteBishop12];
    wr = board->number[WhiteRook12];
    wq = board->number[WhiteQueen12];

    bp = board->number[BlackPawn12];
    bn = board->number[BlackKnight12];
    bb = board->number[BlackBishop12];
    br = board->number[BlackRook12];
    bq = board->number[BlackQueen12];

    wt = wq + wr + wb + wn + wp; // no king
    bt = bq + br + bb + bn + bp; // no king

    wm = wb + wn;
    bm = bb + bn;

    // recogniser

    recog = MAT_NONE;

    if (false) {

    } /*else if (wt == 0 && bt == 0) {

      recog = MAT_KK;

   }*/ else if (wt == 2 && bt == 1) {

        if (wr == 1 && wp == 1 && br == 1) recog = MAT_KRPKR;
        if (wb == 1 && wp == 1 && bb == 1) recog = MAT_KBPKB;

    } else if (wt == 1 && bt == 2) {

        if (wr == 1 && br == 1 && bp == 1) recog = MAT_KRKRP;
        if (wb == 1 && bb == 1 && bp == 1) recog = MAT_KBKBP;
    }

    // draw node (exact-draw recogniser)

    flags = 0; // TODO: MOVE ME
    cflags[0] = cflags[1] = 0;

    // bishop endgame

    if (wq+wr+wn == 0 && bq+br+bn == 0) { // only bishops
        if (wb == 1 && bb == 1) {
            if (wp-bp >= -2 && wp-bp <= +2) { // pawn diff <= 2
                flags |= DrawBishopFlag;
            }
        }
    }

    // multipliers

    mul[0] = mul[1] = 16; // 1

    // white multiplier

    if (wp == 0) { // white has no pawns

        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (false) {

        } else if (w_tot == 1) {

            ASSERT(w_maj==0);
            ASSERT(w_min==1);

            // KBK* or KNK*, always insufficient

            mul[White] = 0;

        } else if (w_tot == 2 && wn == 2) {

            ASSERT(w_maj==0);
            ASSERT(w_min==2);

            // KNNK*, usually insufficient

            if (b_tot != 0 || bp == 0) {
                mul[White] = 0;
            } else { // KNNKP+, might not be draw
                mul[White] = 1; // 1/16
            }

        } else if (w_tot == 2 && wb == 2 && b_tot == 1 && bn == 1) {

            ASSERT(w_maj==0);
            ASSERT(w_min==2);
            ASSERT(b_maj==0);
            ASSERT(b_min==1);

            // KBBKN*, barely drawish (not at all?)

            mul[White] = 8; // 1/2

        } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[White] = 2; // 1/8
        }

    } else if (wp == 1) { // white has one pawn

        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (false) {

        } else if (b_min != 0) {

            // assume black sacrifices a minor against the lone pawn

            b_min--;
            b_tot--;

            if (false) {

            } else if (w_tot == 1) {

                ASSERT(w_maj==0);
                ASSERT(w_min==1);

                // KBK* or KNK*, always insufficient

                mul[White] = 4; // 1/4

            } else if (w_tot == 2 && wn == 2) {

                ASSERT(w_maj==0);
                ASSERT(w_min==2);

                // KNNK*, usually insufficient

                mul[White] = 4; // 1/4

            } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

                // no more than 1 minor up, drawish

                mul[White] = 8; // 1/2
            }

        } else if (br != 0) {

            // assume black sacrifices a rook against the lone pawn

            b_maj--;
            b_tot -= 2;

            if (false) {

            } else if (w_tot == 1) {

                ASSERT(w_maj==0);
                ASSERT(w_min==1);

                // KBK* or KNK*, always insufficient

                mul[White] = 4; // 1/4

            } else if (w_tot == 2 && wn == 2) {

                ASSERT(w_maj==0);
                ASSERT(w_min==2);

                // KNNK*, usually insufficient

                mul[White] = 4; // 1/4

            } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

                // no more than 1 minor up, drawish

                mul[White] = 8; // 1/2
            }
        }
    }

    // black multiplier

    if (bp == 0) { // black has no pawns

        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (false) {

        } else if (b_tot == 1) {

            ASSERT(b_maj==0);
            ASSERT(b_min==1);

            // KBK* or KNK*, always insufficient

            mul[Black] = 0;

        } else if (b_tot == 2 && bn == 2) {

            ASSERT(b_maj==0);
            ASSERT(b_min==2);

            // KNNK*, usually insufficient

            if (w_tot != 0 || wp == 0) {
                mul[Black] = 0;
            } else { // KNNKP+, might not be draw
                mul[Black] = 1; // 1/16
            }

        } else if (b_tot == 2 && bb == 2 && w_tot == 1 && wn == 1) {

            ASSERT(b_maj==0);
            ASSERT(b_min==2);
            ASSERT(w_maj==0);
            ASSERT(w_min==1);

            // KBBKN*, barely drawish (not at all?)

            mul[Black] = 8; // 1/2

        } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[Black] = 2; // 1/8
        }

    } else if (bp == 1) { // black has one pawn

        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (false) {

        } else if (w_min != 0) {

            // assume white sacrifices a minor against the lone pawn

            w_min--;
            w_tot--;

            if (false) {

            } else if (b_tot == 1) {

                ASSERT(b_maj==0);
                ASSERT(b_min==1);

                // KBK* or KNK*, always insufficient

                mul[Black] = 4; // 1/4

            } else if (b_tot == 2 && bn == 2) {

                ASSERT(b_maj==0);
                ASSERT(b_min==2);

                // KNNK*, usually insufficient

                mul[Black] = 4; // 1/4

            } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

                // no more than 1 minor up, drawish

                mul[Black] = 8; // 1/2
            }

        } else if (wr != 0) {

            // assume white sacrifices a rook against the lone pawn

            w_maj--;
            w_tot -= 2;

            if (false) {

            } else if (b_tot == 1) {

                ASSERT(b_maj==0);
                ASSERT(b_min==1);

                // KBK* or KNK*, always insufficient

                mul[Black] = 4; // 1/4

            } else if (b_tot == 2 && bn == 2) {

                ASSERT(b_maj==0);
                ASSERT(b_min==2);

                // KNNK*, usually insufficient

                mul[Black] = 4; // 1/4

            } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

                // no more than 1 minor up, drawish

                mul[Black] = 8; // 1/2
            }
        }
    }

    // potential draw for white

    if (wt == wb+wp && wp >= 1) cflags[White] |= MatRookPawnFlag;
    if (wt == wb+wp && wb <= 1 && wp >= 1 && bt > bp) cflags[White] |= MatBishopFlag;

    if (wt == 2 && wn == 1 && wp == 1 && bt > bp) cflags[White] |= MatKnightFlag;

    // potential draw for black

    if (bt == bb+bp && bp >= 1) cflags[Black] |= MatRookPawnFlag;
    if (bt == bb+bp && bb <= 1 && bp >= 1 && wt > wp) cflags[Black] |= MatBishopFlag;

    if (bt == 2 && bn == 1 && bp == 1 && wt > wp) cflags[Black] |= MatKnightFlag;

    // king safety

    if (bq >= 1 && bq+br+bb+bn >= 2) cflags[White] |= MatKingFlag;
    if (wq >= 1 && wq+wr+wb+wn >= 2) cflags[Black] |= MatKingFlag;

    // phase (0: opening -> 256: endgame)

    phase = TotalPhase;

    phase -= wp * PawnPhase;
    phase -= wn * KnightPhase;
    phase -= wb * BishopPhase;
    phase -= wr * RookPhase;
    phase -= wq * QueenPhase;

    phase -= bp * PawnPhase;
    phase -= bn * KnightPhase;
    phase -= bb * BishopPhase;
    phase -= br * RookPhase;
    phase -= bq * QueenPhase;

    if (phase < 0) phase = 0;

    ASSERT(phase>=0&&phase<=TotalPhase);
    phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

    ASSERT(phase>=0&&phase<=256);

    // material

    opening = 0;
    endgame = 0;

    /* Thomas */
    owf = wn*KnightOpening + wb*BishopOpening + wr*RookOpening + wq*QueenOpening;
    //info->pv[White] = owf;
    opening += owf;
    opening += wp * PawnOpening;

    obf = bn*KnightOpening + bb*BishopOpening + br*RookOpening + bq*QueenOpening;
    //info->pv[Black] = obf;
    opening -= obf;
    opening -= bp * PawnOpening;

    ewf = wn*KnightEndgame + wb*BishopEndgame + wr*RookEndgame + wq*QueenEndgame;
    endgame += wp * PawnEndgame;
    endgame += ewf;

    ebf = bn*KnightEndgame + bb*BishopEndgame + br*RookEndgame + bq*QueenEndgame;
    endgame -= bp * PawnEndgame;
    endgame -= ebf;

    //WhiteMinors = wn + wb;
    //BlackMinors = bn + bb;
    WhiteMajors = wq + wr;
    BlackMajors = bq + br;

    // Trade Bonus

    if (wm+WhiteMajors > bm+BlackMajors+1) { // pieces over majors
        opening += bad_trade_value;
        endgame += bad_trade_value;
    } else if (bm+BlackMajors > wm+WhiteMajors+1) {
        opening -= bad_trade_value;
        endgame -= bad_trade_value;
    } /*else if (WhiteMajors != BlackMajors) { // major over pawns
	if (WhiteMajors > BlackMajors && wm >= bm) {
		opening += bad_trade_value;
		endgame += bad_trade_value;
	} else if (BlackMajors > WhiteMajors && bm >= wm) {
		opening -= bad_trade_value;
		endgame -= bad_trade_value;
	}
   }*/


    // bishop pair

    if (wb >= 2) { // HACK: assumes different colours
        opening += BishopPairOpening;
        endgame += BishopPairEndgame;
    }

    if (bb >= 2) { // HACK: assumes different colours
        opening -= BishopPairOpening;
        endgame -= BishopPairEndgame;
    }

    // two knight penalty
    if (wn >= 2) {
        opening -= 10;
        endgame -= 10;
    }

    if (bn >= 2) {
        opening += 10;
        endgame += 10;
    }

    // two rook penalty
    if (wr >= 2) {
        opening -= 10;
        endgame -= 10;
    }

    if (br >= 2) {
        opening += 10;
        endgame += 10;
    }


    // rook score adjustment for number of pawns
    if (wr) {
        opening += RookPawnPenalty[wp+bp] * wr;
        endgame += RookPawnPenalty[wp+bp] * wr;
    }

    if (br) {
        opening -= RookPawnPenalty[wp+bp] * br;
        endgame -= RookPawnPenalty[wp+bp] * br;
    }

    // Queen and knight are better than queen and bishop.
    if (!wr && !br) {
        if (wq && wn) {
            opening += Queen_Knight_combo;
            endgame += Queen_Knight_combo;
        }
        if (bq && bn) {
            opening -= Queen_Knight_combo;
            opening -= Queen_Knight_combo;
        }
    }

    // Rook and bishop are better than rook and knight.
    if (!wq && !bq) {
        if (wr && wb) {
            opening += Rook_Bishop_combo;
            endgame += Rook_Bishop_combo;
        }
        if (br && bb) {
            opening -= Rook_Bishop_combo;
            endgame -= Rook_Bishop_combo;
        }
    }

    if (wp+bp+wm+bm+WhiteMajors+BlackMajors <= bitbase_pieces) flags |= MatBitbaseFlag;

    // store info

    info->recog = recog;
    info->flags = flags;
    info->cflags[0] = cflags[0];
    info->cflags[1] = cflags[1];
    info->mul[0] = mul[0];
    info->mul[1] = mul[1];
    info->phase = phase;
    info->opening = opening;
    info->endgame = endgame;
}

// end of material.cpp

