
#include "daydreamer.h"
#include <string.h>

static material_data_t* material_table = NULL;
static void compute_material_data(const position_t* pos, material_data_t* md);

static int num_buckets;
static struct {
    int misses;
    int hits;
    int occupied;
    int evictions;
} material_hash_stats;

/*
 * Create a material hash table of the appropriate size.
 */
void init_material_table(const int max_bytes)
{
    assert(max_bytes >= 1024);
    int size = sizeof(material_data_t);
    num_buckets = 1;
    while (size <= max_bytes >> 1) {
        size <<= 1;
        num_buckets <<= 1;
    }
    if (material_table != NULL) free(material_table);
    material_table = malloc(size);
    assert(material_table);
    clear_material_table();
}

/*
 * Wipe the entire table.
 */
void clear_material_table(void)
{
    memset(material_table, 0, sizeof(material_data_t) * num_buckets);
    memset(&material_hash_stats, 0, sizeof(material_hash_stats));
}

/*
 * Look up the material data for the given position.
 */
material_data_t* get_material_data(const position_t* pos)
{
    material_data_t* md = &material_table[pos->material_hash % num_buckets];
    if (md->key == pos->material_hash) {
        material_hash_stats.hits++;
        return md;
    } else if (md->key != 0) {
        material_hash_stats.evictions++;
    } else {
        material_hash_stats.misses++;
        material_hash_stats.occupied++;
    }
    compute_material_data(pos, md);
    md->key = pos->material_hash;
    return md;
}

/*
 * Calculate static score adjustments and scaling factors that are based
 * solely on the combination of pieces on the board. Each combination
 * has an associated hash key so that this data can be cached in in the
 * material table. There are relatively few possible material configurations
 * reachable in a typical search, so the hit rate should be extremely high.
 */
