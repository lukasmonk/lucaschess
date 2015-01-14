
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Gerbil
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Gerbil chess program project.
//
//	Gerbil is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Gerbil is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Gerbil; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include "engine.hpp"
#include "gproto.hpp"
#include "hash.hpp"
#include "stats.hpp"
#include "uci.hpp"

#include <stdlib.h>

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif

#define BUCKET_SIZE 4       // power of 2 (1,2,4)

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This module has two basic functions.  It manages the repetition hash
//	table, and it manages the main transposition hash table.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These functions affect the repetition-check hash table, which is a small
//	table designed to hold all the moves in the current line I'm thinking
//	about, plus all the moves that have already been played on the board.

//	The hash table must be messed with in first-in/last-out order.  If you
//	hash element A, then you hash element B, then you remove element A, it is
//	not guaranteed that you will be able to find element B anymore, which is
//	very bad.  You must remove element B before element A.  Alpha-beta works
//	in this order naturally, so no problem.

//	If you mess up and leave orphan elements in this table while doing a
//	search, you risk a crash at best, and hard to find day-long bugs at worst.

//	This table must be a power of two in size.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The hash table is composed of two parallel tables, each with different
//	replacement schemes.  It ends up being an array of these:

typedef	struct tagHASHD {
	HASH	hashDepth;			// "Different search or deeper depth" table.
}	HASHD, * PHASHD, * RGHASHD;


HASHK s_arghashkBB[16][csqMAX];
HASHK s_arghashkEnP[filMAX];	// Hash keys to be XOR'd in if an en-passant
								//  capture is possible.
HASHK s_arghashkCf[cfMAX];		// Hash keys to beXOR'd in based upon castling
								//  legality.


