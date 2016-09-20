
#ifndef SEARCH_H
#define SEARCH_H
#ifdef __cplusplus
extern "C" {
#endif

#define PLY                 1.0
#define MAX_SEARCH_PLY      127
#define depth_to_index(x)   ((int)(x))

typedef enum {
    SEARCH_ABORTED, SEARCH_FAIL_HIGH, SEARCH_FAIL_LOW, SEARCH_EXACT
} search_result_t;

typedef struct {
    move_t pv[MAX_SEARCH_PLY+1];
    move_t killers[2];
    move_t mate_killer;
} search_node_t;

typedef enum {
    ENGINE_IDLE=0, ENGINE_PONDERING, ENGINE_THINKING, ENGINE_ABORTED
} engine_status_t;

typedef int score_type_t;
#define SCORE_LOWERBOUND    0x01
#define SCORE_UPPERBOUND    0x02
#define SCORE_EXACT         0x03
#define SCORE_MASK          0x03
#define MATE_THREAT         0x04

typedef move_t(*book_fn)(position_t*);
typedef struct {
    int multi_pv;
    int output_delay;
    bool use_book;
    bool book_loaded;
    book_fn probe_book;
    bool use_scorpio_bb;
    bool use_gtb;
    bool use_gtb_dtm;
    bool root_in_gtb;
    bool nonblocking_gtb;
    int gtb_cache_size;
    int gtb_scheme;
    int max_egtb_pieces;
    int verbosity;
    bool chess960;
    bool arena_castle;
    bool ponder;
} options_t;

extern options_t options;

#define HIST_BUCKETS    15

typedef struct {
    int transposition_cutoffs[MAX_SEARCH_PLY + 1];
    int nullmove_cutoffs[MAX_SEARCH_PLY + 1];
    int move_selection[HIST_BUCKETS + 1];
    int pv_move_selection[HIST_BUCKETS + 1];
    int razor_attempts[3];
    int razor_prunes[3];
    int root_fail_highs;
    int root_fail_lows;
    int egbb_hits;
} search_stats_t;

typedef struct {
    float history[16*64]; // move indexed by piece type and destination square
    int success[16*64];
    int failure[16*64];
} history_t;

#define MAX_HISTORY         1000000
#define MAX_HISTORY_INDEX   (16*64)
#define depth_to_history(d) ((d)*(d))
#define history_index(m)   \
    ((get_move_piece_type(m)<<6)|(square_to_index(get_move_to(m))))

typedef struct {
    uint64_t nodes;
    move_t move;
    int score;
    int max_ply;
    int qsearch_score;
    move_t pv[MAX_SEARCH_PLY + 1];
} root_move_t;

typedef struct {
    position_t root_pos;
    search_stats_t stats;

    // search state info
    root_move_t root_moves[256];
    root_move_t* current_root_move;
    int best_score;
    int scores_by_iteration[MAX_SEARCH_PLY + 1];
    int root_indecisiveness;
    move_t pv[MAX_SEARCH_PLY + 1];
    search_node_t search_stack[MAX_SEARCH_PLY + 1];
    history_t history;
    uint64_t nodes_searched;
    uint64_t qnodes_searched;
    uint64_t pvnodes_searched;
    float current_depth;
    int current_move_index;
    bool resolving_fail_high;
    move_t obvious_move;
    engine_status_t engine_status;

    // when should we stop?
    milli_timer_t timer;
    uint64_t node_limit;
    float depth_limit;
    int time_limit;
    int time_target;
    int time_bonus;
    int mate_search; // TODO: implement me
    bool infinite;
} search_data_t;

extern search_data_t root_data;

#define POLL_INTERVAL   0x3fff
#define MATE_VALUE      32000
#define DRAW_VALUE      0
#define MIN_MATE_VALUE (MATE_VALUE-1024)

#define is_mate_score(score)    \
    (((score)>(MATE_VALUE-MAX_SEARCH_PLY)) || \
     ((score)<(-MATE_VALUE+MAX_SEARCH_PLY)))
#define mate_in(ply)                (MATE_VALUE-(ply))
#define mated_in(ply)               (-MATE_VALUE+(ply))
#define should_output(s)    \
    (elapsed_time(&((s)->timer)) > options.output_delay)


#ifdef __cplusplus
} // extern "C"
#endif
#endif // SEARCH_H
