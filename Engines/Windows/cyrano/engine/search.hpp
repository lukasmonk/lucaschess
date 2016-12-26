//
//
// Copyright (C) 2007  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifndef _SEARCH_HXX
#define _SEARCH_HXX

// the definition of the node types can be found for example in
// "Multi-Cut alpha/beta-pruning in game-tre search"

typedef enum _nodeType {
    nodePV,
    nodeCUT,
    nodeALL
} nodeType;


// Killer moves, 2 per depth

class _killers {
public:
    void raz(void) {
        Killer1 = 0;
        Killer2 = 0;
    }
    void INLINE set(U32 m) {
        if( !(Killer1 == m ) ) {
            Killer2 = Killer1;
            Killer1 = m;
        }
    }
    U32 Killer1;
    U32 Killer2;
};
extern _killers Killers[];
extern _killers MateKillers[];

// it is not clear atm which history indexing is better

//#define HIST_SIZE       (64*64)
//#define HIST_INDEX(m)   (From(m)+64*To(m))
#define HIST_SIZE       (2*16*64)
#define HIST_INDEX(m)   ((Pc(m)+16*To(m))*2 + (Capt(m) ? 1 : 0))

class _CounterMove {
private:
    _killers counterMoves[HIST_SIZE];
public:
    void raz(void) {
        for(int i = 0; i < HIST_SIZE; i++) {
            counterMoves[i].raz();
        }
    }
    void INLINE set(U32 previousMove, U32 counterMove) {
        if( !Capt(counterMove) && !Capt(previousMove) ) {
            int idx = HIST_INDEX(previousMove);
            counterMoves[idx].set(counterMove);
        }
    }
    void INLINE get(U32 previousMove, U32 &counterMove1, U32 &counterMove2) const {
        if( !Capt(previousMove) ) {
            int idx = HIST_INDEX(previousMove);
            counterMove1 = counterMoves[idx].Killer1;
            counterMove2 = counterMoves[idx].Killer2;
        } else {
            counterMove1 = 0;
            counterMove2 = 0;
        }
    }
};
extern _CounterMove CounterMoves;

class _History {
private:
    unsigned int good_hit[HIST_SIZE];
//    unsigned int depth_hit[HIST_SIZE];
    int move_count[HIST_SIZE];
    int failed_high[HIST_SIZE];
    int failed_low_count[HIST_SIZE];
    int shift_count;
public:
    _History() {};
    void raz(void) {
        shift_count = 0;
        for(int i = 0; i < HIST_SIZE; i++) {
            good_hit[i] = failed_low_count[i] = 0;
            move_count[i] = failed_high[i] = 1;
        }
    }
    // increment the history counter on failed high
    void INLINE update(const CM *cm, int depth) {
        if( Capt(cm->m) )
            return;
        int idx = HIST_INDEX(cm->m);
        good_hit[ idx ] += depth*depth;
        if( good_hit[ idx ] >= 0x00100000 ) {
            shift_count++;
            for(int i = 0; i < HIST_SIZE ; i++ ) {
                good_hit[ i ] = (good_hit[ i ] + 1) / 2;
            }
        }
    }
    void INLINE update_bad(const U32 m) {
        int idx = HIST_INDEX(m);
        move_count[ idx ]++;
    }
    void INLINE update_good(const U32 m) {
        int idx = HIST_INDEX(m);
        move_count[ idx ]++;
        failed_high[ idx ]++;
    }
    void seen(const CM *cm, nodeType nt) {
        if( cm != NULL /*&& nt != nodeALL*/ ) {
            int idx = HIST_INDEX(cm->m);
            move_count[ idx ]++;
        }
    }
    int seen_count(const CM *cm) const {
        return move_count[ HIST_INDEX(cm->m) ];
    }
    void failed_low(const CM *cm, nodeType nt) {
        if( cm != NULL && nt != nodeALL ) {
            int idx = HIST_INDEX(cm->m);
            failed_low_count[ idx ]++;
        }
    }
    int get_ratio(const CM *cm) const {
        int idx = HIST_INDEX(cm->m);
        if( move_count [ idx ] == 0 )
            return 1024;
        return (1024 * failed_high[ idx ]) / move_count [ idx ];
//        return good_count[ idx ] - bad_count [ idx ];
    }
    unsigned int sort_key(const CM *cm) const {
        int idx = HIST_INDEX(cm->m);
        return good_hit[ idx ];
    }
    unsigned int get_stats(const CM *cm) const {
        int idx = HIST_INDEX(cm->m);
        return (failed_high[ idx ] << 16) | failed_low_count[ idx ];
    }
    bool INLINE can_reduce(const CM *cm) const {
        int idx = HIST_INDEX(cm->m);
        // we don't want to reduce if there is less than xx percent of time the move has
        // failed low
        if( ((1024*failed_high[ idx ]) / move_count [ idx ]) >= 700)
            return false;

        return true;
    }
    bool INLINE can_prune(const CM *cm, int plyrem) const {
        int idx = HIST_INDEX(cm->m);
        // we don't want to reduce if there is less than xx percent of time the move has
        // failed low
//        if( (( (plyrem+1) * 1024*failed_high[ idx ]) / move_count [ idx ]) < 400)
//            return true;
        if( (( (plyrem+1) * 1024*failed_high[ idx ]) / move_count [ idx ]) < 700)
            return true;
//        if( ((1024*failed_high[ idx ]) / move_count [ idx ]) < 200)
//            return true;

        return false;
    }
};

extern _History History;


#endif // _SEARCH_HXX

