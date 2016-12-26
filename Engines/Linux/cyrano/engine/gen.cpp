
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
#include "moves.hpp"
#include "eval.hpp"

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif


unsigned int MVV_LVA[16][16];

// Another set of values for the pieces
// Here knight and bishop share the same value because anyway any kind of
// exchange evaluator does not really take care of the position
// We just want to know if we loose/win material or not, we don't care if we have
// changed a bad bishop for a good knight for example or if we made doubled pawns
#define exchangePieceValueMAX 2000

const int exchangePieceValue[16] = {
	2000,   // 50
	2000,   // 50
	valQUEEN,
	valQUEEN,
	valROOK,
	valROOK,
	valBISHOP,
	valBISHOP,
	valBISHOP,
	valBISHOP,
	valPAWN,
	valPAWN,
	valPAWN,    // EP
	valPAWN,    // EP
    0,
    0
};


// Precompute the MVV_LVA array so that I can try some funny things without changing code
// everywhere. Anyway to compute MVV/LVA I would have to do 2 array access, now I only need
// one access

void initMVVLVA(const CON * pcon) {

    for(int capturingPiece = KING; capturingPiece <= EP_CAPT + 1; capturingPiece++)
        for(int capturedPiece = KING; capturedPiece <= EP_CAPT + 1; capturedPiece++) {
            int valCaptured = exchangePieceValue[capturedPiece];
            int valCapturing = exchangePieceValue[capturingPiece];
#if 0
            // scheme 1 the smallest capturing piece first
            // pxQ > pxR > bxQ > bxR > rxB > rxP > qxQ > qxP
            int mvv_lva = valCaptured - valCapturing;
            unsigned int goodCap = (mvv_lva > 0) ? cmkGOODCAPT : ((mvv_lva == 0) ? cmkEQUCAPT : cmkBADCAPT);
//            unsigned int goodCap = (mvv_lva >= 0) ? cmkGOODCAPT : cmkBADCAPT;
            // store the result, the constant is there to keep numbers positive
            MVV_LVA[capturingPiece][capturedPiece] = goodCap | (mvv_lva + exchangePieceValueMAX);
//            MVV_LVA[capturingPiece][capturedPiece] = cmkBADCAPT;
#else
            // scheme 2 the biggest taken piece first
            // pxQ > bxQ > qxQ > pxR > bxR > rxR > rxP > qxP
//            int mvv_lva = valCaptured - valCapturing;
            int rankCaptured = 16 - (capturedPiece>>1);
            int rankCapturing = 16 - (capturingPiece>>1);
            int mvv_lva = valCaptured - valCapturing;
            unsigned int goodCap = (mvv_lva >= 0) ? cmkGOODCAPT : cmkBADCAPT;
//            mvv_lva = valCaptured - valCapturing / 10;
            mvv_lva = rankCaptured * 16 - rankCapturing + 16;
            Assert( mvv_lva >= 0 );
            MVV_LVA[capturingPiece][capturedPiece] = goodCap | mvv_lva;
            //printf(" %d x %d = %x | %d\n", capturingPiece, capturedPiece, goodCap, mvv_lva);
#endif

        }

}
