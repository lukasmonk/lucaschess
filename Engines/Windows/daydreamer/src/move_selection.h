
#ifndef MOVE_SELECTION_H
#define MOVE_SELECTION_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROOT_GEN, PV_GEN, NONPV_GEN,
    ESCAPE_GEN, Q_GEN, Q_CHECK_GEN,
} generation_t;

typedef enum {
    PHASE_BEGIN,
    PHASE_END,
    PHASE_ROOT,
    PHASE_TRANS,
    PHASE_EVASIONS,
    PHASE_PV,
    PHASE_NON_PV,
    PHASE_QSEARCH,
    PHASE_QSEARCH_CH,
    PHASE_KILLERS,
    PHASE_GOOD_TACTICS,
    PHASE_BAD_TACTICS,
    PHASE_QUIET,
    PHASE_DEFERRED,
} selection_phase_t;

typedef struct {
    selection_phase_t* phase;
    move_t* moves;
    int64_t* scores;
    move_t base_moves[256];
    move_t deferred_moves[256];
    int num_deferred_moves;
    int64_t base_scores[256];
    move_t pv_moves[256];
    int64_t pv_nodes[256];
    int pv_index;
    int moves_end;
    int current_move_index;
    generation_t generator;
    move_t hash_move[2];
    move_t mate_killer;
    move_t killers[5];
    int num_killers;
    int moves_so_far;
    int quiet_moves_so_far;
    float depth;
    position_t* pos;
    bool single_reply;
} move_selector_t;

#ifdef __cplusplus
} // extern "C"
#endif
#endif // SELECTION_H

