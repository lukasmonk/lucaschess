//
// Cyrano Chess engine
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

#ifndef _HASH_HXX
#define _HASH_HXX

extern HASHK s_arghashkEnP[filMAX];	// Hash keys to be XOR'd in if an en-passant
								//  capture is possible.
extern HASHK s_arghashkCf[cfMAX];		// Hash keys to beXOR'd in based upon castling
								//  legality.

//	Transposition hash.  This structure is packed, meaning that it contains
//	chars and shorts, which are normally a bad idea.  The reason this is done
//	is that a huge array of these is allocated, and the number of items in the
//	array for a given amount of memory should be maximized.

typedef enum _hashBound {
    hashfNONE = 0x0,
    hashfALPHA = 0x1,			// The score is "val" or worse.  If
								//  "hashfBETA" is also set, the score is
								//  exactly "val".
    hashfBETA = 0x2,			// The score is "val" or better.
    hashfEXACT = 0x3
} hashBound;


class HASH {
public:
    U32     hashkSig;
    U32     count;
//	HASHK	hashk;				// Full hash key.  This is stored in order to
								//  try to avoid collisions.  The effects of
								//  collisions are otherwise ignored.
	short	val;				// This is either an exact value or a bound,
								//  as indicated by the "hashf" flags.
	U8      seq;				// Sequence number (date)

    static U8 s_seq;

private:
    struct {
//        U8 bound : 8;
//        U8 threat : 1;
        U8 bound : 6;
        U8 ext : 1;
        U8 forced : 1;
    } hashf;
//	U8      hashf;				// Flags, as defined above.
	U8      plyRem;				// Draft under this node.
    U8      m1;                 // the move coded on 24 bits
    U8      m2;
    U8      m3;

public:
    static const int pvDepth = -1;

    void setPlyRem(int _plyrem) {
        plyRem = U8(_plyrem + 1);
    }
    int getPlyRem() const {
        return int(plyRem) - 1;
    }
    void setMove(U32 m) {
        m1 = U8(m & 0xff);
        m2 = U8((m >> 8) & 0xff);
        m3 = U8((m >> 16) & 0xff);
    }
    U32 getMove(void) const {
        return ((m1) | (m2<<8) | (m3<<16));
    }
    void setBound( hashBound boundType) {
        hashf.bound = U8( boundType );
    }
    hashBound getBound( void) const {
        return static_cast<hashBound>( hashf.bound );
    }
//    void setThreat(bool threat) {
//        hashf.threat = threat ? 1 : 0;
//    }
    bool getThreat(void) const {
//        return hashf.threat ? true : false;
        return false;
    }
    void setForced(bool threat) {
        hashf.forced = threat ? 1 : 0;
    }
    bool getForced(void) const {
        return hashf.forced ? true : false;
    }
    void setExt(bool threat) {
        hashf.ext = threat ? 1 : 0;
    }
    bool getExt(void) const {
        return hashf.ext ? true : false;
    }
    void touch() {
        s_seq = HASH::s_seq;
    }
};
typedef HASH *PHASH;

class EvalCacheEntry {
public:
    HASHK   hashk;
    short   val;
};

class EvalCache {
private:
    EvalCacheEntry *cache;
    int numEntries;
public:
    EvalCache(int _numEntries);
    ~EvalCache();
    const EvalCacheEntry *find(const STE *pste);
    void set(const STE *pste, int val);
};

extern EvalCache *evalCache;

//	This is to be called with the side to move changes, or if in the first
//	position, it is black to move.

#define HashkSwitch(hashkDst)	((hashkDst) ^ C64(0x21D420B884CD6731))

bool FInitHashe(PCON pcon);
void VInitHashk(void);
void VClearHashe(void);

bool FRepSet(CON const *pcon, STE const *pste, HASHK hashk);

void RecordHashValue(PCON pcon, PSTE pste, const CM &cm, int val, int Alpha, int Beta, int depth, bool hasThreat);
void RecordHashValue(PCON pcon, PSTE pste, const CM &cm, int val, hashBound hashf, int depth);
PHASH ProbeHashValue(const STE *pste);

extern HASHK s_arghashkBB[16][csqMAX];

static INLINE void rehashBB(HASHK &hashk, int pc, int isq) {
    hashk ^= s_arghashkBB[pc][isq];
}

#endif