bool FRepSet(CON const *pcon, STE const *pste, HASHK hashk)
{
//    if(pste->plyFifty < 4 )
//        return false;

    for (int i = 4; i <= pste->plyFifty; i += 2) {
        if(pcon->gc.arghashk[ pcon->gc.ccm - i ] == hashk )
            return true;
    }
    return false;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These functions deal with the transposition hash table.  The transposition
//	hash table is very large, and is actually two tables.

//	The first table operates via a "replace always" scheme.  If you try to
//	insert an element into the table, it always is inserted.

//	The second table operates via a "replace if deeper or more recent search"
//	replacement scheme.  Every time the "think" function is called, a sequence
//	number is incremented.  If you try to insert an element into the table, it
//	is inserted if the sequence number now is different than the one
//	associated with the element in the table.  If the sequence numbers are the
//	same, an element will be inserted only if the depth associated with the
//	element you are trying to add is greater than the depth associated with
//	the element already here.

//	This two table scheme is commonly known, although I found out about it via
//	an email from Ken Thompson in 1994 or so.  His idea is that deep searches
//	can stick around for a while, while stuff that is very recent can also
//	stay around, even if it is shallow.

//	His system also allowed for repetitions to be detected through this
//	main hash table, but I find that using a smaller table specifically for
//	that results in less complexity and (hopefully) fewer bugs.

//	If you make changes to the transposition hash, you should test the program
//	with Fine #70:

//  8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1

//	The best move is Kb1 (a1b1), which should be found with a decent positive
//	score after a couple of seconds, or perhaps less, depending upon how
//	efficient this program ends up getting.  If you've broken the
//	transposition hash system, you won't solve this problem in sensible time.

//	It is very easy to screw up the transposition table hash, and it's
//	possible that this implementation has bugs already.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The hash table.

static HASHD *s_rghashd = 0;
static void *hashBuffer = 0;
static unsigned	s_chashdMac = 0;    // index mask
static int chashdMax = 0;

//	This is used to control overwrites in one of the hash tables, as is
//	documented elsewhere in this module.  Look for "sequence number".

U8 HASH::s_seq = 0;

EvalCache *evalCache = 0;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This checks both hash tables, looking primarily for an element that
//	causes a cutoff.  If neither element can produce a cutoff, I'll check
//	to see if either of them has a "best" move associated with it.  If so,
//	I'll remember the element, so I can increase the candidate move key (cmk)
//	for the move after I generate moves.

//	A "best" move is indicated by the "isqFrom" field being something other
//	than zero.  If it is zero, the other two move fields ("isqTo" and
//	"pcTo") are undefined.

//	It's very possible to have two different hash elements for this position,
//	each with a different best move.  If both elements have the same move,
//	I'll only remember one of them.


//	Given a hash element, this checks to see if conditions are ripe for a
//	cutoff.  In order to produce a cutoff, the element has to record a
//	search that's deep enough.  If so, an "exact" element always returns
//	a cutoff.  An "alpha" or "beta" element only returns a cutoff if it can
//	be proven that the score associated with it would result in a cutoff
//	given the current alpha or beta.

static INLINE U32 shift32(U64 b) {
#if defined(_MSC_VER)
        // HACK ALERT
        union {
            U64 s64;
            struct {
                U32 b;
                U32 c;
            } s32;
        } u3264;
        u3264.s64 = b;
        U32 x = u3264.s32.c;
#else
        U32 x = U32(b >> 32);
#endif
        return x;
    }

#define HASHKEY_OFFSET(h)   U32(h)
#define HASHKEY_SIG(h)      shift32(h)

PHASH ProbeHashValue(const STE *pste)
{
    HASHD *phashd = &s_rghashd[HASHKEY_OFFSET(pste->hashkPc) & s_chashdMac];
    U32 thisHashSig = HASHKEY_SIG(pste->hashkPc);
    for(int i = 0; i < BUCKET_SIZE; i++) {
        if( (phashd + i)->hashDepth.hashkSig == thisHashSig ) {
            stats.Hdepthfound++;
            return &(phashd + i)->hashDepth;
        }
    }
    return 0;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tries to update both hash tables with the results of this search.

//	A fine point here is that I don't overwrite the "best" move in an element
//	with a null move if there was already a "best" move in this element, as
//	long as the hash key in here is the key for the current position.

//	Meaning, if I found that this position was >=50 at some time past, I might
//	have a "best" move in here, which is the move that generated the score of
//	>=50.  Later on if I come back with a higher window, and I find that the
//	move is <=75 (a fail low), I won't have a best move from that search,
//	because none of the moves worked.  I'll keep the old best move in the
//	element, because it's possible that it's still best, and in any case it's
//	a good guess.


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
static inline hashBound calcBoundType(int val, int Alpha, int Beta) {
    if( val >= Beta )
        return hashfBETA;
    else if( val <= Alpha )
        return hashfALPHA;
    else
        return hashfEXACT;
}
void RecordHashValue(PCON pcon, PSTE pste, const CM &cm, int val, hashBound hashf, int depth)
{
#if 1
    if( val >= valMATEscore ) {
        val += depth - 1;
    } else if( val <= -valMATEscore ) {
        val -= depth - 1;
    }
#endif
    int plyrem = pste->plyRem;

//    if( !pcon->hashStore )  // HACK
//        return;
    if( !pcon->hashStore )  // HACK
        plyrem = HASH::pvDepth;

//    (pste)->ttmove = (pste+1)->ttmove;
/*    if( (pste+1)->ttmove ) {
//        return;
        plyrem -= 1;
        if( pste->plyRem <= 0 )
            return;
    }*/
    //
    //	Replace allways
    //
    PHASHD phashd = &s_rghashd[HASHKEY_OFFSET(pste->hashkPc) & s_chashdMac];
    U32 thisHashSig = HASHKEY_SIG(pste->hashkPc);
    PHASHD goodSlot = phashd;
    int bestScore = -10000000;
    bool forced = false;
    bool ext = false;
    U32 nodes = 0;
    for(int i = 0; i < BUCKET_SIZE; i++) {
        PHASHD newSlot = (phashd + i);
        // reuse slot if found
        if( newSlot->hashDepth.hashkSig == thisHashSig ) {
            goodSlot = newSlot;
            forced = newSlot->hashDepth.getForced();
            ext = newSlot->hashDepth.getExt();
            nodes = newSlot->hashDepth.count;
            if( true && (plyrem == HASH::pvDepth) /*&& goodSlot->hashDepth.getPlyRem() > plyrem */) {
                // don't overide if it's deeper
                PHASH phash = &goodSlot->hashDepth;
                if( phash->getMove() == 0 )
                    phash->setMove( cm.m );
                //phash->setBound( hashf );
                //phash->setForced( forced );
                //phash->count = nodes;
                //Assert( plyrem >= 0 && plyrem < 125 );
                //phash->plyRem = plyrem;
                phash->seq = HASH::s_seq;
                return;
            }
/*            if( goodSlot->hashDepth.getBound() != hashf && hashf != hashfEXACT && goodSlot->hashDepth.getBound() != hashfEXACT)
                hashf = hashf;
            if( goodSlot->hashDepth.getBound() == hashfBETA && hashf == hashfALPHA && val > goodSlot->hashDepth.val ) {
                hashf = hashfBETA;
                val = goodSlot->hashDepth.val;
            }*/
            break;
        }
#if 0
        // try to find an old slot
        if( (goodSlot->hashDepth.seq == HASH::s_seq)  ) {
            // todo : age is inexact if coming from a previous search
            // we prefer to replace a slot from a previous search
            if( (newSlot->hashDepth.seq != HASH::s_seq)  )
                goodSlot = newSlot;
            // but if we can't we prefer to replace a slot from a lower search depth 
            else if( (newSlot->hashDepth.getPlyRem() < goodSlot->hashDepth.getPlyRem()) )
                goodSlot = newSlot;
        } else {
            // we are going to replace a slot from a previous search
            // replace the slot with the lowest search depth
            if( (newSlot->hashDepth.seq != HASH::s_seq) && (newSlot->hashDepth.getPlyRem() < goodSlot->hashDepth.getPlyRem()) )
                goodSlot = newSlot;
        }
#else
        // in this version I try not to replace entries from the previous search
        // because I really need them
        // s_seq    new.seq deltaSeq    u8()
        //  5       4       1           1
        //  5       3       2           2
        //  255     254     1           1
        //  0       255     -255        1
        //  1       255     -254        2
        //  10      11      -1          255
        // note that the hash memory is not initialized
        int deltaSeq = U8(HASH::s_seq - newSlot->hashDepth.seq);  // hanlde wrap around
        int score = 256*(deltaSeq);
//        score += goodSlot->hashDepth.getPlyRem() - newSlot->hashDepth.getPlyRem();
        score -= newSlot->hashDepth.getPlyRem();
        if( score > bestScore ) {
            bestScore = score;
            goodSlot = newSlot;
        }
#endif
    }
    PHASH phash = &goodSlot->hashDepth;
#if 0
    stats.Hdepthinsert++;
    if( s_seq == phash->seq )
        if( phash->hashk != thisHashSig)
            stats.Hcollide++;
#endif
    // else it's an update
    if( HASH::s_seq != phash->seq )
        stats.Hused++;
    Assert(val >= -32768 && val <= 32767);
    phash->val = short(val);
    if (cm.m != 0) {
        phash->setMove( cm.m );
    } else if (phash->hashkSig != thisHashSig)
        phash->setMove( 0 );
    phash->hashkSig = thisHashSig;
    phash->setBound( hashf );
//    phash->setThreat( hasThreat );
    phash->setForced( forced );
    phash->setExt( ext );
    phash->count = nodes;
    Assert( plyrem >= -1 && plyrem < 125 );
    phash->setPlyRem( plyrem );
    phash->seq = HASH::s_seq;
}

void RecordHashValue(PCON pcon, PSTE pste, const CM &cm, int val, int Alpha, int Beta, int depth, bool hasThreat)
{
    hashBound hashf = calcBoundType(val, Alpha, Beta);
    RecordHashValue(pcon, pste, cm, val, hashf, depth);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	I didn't want to export this global throughout the rest of the project.

void VIncrementHashSeq(void)
{
	HASH::s_seq++;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Zero all hash memory.

void VClearHashe(void)
{
    if( chashdMax )
	    memset(s_rghashd, 0, (chashdMax) * sizeof(HASHD));
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	"rand()" produces 15-bit values (0..32767).  I want 64 bits.
#pragma optimize( "", off ) 

static U64 U64Rand(void)
{
	return (U64)rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30);
}
#pragma optimize( "", on ) 

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Fills up the hash key table with random gibberish.  These will be XOR'd
//	together in order to create the semi-unique hash key for any given
//	position.  They need to be as random as possible otherwise there will be
//	too many hash collisions.

//	I've never cared about hash collisions.  If they are a bigger problem
//	than I think, you might want to make this more random.

void VInitHashk(void)
{
	for (int fil = filA; fil <= filH; fil++)
		s_arghashkEnP[fil] = U64Rand();
	for (int cf = 0; cf < cfMAX; cf++)
		s_arghashkCf[cf] = U64Rand();
	for (int pc = 0; pc <= 15; pc++)
        for (int isq = 0; isq < csqMAX; isq++) {
			s_arghashkBB[pc][isq] = U64Rand();
        }
    if( evalCache == 0 )
        evalCache = new EvalCache(1 << 14);
//        evalCache = new EvalCache(1 << 15);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This inits the main transposition hash table.  As of now this function
//	uses a constant for the table size, but this could easily be changed to
//	allow a variable.

//	The table must be a power of two in size or things will break elsewhere,
//	because I don't use "key % size" to get the element, I use
//	"key & (size - 1)", which only works with a power of two.

bool FInitHashe(PCON pcon)
{
    if( hashBuffer ) {
        free( hashBuffer );
        hashBuffer = 0;
    }
	s_chashdMac = 15;
    int cbMaxHash = uciHash.get_spin();

    if( cbMaxHash <= 0 || cbMaxHash > 1024 )
        cbMaxHash = 32;
    cbMaxHash *= 1024*1024;
	chashdMax = 1;
	for (;;) {
		if (chashdMax * 2 * (int)sizeof(HASHD) > cbMaxHash)
			break;
		chashdMax *= 2;
	}
	VPrSendComment("%d Kbytes main hash memory (%d entries)", chashdMax * sizeof(HASHD) / 1024, chashdMax);
    Assert( sizeof(HASHD) == 16 );
    hashBuffer = (RGHASHD) malloc( 64 + chashdMax * sizeof(HASHD));
	if ( hashBuffer == NULL) {
		VPrSendComment("Can't allocate hash memory: %d bytes",
			chashdMax * sizeof(HASHD));
		return false;
	}
    s_rghashd = (RGHASHD) ((U64(hashBuffer)+63) & (~63));
	s_chashdMac = ((chashdMax / BUCKET_SIZE) - 1) * BUCKET_SIZE;
    stats.Hsize = chashdMax;
	VClearHashe();
	return true;
}


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

EvalCache::EvalCache(int _numEntries) {
	VPrSendComment("%d Kbytes eval cache memory (%d entries)", _numEntries * sizeof(EvalCacheEntry) / 1024, _numEntries);
    numEntries = _numEntries;
    // TODO : alignment, sizeof(EvalCacheEntry)
    cache = new EvalCacheEntry[ numEntries ];
}
const EvalCacheEntry *EvalCache::find(const STE *pste) {
    int idx = int(pste->hashkPc & (numEntries - 1));
    if( cache[idx].hashk == pste->hashkPc )
        return &cache[idx];
    else
        return 0;
}
void EvalCache::set(const STE *pste, int val) {
    int idx = int(pste->hashkPc & (numEntries - 1));
    cache[idx].hashk = pste->hashkPc;
    cache[idx].val = val;
}
EvalCache::~EvalCache() {
    delete [] cache;
}
