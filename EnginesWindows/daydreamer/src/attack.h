
#ifndef ATTACK_H
#define ATTACK_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NONE_FLAG=0, WP_FLAG=1<<0, BP_FLAG=1<<1, N_FLAG=1<<2,
    B_FLAG=1<<3, R_FLAG=1<<4, Q_FLAG=1<<5, K_FLAG=1<<6
} piece_flag_tag_t;
typedef int piece_flag_t;
extern const piece_flag_t piece_flags[];
#define get_piece_flag(piece)       piece_flags[(piece)]

typedef struct {
    piece_flag_t possible_attackers;
    direction_t relative_direction;
} attack_data_t;

extern const attack_data_t* board_attack_data;
extern const int* distance_data;

#define get_attack_data(from, to)   board_attack_data[(from)-(to)]
#define distance(from,to)           distance_data[(from)-(to)]
#define direction(from, to) \
    get_attack_data((from),(to)).relative_direction
#define possible_attack(from, to, piece) \
    ((get_attack_data((from),(to)).possible_attackers & \
     piece_flags[(piece)]) != 0)

#ifdef __cplusplus
} // extern "C"
#endif
#endif // ATTACK_H

