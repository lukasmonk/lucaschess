
#ifndef TRANS_TABLE_H
#define TRANS_TABLE_H
#ifdef __cplusplus
extern "C" {
#endif

// TODO: shrink this structure.
// TODO: track mate threats and whether null moves should be attempted
typedef struct {
    hashkey_t key;
    move_t move;
    float depth;
    int16_t score;
    uint8_t age;
    uint8_t flags;
} transposition_entry_t;

#ifdef __cplusplus
} // extern "C"
#endif
#endif // TRANS_TABLE_H