static void compute_material_data(const position_t* pos, material_data_t* md)
{
    md->phase = game_phase(pos);

    md->score.midgame = 0;
    md->score.endgame = 0;

    int wp = pos->piece_count[WP];
    int bp = pos->piece_count[BP];
    int wn = pos->piece_count[WN];
    int bn = pos->piece_count[BN];
    int wb = pos->piece_count[WB];
    int bb = pos->piece_count[BB];
    int wr = pos->piece_count[WR];
    int br = pos->piece_count[BR];
    int wq = pos->piece_count[WQ];
    int bq = pos->piece_count[BQ];
    int w_major = 2*wq + wr;
    int w_minor = wn + wb;
    int w_piece = 2*w_major + w_minor;
    int w_all = wq + wr + wb + wn + wp;
    int b_major = 2*bq + br;
    int b_minor = bn + bb;
    int b_piece = 2*b_major + b_minor;
    int b_all = bq + br + bb + bn + bp;

    // Pair bonuses
    if (wb > 1) {
        md->score.midgame += 30;
        md->score.endgame += 45;
    }
    if (bb > 1) {
        md->score.midgame -= 30;
        md->score.endgame -= 45;
    }
    if (wr > 1) {
        md->score.midgame -= 12;
        md->score.endgame -= 17;
    }
    if (br > 1) {
        md->score.midgame += 12;
        md->score.endgame += 17;
    }
    if (wq > 1) {
        md->score.midgame -= 8;
        md->score.endgame -= 12;
    }
    if (bq > 1) {
        md->score.midgame += 8;
        md->score.endgame += 12;
    }

    // Pawn bonuses
    int material_adjust = 0;
    material_adjust += wn * 3 * (wp - 4);
    material_adjust -= bn * 3 * (bp - 4);
    material_adjust += wb * 2 * (wp - 4);
    material_adjust -= bb * 2 * (bp - 4);
    material_adjust += wr * (-3) * (wp - 4);
    material_adjust -= br * (-3) * (bp - 4);
    material_adjust += 10 * (b_minor - w_minor);
    material_adjust += 10 * (b_major - w_major);
    md->score.midgame += material_adjust;
    md->score.endgame += material_adjust;

    // Recognize specific material combinations where we want to do separate
    // scaling or scoring.
    md->eg_type = EG_NONE;
    md->scale[WHITE] = md->scale[BLACK] = 1024;
    if (w_all + b_all == 0) {
        md->eg_type = EG_DRAW;
    } else if (w_all + b_all == 1) {
        if (wp) {
            md->eg_type = EG_KPK;
            md->strong_side = WHITE;
        } else if (bp) {
            md->eg_type = EG_KPK;
            md->strong_side = BLACK;
        } else if (wq || wr) {
            md->eg_type = EG_WIN;
            md->strong_side = WHITE;
        } else if (bq || br) {
            md->eg_type = EG_WIN;
            md->strong_side = BLACK;
        } else {
            md->eg_type = EG_DRAW;
        } 
    } else if (w_all == 0) {
        if (b_piece > 2) {
            md->eg_type = EG_WIN;
            md->strong_side = BLACK;
        } else if (b_piece == 2) {
            if (bn == 2) {
                md->eg_type = EG_DRAW;
            } else if (bb && bn) {
                md->eg_type = EG_KBNK;
                md->strong_side = BLACK;
            } else {
                md->eg_type = EG_WIN;
                md->strong_side = BLACK;
            }
        }
    } else if (b_all == 0) {
        if (w_piece > 2) {
            md->eg_type = EG_WIN;
            md->strong_side = WHITE;
        } else if (w_piece == 2) {
            if (wn == 2) {
                md->eg_type = EG_DRAW;
            } else if (wb && wn) {
                md->eg_type = EG_KBNK;
                md->strong_side = WHITE;
            } else {
                md->eg_type = EG_WIN;
                md->strong_side = WHITE;
            }
        }
    }
    
    // Endgame scaling factors
    if (md->eg_type == EG_WIN) {
        md->scale[md->strong_side^1] = 0;
    } else if (md->eg_type == EG_DRAW) {
        md->scale[BLACK] = md->scale[WHITE] = 0;
        return;
    }

    // It's hard to win if you don't have any pawns, or if you only have one
    // and your opponent can trade it for a piece without leaving mating
    // material. Bishops tend to be better than knights in this scenario.
    if (!wp) {
        if (w_piece == 1) {
            md->scale[WHITE] = 0;
        } else if (w_piece == 2 && wn == 2) {
            md->scale[WHITE] = 32;
        } else if (w_piece - b_piece < 2 && w_major < 3) {
            md->scale[WHITE] = 128;
        } else if (w_piece == 2 && wb == 2) {
            md->scale[WHITE] = 768;
        } else if (!w_major) md->scale[WHITE] = 512;
    } else if (wp == 1 && b_piece) {
        if (w_piece == 1 || (w_piece == 2 && wn == 2)) {
            md->scale[WHITE] = 256;
        } else if (w_piece - b_piece + (b_major == 0) < 1 && w_major < 3) {
            md->scale[WHITE] = 512;
        }
    }

    if (!bp) {
        if (b_piece == 1) {
            md->scale[BLACK] = 0;
        } else if (b_piece == 2 && bn == 2) {
            md->scale[BLACK] = 32;
        } else if (b_piece - w_piece < 2 && b_major < 3) {
            md->scale[BLACK] = 128;
        } else if (b_piece == 2 && bb == 2) {
            md->scale[BLACK] = 768;
        } else if(!b_major) md->scale[BLACK] = 512;
    } else if (bp == 1 && w_piece) {
        if (b_piece == 1 || (b_piece == 2 && bn == 2)) {
            md->scale[BLACK] = 256;
        } else if (b_piece - w_piece + (w_major == 0) < 1 && b_major < 3) {
            md->scale[BLACK] = 512;
        }
    }
}

/*
 * Is this position an opening or an endgame? Scored on a scale of 0-24,
 * with 24 being a pure opening and 0 a pure endgame.
 * Note: the maximum phase is given by MAX_PHASE, which needs to be updated
 *       in conjunction with this function.
 */
int game_phase(const position_t* pos)
{
    return pos->piece_count[WN] + pos->piece_count[WB] +
        2*pos->piece_count[WR] + 4*pos->piece_count[WQ] +
        pos->piece_count[BN] + pos->piece_count[BB] +
        2*pos->piece_count[BR] + 4*pos->piece_count[BQ];
}

