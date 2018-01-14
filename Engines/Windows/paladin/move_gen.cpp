// implementation of methods in BitBoradUtils class specifically related to move generation
#include "chess.h"

// adds the given board to list and increments the move counter
void BitBoardUtils::addMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *newBoard)
{
    **newPos = *newBoard;
    (*newPos)++;
    (*nMoves)++;
}


void BitBoardUtils::addSlidingMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos,
                                   uint64 src, uint64 dst, uint8 chance)
{

    HexaBitBoardPosition newBoard;

    // remove the dst from all bitboards
    newBoard.bishopQueens   = pos->bishopQueens & ~dst;
    newBoard.rookQueens     = pos->rookQueens   & ~dst;
    newBoard.kings          = pos->kings        & ~dst;
    newBoard.knights        = pos->knights      & ~dst;
    newBoard.pawns          = pos->pawns        & ~(dst & RANKS2TO7);

    // figure out if the piece was a bishop, rook, or a queen
    uint64 isBishop = newBoard.bishopQueens & src;
    uint64 isRook   = newBoard.rookQueens   & src;

    // remove src from the appropriate board / boards if queen
    newBoard.bishopQueens ^= isBishop;
    newBoard.rookQueens   ^= isRook;

    // add dst
    newBoard.bishopQueens |= isBishop ? dst : 0;
    newBoard.rookQueens   |= isRook   ? dst : 0;

    if (chance == WHITE)
    {
        newBoard.whitePieces = (pos->whitePieces ^ src) | dst;
    }
    else
    {
        newBoard.whitePieces = pos->whitePieces  & ~dst;
    }

    // update game state (the old game state already got copied over above when copying pawn bitboard)
    newBoard.chance = !chance;
    newBoard.enPassent = 0;
    //newBoard.halfMoveCounter++;   // quiet move -> increment half move counter // TODO: correctly increment this based on if there was a capture

    // need to update castle flag for both sides (src moved in same side, and dst move on other side)
    updateCastleFlag(&newBoard, dst, chance);
    updateCastleFlag(&newBoard, src, !chance);

    // add the move
    addMove(nMoves, newPos, &newBoard);
}


void BitBoardUtils::addKnightMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance)
{
    HexaBitBoardPosition newBoard;

    // remove the dst from all bitboards
    newBoard.bishopQueens   = pos->bishopQueens & ~dst;
    newBoard.rookQueens     = pos->rookQueens   & ~dst;
    newBoard.kings          = pos->kings        & ~dst;
    newBoard.pawns          = pos->pawns        & ~(dst & RANKS2TO7);

    // remove src and add destination
    newBoard.knights = (pos->knights ^ src) | dst;

    if (chance == WHITE)
    {
        newBoard.whitePieces = (pos->whitePieces ^ src) | dst;
    }
    else
    {
        newBoard.whitePieces = pos->whitePieces  & ~dst;
    }

    // update game state (the old game state already got copied over above when copying pawn bitboard)
    newBoard.chance = !chance;
    newBoard.enPassent = 0;
    //newBoard.halfMoveCounter++;   // quiet move -> increment half move counter // TODO: correctly increment this based on if there was a capture
    updateCastleFlag(&newBoard, dst, chance);

    // add the move
    addMove(nMoves, newPos, &newBoard);
}


void BitBoardUtils::addKingMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance)
{
    HexaBitBoardPosition newBoard;

    // remove the dst from all bitboards
    newBoard.bishopQueens   = pos->bishopQueens & ~dst;
    newBoard.rookQueens     = pos->rookQueens   & ~dst;
    newBoard.knights        = pos->knights      & ~dst;
    newBoard.pawns          = pos->pawns        & ~(dst & RANKS2TO7);

    // remove src and add destination
    newBoard.kings = (pos->kings ^ src) | dst;

    if (chance == WHITE)
    {
        newBoard.whitePieces = (pos->whitePieces ^ src) | dst;
        newBoard.whiteCastle = 0;
    }
    else
    {
        newBoard.whitePieces = pos->whitePieces  & ~dst;
        newBoard.blackCastle = 0;
    }

    // update game state (the old game state already got copied over above when copying pawn bitboard)
    newBoard.chance = !chance;
    newBoard.enPassent = 0;
    // newBoard.halfMoveCounter++;   // quiet move -> increment half move counter (TODO: fix this for captures)
    updateCastleFlag(&newBoard, dst, chance);

    // add the move
    addMove(nMoves, newPos, &newBoard);
}


void BitBoardUtils::addCastleMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos,
                                  uint64 kingFrom, uint64 kingTo, uint64 rookFrom, uint64 rookTo, uint8 chance)
{
    HexaBitBoardPosition newBoard;
    newBoard.bishopQueens   = pos->bishopQueens;
    newBoard.pawns          = pos->pawns;
    newBoard.knights        = pos->knights;
    newBoard.kings          = (pos->kings ^ kingFrom)      | kingTo;
    newBoard.rookQueens     = (pos->rookQueens ^ rookFrom) | rookTo;

    newBoard.chance = !chance;
    newBoard.enPassent = 0;
    newBoard.halfMoveCounter = 0;
    if (chance == WHITE)
    {
        newBoard.whitePieces = (pos->whitePieces ^ (kingFrom | rookFrom)) | (kingTo | rookTo);
        newBoard.whiteCastle = 0;
    }
    else
    {
        newBoard.blackCastle = 0;
        newBoard.whitePieces = pos->whitePieces;
    }

    // add the move
    addMove(nMoves, newPos, &newBoard);

}


// only for normal moves
// promotions and en-passent handled in seperate functions
void BitBoardUtils::addSinglePawnMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos,
                                      uint64 src, uint64 dst, uint8 chance, bool doublePush, uint8 pawnIndex)
{
    HexaBitBoardPosition newBoard;

    // remove the dst from all bitboards
    newBoard.bishopQueens   = pos->bishopQueens & ~dst;
    newBoard.rookQueens     = pos->rookQueens   & ~dst;
    newBoard.knights        = pos->knights      & ~dst;
    newBoard.kings          = pos->kings;         // no need to remove dst from kings bitboard as you can't kill a king

    // remove src and add destination
    newBoard.pawns = (pos->pawns ^ src) | dst;
    if (chance == WHITE)
    {
        newBoard.whitePieces = (pos->whitePieces ^ src) | dst;
    }
    else
    {
        newBoard.whitePieces = pos->whitePieces  & ~dst;
    }

    // no need to update castle flag if the captured piece is a rook
    // as normal pawn moves (except for promotion) can't capture a rook from it's base position

    // update game state (the old game state already got copied over above when copying pawn bitboard)
    newBoard.chance = !chance;
    if (doublePush)
    {
        newBoard.enPassent = (pawnIndex & 7) + 1;   // store file + 1
    }
    else
    {
        newBoard.enPassent = 0;
    }

    newBoard.halfMoveCounter = 0;   // reset half move counter for pawn push

    // add the move
    addMove(nMoves, newPos, &newBoard);
}

void BitBoardUtils::addEnPassentMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance)
{
    HexaBitBoardPosition newBoard;

    uint64 capturedPiece = (chance == WHITE) ? southOne(dst) : northOne(dst);

    newBoard.bishopQueens = pos->bishopQueens;
    newBoard.rookQueens = pos->rookQueens;
    newBoard.knights = pos->knights;
    newBoard.kings = pos->kings;


    // remove src and captured piece. Add destination 
    newBoard.pawns = (pos->pawns ^ (capturedPiece | src)) | dst;
    if (chance == WHITE)
    {
        newBoard.whitePieces = (pos->whitePieces ^ src) | dst;
    }
    else
    {
        newBoard.whitePieces = pos->whitePieces  ^ capturedPiece;
    }

    // update game state (the old game state already got copied over above when copying pawn bitboard)
    newBoard.chance = !chance;
    newBoard.halfMoveCounter = 0;   // reset half move counter for en-passent
    newBoard.enPassent = 0;

    // no need to update castle flag for en-passent

    // add the move
    addMove(nMoves, newPos, &newBoard);
}

// adds promotions if at promotion square
// or normal pawn moves if not promotion. Never called for double pawn push (the above function is called directly)
void BitBoardUtils::addPawnMoves(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance)
{
    // promotion
    if (dst & (RANK1 | RANK8))
    {
        HexaBitBoardPosition newBoard;

        // remove the dst from all bitboards
        newBoard.kings = pos->kings;         // no need to remove dst from kings bitboard as you can't kill a king

        // remove src and add dst
        if (chance == WHITE)
        {
            newBoard.whitePieces = (pos->whitePieces ^ src) | dst;
        }
        else
        {
            newBoard.whitePieces = pos->whitePieces  & ~dst;
        }

        // remove src pawn
        newBoard.pawns = (pos->pawns ^ src);

        // update game state (the old game state already got copied over above when copying pawn bitboard)
        newBoard.chance = !chance;
        newBoard.enPassent = 0;
        newBoard.halfMoveCounter = 0;   // reset half move counter for pawn push
        updateCastleFlag(&newBoard, dst, chance);

        // add the moves (TODO: add promotion to queen on top! - for better move ordering)
        // 1. promotion to knight
        newBoard.knights = pos->knights | dst;
        newBoard.bishopQueens = pos->bishopQueens & ~dst;
        newBoard.rookQueens = pos->rookQueens   & ~dst;
        addMove(nMoves, newPos, &newBoard);

        // 2. promotion to bishop
        newBoard.knights = pos->knights      & ~dst;
        newBoard.bishopQueens = pos->bishopQueens | dst;
        newBoard.rookQueens = pos->rookQueens   & ~dst;
        addMove(nMoves, newPos, &newBoard);

        // 3. promotion to queen
        newBoard.rookQueens = pos->rookQueens | dst;
        addMove(nMoves, newPos, &newBoard);

        // 4. promotion to rook
        newBoard.bishopQueens = pos->bishopQueens & ~dst;
        addMove(nMoves, newPos, &newBoard);

    }
    else
    {
        // pawn index is used only for double-pushes (to set en-passent square)
        addSinglePawnMove(nMoves, newPos, pos, src, dst, chance, false, 0);
    }
}


// called from generateMoves function (the above ones are called from generateBoards)
void BitBoardUtils::addCompactMove(int *nMoves, CMove **genMoves, uint8 from, uint8 to, uint8 flags)
{
    CMove move(from, to, flags);
    **genMoves = move;
    (*genMoves)++;
    (*nMoves)++;
}

// adds promotions if at promotion square
// or normal pawn moves if not promotion
void BitBoardUtils::addCompactPawnMoves(int *nMoves, CMove **genMoves, uint8 from, uint64 dst, uint8 flags)
{
    uint8 to = bitScan(dst);

    // promotion
    if (dst & (RANK1 | RANK8))
    {
        addCompactMove(nMoves, genMoves, from, to, flags | CM_FLAG_QUEEN_PROMOTION);
        addCompactMove(nMoves, genMoves, from, to, flags | CM_FLAG_KNIGHT_PROMOTION);
        addCompactMove(nMoves, genMoves, from, to, flags | CM_FLAG_ROOK_PROMOTION);
        addCompactMove(nMoves, genMoves, from, to, flags | CM_FLAG_BISHOP_PROMOTION);
    }
    else
    {
        addCompactMove(nMoves, genMoves, from, to, flags);
    }
}

template<uint8 chance>
int BitBoardUtils::generateBoardsOutOfCheck(HexaBitBoardPosition *pos, HexaBitBoardPosition *newPositions,
                                            uint64 allPawns, uint64 allPieces, uint64 myPieces,
                                            uint64 enemyPieces, uint64 pinned, uint64 threatened,
                                            uint8 kingIndex)
{
    int nMoves = 0;
    uint64 king = pos->kings & myPieces;

    // figure out the no. of attackers 
    uint64 attackers = 0;

    // pawn attacks
    uint64 enemyPawns = allPawns & enemyPieces;
    attackers |= ((chance == WHITE) ? (northEastOne(king) | northWestOne(king)) :
                                      (southEastOne(king) | southWestOne(king))) & enemyPawns;

    // knight attackers
    uint64 enemyKnights = pos->knights & enemyPieces;
    attackers |= knightAttacks(king) & enemyKnights;

    // bishop attackers
    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    attackers |= bishopAttacks(king, ~allPieces) & enemyBishops;

    // rook attackers
    uint64 enemyRooks = pos->rookQueens & enemyPieces;
    attackers |= rookAttacks(king, ~allPieces) & enemyRooks;


    // A. Try king moves to get the king out of check
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(kingIndex);
#else
    uint64 kingMoves = kingAttacks(king);
#endif

    kingMoves &= ~(threatened | myPieces);  // king can't move to a square under threat or a square containing piece of same side
    while (kingMoves)
    {
        uint64 dst = getOne(kingMoves);
        addKingMove(&nMoves, &newPositions, pos, king, dst, chance);
        kingMoves ^= dst;
    }


    // B. try moves to kill/block attacking pieces
    if (isSingular(attackers))
    {
        // Find the safe squares - i.e, if a dst square of a move is any of the safe squares, 
        // it will take king out of check

        // for pawn and knight attack, the only option is to kill the attacking piece
        // for bishops rooks and queens, it's the line between the attacker and the king, including the attacker
        uint64 safeSquares = attackers | sqsInBetween(kingIndex, bitScan(attackers));

        // pieces that are pinned don't have any hope of saving the king
        // TODO: Think more about it
        myPieces &= ~pinned;

        // 1. pawn moves
        uint64 myPawns = allPawns & myPieces;

        // checking rank for pawn double pushes
        uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

        uint64 enPassentTarget = 0;
        if (pos->enPassent)
        {
            if (chance == BLACK)
            {
                enPassentTarget = BIT(pos->enPassent - 1) << (8 * 2);
            }
            else
            {
                enPassentTarget = BIT(pos->enPassent - 1) << (8 * 5);
            }
        }

        // en-passent can only save the king if the piece captured is the attacker
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);
        if (enPassentCapturedPiece != attackers)
            enPassentTarget = 0;

        while (myPawns)
        {
            uint64 pawn = getOne(myPawns);

            // pawn push
            uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~allPieces);
            if (dst)
            {
                if (dst & safeSquares)
                {
                    addPawnMoves(&nMoves, &newPositions, pos, pawn, dst, chance);
                }
                else
                {
                    // double push (only possible if single push was possible and single push didn't save the king)
                    dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                        southOne(dst & checkingRankDoublePush)) & (safeSquares)&(~allPieces);

                    if (dst)
                    {
                        addSinglePawnMove(&nMoves, &newPositions, pos, pawn, dst, chance, true, bitScan(pawn));
                    }
                }
            }

            // captures (only one of the two captures will save the king.. if at all it does)
            uint64 westCapture = (chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn);
            uint64 eastCapture = (chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn);
            dst = (westCapture | eastCapture) & enemyPieces & safeSquares;
            if (dst)
            {
                addPawnMoves(&nMoves, &newPositions, pos, pawn, dst, chance);
            }

            // en-passent 
            dst = (westCapture | eastCapture) & enPassentTarget;
            if (dst)
            {
                addEnPassentMove(&nMoves, &newPositions, pos, pawn, dst, chance);
            }

            myPawns ^= pawn;
        }

        // 2. knight moves
        uint64 myKnights = (pos->knights & myPieces);
        while (myKnights)
        {
            uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
            uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & safeSquares;
#else
            uint64 knightMoves = knightAttacks(knight) & safeSquares;
#endif
            while (knightMoves)
            {
                uint64 dst = getOne(knightMoves);
                addKnightMove(&nMoves, &newPositions, pos, knight, dst, chance);
                knightMoves ^= dst;
            }
            myKnights ^= knight;
        }

        // 3. bishop moves
        uint64 bishops = pos->bishopQueens & myPieces;
        while (bishops)
        {
            uint64 bishop = getOne(bishops);
            uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & safeSquares;

            while (bishopMoves)
            {
                uint64 dst = getOne(bishopMoves);
                addSlidingMove(&nMoves, &newPositions, pos, bishop, dst, chance);
                bishopMoves ^= dst;
            }
            bishops ^= bishop;
        }

        // 4. rook moves
        uint64 rooks = pos->rookQueens & myPieces;
        while (rooks)
        {
            uint64 rook = getOne(rooks);
            uint64 rookMoves = rookAttacks(rook, ~allPieces) & safeSquares;

            while (rookMoves)
            {
                uint64 dst = getOne(rookMoves);
                addSlidingMove(&nMoves, &newPositions, pos, rook, dst, chance);
                rookMoves ^= dst;
            }
            rooks ^= rook;
        }

    }   // end of if single attacker
    else
    {
        // multiple threats => only king moves possible
    }

    return nMoves;
}



template <uint8 chance>
int BitBoardUtils::generateBoards(HexaBitBoardPosition *pos, HexaBitBoardPosition *newPositions)
{
    int nMoves = 0;

    uint64 allPawns = pos->pawns & RANKS2TO7;    // get rid of game state variables

    uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;
    uint64 blackPieces = allPieces & (~pos->whitePieces);

    uint64 myPieces = (chance == WHITE) ? pos->whitePieces : blackPieces;
    uint64 enemyPieces = (chance == WHITE) ? blackPieces : pos->whitePieces;

    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    uint64 enemyRooks = pos->rookQueens & enemyPieces;

    uint64 myKing = pos->kings & myPieces;
    uint8  kingIndex = bitScan(myKing);

    uint64 pinned = findPinnedPieces(pos->kings & myPieces, enemyBishops, enemyRooks, allPieces, kingIndex);

    uint64 threatened = findAttackedSquares(~allPieces, enemyBishops, enemyRooks, allPawns & enemyPieces,
                                            pos->knights & enemyPieces, pos->kings & enemyPieces,
                                            myKing, !chance);



    // king is in check: call special generate function to generate only the moves that take king out of check
    if (threatened & (pos->kings & myPieces))
    {
        return generateBoardsOutOfCheck<chance>(pos, newPositions, allPawns, allPieces, myPieces, enemyPieces,
            pinned, threatened, kingIndex);
    }

    uint64 myPawns = allPawns & myPieces;

    // 0. generate en-passent moves first
    uint64 enPassentTarget = 0;
    if (pos->enPassent)
    {
        if (chance == BLACK)
        {
            enPassentTarget = BIT(pos->enPassent - 1) << (8 * 2);
        }
        else
        {
            enPassentTarget = BIT(pos->enPassent - 1) << (8 * 5);
        }
    }

    if (enPassentTarget)
    {
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);

        uint64 epSources = (eastOne(enPassentCapturedPiece) | westOne(enPassentCapturedPiece)) & myPawns;

        while (epSources)
        {
            uint64 pawn = getOne(epSources);
            if (pawn & pinned)
            {
                // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
                uint64 line = sqsInLine(bitScan(pawn), kingIndex);

                if (enPassentTarget & line)
                {
                    addEnPassentMove(&nMoves, &newPositions, pos, pawn, enPassentTarget, chance);
                }
            }
            else
                /*if (!(enPassentCapturedPiece & pinned))*/
                // the captured pawn should not be pinned in diagonal direction but it can be in vertical dir.
                // the diagonal pinning can't happen for enpassent in real chess game, so anyways it's not vaild
            {
                uint64 propogator = (~allPieces) | enPassentCapturedPiece | pawn;
                uint64 causesCheck = (eastAttacks(enemyRooks, propogator) | westAttacks(enemyRooks, propogator)) &
                    (pos->kings & myPieces);
                if (!causesCheck)
                {
                    addEnPassentMove(&nMoves, &newPositions, pos, pawn, enPassentTarget, chance);
                }
            }
            epSources ^= pawn;
        }
    }

    // 1. pawn moves

    // checking rank for pawn double pushes
    uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

    // first deal with pinned pawns
    uint64 pinnedPawns = myPawns & pinned;

    while (pinnedPawns)
    {
        uint64 pawn = getOne(pinnedPawns);
        uint8 pawnIndex = bitScan(pawn);    // same as bitscan on pinnedPawns

        // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
        uint64 line = sqsInLine(pawnIndex, kingIndex);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & line & (~allPieces);
        if (dst)
        {
            addSinglePawnMove(&nMoves, &newPositions, pos, pawn, dst, chance, false, pawnIndex);

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                southOne(dst & checkingRankDoublePush)) & (~allPieces);
            if (dst)
            {
                addSinglePawnMove(&nMoves, &newPositions, pos, pawn, dst, chance, true, pawnIndex);
            }
        }

        // captures
        // (either of them will be valid - if at all)
        dst = ((chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn)) & line;
        dst |= ((chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn)) & line;

        if (dst & enemyPieces)
        {
            addPawnMoves(&nMoves, &newPositions, pos, pawn, dst, chance);
        }

        pinnedPawns ^= pawn;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
    }

    myPawns = myPawns & ~pinned;

    while (myPawns)
    {
        uint64 pawn = getOne(myPawns);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~allPieces);
        if (dst)
        {
            addPawnMoves(&nMoves, &newPositions, pos, pawn, dst, chance);

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                southOne(dst & checkingRankDoublePush)) & (~allPieces);

            if (dst) addSinglePawnMove(&nMoves, &newPositions, pos, pawn, dst, chance, true, bitScan(pawn));
        }

        // captures
        uint64 westCapture = (chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn);
        dst = westCapture & enemyPieces;
        if (dst) addPawnMoves(&nMoves, &newPositions, pos, pawn, dst, chance);

        uint64 eastCapture = (chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn);
        dst = eastCapture & enemyPieces;
        if (dst) addPawnMoves(&nMoves, &newPositions, pos, pawn, dst, chance);

        myPawns ^= pawn;
    }

    // generate castling moves
    if (chance == WHITE)
    {
        if ((pos->whiteCastle & CASTLE_FLAG_KING_SIDE) &&   // castle flag is set
            !(F1G1 & allPieces) &&                          // squares between king and rook are empty
            !(F1G1 & threatened))                           // and not in threat from enemy pieces
        {
            // white king side castle
            addCastleMove(&nMoves, &newPositions, pos, BIT(E1), BIT(G1), BIT(H1), BIT(F1), chance);
        }
        if ((pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE) &&  // castle flag is set
            !(B1D1 & allPieces) &&                          // squares between king and rook are empty
            !(C1D1 & threatened))                           // and not in threat from enemy pieces
        {
            // white queen side castle
            addCastleMove(&nMoves, &newPositions, pos, BIT(E1), BIT(C1), BIT(A1), BIT(D1), chance);
        }
    }
    else
    {
        if ((pos->blackCastle & CASTLE_FLAG_KING_SIDE) &&   // castle flag is set
            !(F8G8 & allPieces) &&                          // squares between king and rook are empty
            !(F8G8 & threatened))                           // and not in threat from enemy pieces
        {
            // black king side castle
            addCastleMove(&nMoves, &newPositions, pos, BIT(E8), BIT(G8), BIT(H8), BIT(F8), chance);
        }
        if ((pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE) &&  // castle flag is set
            !(B8D8 & allPieces) &&                          // squares between king and rook are empty
            !(C8D8 & threatened))                           // and not in threat from enemy pieces
        {
            // black queen side castle
            addCastleMove(&nMoves, &newPositions, pos, BIT(E8), BIT(C8), BIT(A8), BIT(D8), chance);
        }
    }

    // generate king moves
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(kingIndex);
#else
    uint64 kingMoves = kingAttacks(myKing);
#endif

    kingMoves &= ~(threatened | myPieces);  // king can't move to a square under threat or a square containing piece of same side
    while (kingMoves)
    {
        uint64 dst = getOne(kingMoves);
        addKingMove(&nMoves, &newPositions, pos, myKing, dst, chance);
        kingMoves ^= dst;
    }

    // generate knight moves (only non-pinned knights can move)
    uint64 myKnights = (pos->knights & myPieces) & ~pinned;
    while (myKnights)
    {
        uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
        uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~myPieces;
#else
        uint64 knightMoves = knightAttacks(knight) & ~myPieces;
#endif
        while (knightMoves)
        {
            uint64 dst = getOne(knightMoves);
            addKnightMove(&nMoves, &newPositions, pos, knight, dst, chance);
            knightMoves ^= dst;
        }
        myKnights ^= knight;
    }


    // generate bishop (and queen) moves
    uint64 myBishops = pos->bishopQueens & myPieces;

    // first deal with pinned bishops
    uint64 bishops = myBishops & pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        // TODO: bishopAttacks() function uses a kogge-stone sliding move generator. Switch to magics!
        uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & ~myPieces;
        bishopMoves &= sqsInLine(bitScan(bishop), kingIndex);    // pined sliding pieces can move only along the line

        while (bishopMoves)
        {
            uint64 dst = getOne(bishopMoves);
            addSlidingMove(&nMoves, &newPositions, pos, bishop, dst, chance);
            bishopMoves ^= dst;
        }
        bishops ^= bishop;
    }

    // remaining bishops/queens
    bishops = myBishops & ~pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & ~myPieces;

        while (bishopMoves)
        {
            uint64 dst = getOne(bishopMoves);
            addSlidingMove(&nMoves, &newPositions, pos, bishop, dst, chance);
            bishopMoves ^= dst;
        }
        bishops ^= bishop;

    }


    // rook/queen moves
    uint64 myRooks = pos->rookQueens & myPieces;

    // first deal with pinned rooks
    uint64 rooks = myRooks & pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~allPieces) & ~myPieces;
        rookMoves &= sqsInLine(bitScan(rook), kingIndex);    // pined sliding pieces can move only along the line

        while (rookMoves)
        {
            uint64 dst = getOne(rookMoves);
            addSlidingMove(&nMoves, &newPositions, pos, rook, dst, chance);
            rookMoves ^= dst;
        }
        rooks ^= rook;
    }

    // remaining rooks/queens
    rooks = myRooks & ~pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~allPieces) & ~myPieces;

        while (rookMoves)
        {
            uint64 dst = getOne(rookMoves);
            addSlidingMove(&nMoves, &newPositions, pos, rook, dst, chance);
            rookMoves ^= dst;
        }
        rooks ^= rook;

    }

    return nMoves;
}

template<uint8 chance>
int BitBoardUtils::countMovesOutOfCheck(HexaBitBoardPosition *pos, uint64 allPawns, uint64 allPieces, uint64 myPieces, uint64 enemyPieces, uint64 pinned, uint64 threatened, uint8 kingIndex)
{
    int nMoves = 0;
    uint64 king = pos->kings & myPieces;

    // figure out the no. of attackers 
    uint64 attackers = 0;

    // pawn attacks
    uint64 enemyPawns = allPawns & enemyPieces;
    attackers |= ((chance == WHITE) ? (northEastOne(king) | northWestOne(king)) :
                                      (southEastOne(king) | southWestOne(king))) & enemyPawns;

    // knight attackers
    uint64 enemyKnights = pos->knights & enemyPieces;
    attackers |= knightAttacks(king) & enemyKnights;

    // bishop attackers
    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    attackers |= bishopAttacks(king, ~allPieces) & enemyBishops;

    // rook attackers
    uint64 enemyRooks = pos->rookQueens & enemyPieces;
    attackers |= rookAttacks(king, ~allPieces) & enemyRooks;


    // A. Try king moves to get the king out of check
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(kingIndex);
#else
    uint64 kingMoves = kingAttacks(king);
#endif

    kingMoves &= ~(threatened | myPieces);  // king can't move to a square under threat or a square containing piece of same side
    nMoves += popCount(kingMoves);

    // B. try moves to kill/block attacking pieces
    if (isSingular(attackers))
    {
        // Find the safe squares - i.e, if a dst square of a move is any of the safe squares, 
        // it will take king out of check

        // for pawn and knight attack, the only option is to kill the attacking piece
        // for bishops rooks and queens, it's the line between the attacker and the king, including the attacker
        uint64 safeSquares = attackers | sqsInBetween(kingIndex, bitScan(attackers));

        // pieces that are pinned don't have any hope of saving the king
        // TODO: Think more about it
        myPieces &= ~pinned;

        // 1. pawn moves
        uint64 myPawns = allPawns & myPieces;

        // checking rank for pawn double pushes
        uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

        uint64 enPassentTarget = 0;
        if (pos->enPassent)
        {
            if (chance == BLACK)
            {
                enPassentTarget = BIT(pos->enPassent - 1) << (8 * 2);
            }
            else
            {
                enPassentTarget = BIT(pos->enPassent - 1) << (8 * 5);
            }
        }

        // en-passent can only save the king if the piece captured is the attacker
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);
        if (enPassentCapturedPiece != attackers)
            enPassentTarget = 0;

        while (myPawns)
        {
            uint64 pawn = getOne(myPawns);

            // pawn push
            uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~allPieces);
            if (dst)
            {
                if (dst & safeSquares)
                {
                    if (dst & (RANK1 | RANK8))
                        nMoves += 4;    // promotion
                    else
                        nMoves++;
                }
                else
                {
                    // double push (only possible if single push was possible and single push didn't save the king)
                    dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                        southOne(dst & checkingRankDoublePush)) & (safeSquares)&(~allPieces);

                    if (dst)
                    {
                        nMoves++;
                    }
                }
            }

            // captures (only one of the two captures will save the king.. if at all it does)
            uint64 westCapture = (chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn);
            uint64 eastCapture = (chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn);
            dst = (westCapture | eastCapture) & enemyPieces & safeSquares;
            if (dst)
            {
                if (dst & (RANK1 | RANK8))
                    nMoves += 4;    // promotion
                else
                    nMoves++;
            }

            // en-passent 
            dst = (westCapture | eastCapture) & enPassentTarget;
            if (dst)
            {
                nMoves++;
            }

            myPawns ^= pawn;
        }

        // 2. knight moves
        uint64 myKnights = (pos->knights & myPieces);
        while (myKnights)
        {
            uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
            uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & safeSquares;
#else
            uint64 knightMoves = knightAttacks(knight) & safeSquares;
#endif
            nMoves += popCount(knightMoves);
            myKnights ^= knight;
        }

        // 3. bishop moves
        uint64 bishops = pos->bishopQueens & myPieces;
        while (bishops)
        {
            uint64 bishop = getOne(bishops);
            uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & safeSquares;

            nMoves += popCount(bishopMoves);
            bishops ^= bishop;
        }

        // 4. rook moves
        uint64 rooks = pos->rookQueens & myPieces;
        while (rooks)
        {
            uint64 rook = getOne(rooks);
            uint64 rookMoves = rookAttacks(rook, ~allPieces) & safeSquares;

            nMoves += popCount(rookMoves);
            rooks ^= rook;
        }

    }   // end of if single attacker
    else
    {
        // multiple threats => only king moves possible
    }

    return nMoves;
}


// count moves for the given board position
// returns the no of moves generated
template <uint8 chance>
int BitBoardUtils::countMoves(HexaBitBoardPosition *pos)
{
    int nMoves = 0;

    uint64 allPawns = pos->pawns & RANKS2TO7;    // get rid of game state variables

    uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;
    uint64 blackPieces = allPieces & (~pos->whitePieces);

    uint64 myPieces = (chance == WHITE) ? pos->whitePieces : blackPieces;
    uint64 enemyPieces = (chance == WHITE) ? blackPieces : pos->whitePieces;

    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    uint64 enemyRooks = pos->rookQueens & enemyPieces;

    uint64 myKing = pos->kings & myPieces;
    uint8  kingIndex = bitScan(myKing);

    uint64 pinned = findPinnedPieces(pos->kings & myPieces, enemyBishops, enemyRooks, allPieces, kingIndex);

    uint64 threatened = findAttackedSquares(~allPieces, enemyBishops, enemyRooks, allPawns & enemyPieces,
                                             pos->knights & enemyPieces, pos->kings & enemyPieces, myKing, !chance);


    // king is in check: call special generate function to generate only the moves that take king out of check
    if (threatened & (pos->kings & myPieces))
    {
        return countMovesOutOfCheck<chance>(pos, allPawns, allPieces, myPieces, enemyPieces,
            pinned, threatened, kingIndex);
    }

    uint64 myPawns = allPawns & myPieces;

    // 0. generate en-passent moves first
    uint64 enPassentTarget = 0;
    if (pos->enPassent)
    {
        if (chance == BLACK)
        {
            enPassentTarget = BIT(pos->enPassent - 1) << (8 * 2);
        }
        else
        {
            enPassentTarget = BIT(pos->enPassent - 1) << (8 * 5);
        }
    }

    if (enPassentTarget)
    {
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);

        uint64 epSources = (eastOne(enPassentCapturedPiece) | westOne(enPassentCapturedPiece)) & myPawns;
        /*
        uint64 epSources = ((chance == WHITE) ? southEastOne(enPassentTarget) | southWestOne(enPassentTarget) :
        northEastOne(enPassentTarget) | northWestOne(enPassentTarget)) & myPawns;
        */

        while (epSources)
        {
            uint64 pawn = getOne(epSources);
            if (pawn & pinned)
            {
                // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
                uint64 line = sqsInLine(bitScan(pawn), kingIndex);

                if (enPassentTarget & line)
                {
                    nMoves++;
                }
            }
            else
                /*if (!(enPassentCapturedPiece & pinned))*/
                // the captured pawn should not be pinned in diagonal direction but it can be in vertical dir.
                // the diagonal pinning can't happen for enpassent in real chess game, so anyways it's not vaild
            {
                uint64 propogator = (~allPieces) | enPassentCapturedPiece | pawn;
                uint64 causesCheck = (eastAttacks(enemyRooks, propogator) | westAttacks(enemyRooks, propogator)) &
                    (pos->kings & myPieces);
                if (!causesCheck)
                {
                    nMoves++;
                }
            }
            epSources ^= pawn;
        }
    }
    // 1. pawn moves

    // checking rank for pawn double pushes
    uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

    // first deal with pinned pawns
    uint64 pinnedPawns = myPawns & pinned;

    while (pinnedPawns)
    {
        uint64 pawn = getOne(pinnedPawns);
        uint8 pawnIndex = bitScan(pawn);    // same as bitscan on pinnedPawns

        // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
        uint64 line = sqsInLine(pawnIndex, kingIndex);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & line & (~allPieces);
        if (dst)
        {
            nMoves++;

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                southOne(dst & checkingRankDoublePush)) & (~allPieces);
            if (dst)
            {
                nMoves++;
            }
        }

        // captures
        // (either of them will be valid - if at all)
        dst = ((chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn)) & line;
        dst |= ((chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn)) & line;

        if (dst & enemyPieces)
        {
            if (dst & (RANK1 | RANK8))
                nMoves += 4;    // promotion
            else
                nMoves++;
        }

        pinnedPawns ^= pawn;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
    }

    myPawns = myPawns & ~pinned;

    // pawn push
    uint64 dsts = ((chance == WHITE) ? northOne(myPawns) : southOne(myPawns)) & (~allPieces);
    nMoves += popCount(dsts);
    uint64 promotions = dsts & (RANK1 | RANK8);
    nMoves += 3 * popCount(promotions);

    // double push
    dsts = ((chance == WHITE) ? northOne(dsts & checkingRankDoublePush) :
        southOne(dsts & checkingRankDoublePush)) & (~allPieces);
    nMoves += popCount(dsts);

    // captures
    dsts = ((chance == WHITE) ? northWestOne(myPawns) : southWestOne(myPawns)) & enemyPieces;
    nMoves += popCount(dsts);
    promotions = dsts & (RANK1 | RANK8);
    nMoves += 3 * popCount(promotions);


    dsts = ((chance == WHITE) ? northEastOne(myPawns) : southEastOne(myPawns)) & enemyPieces;
    nMoves += popCount(dsts);
    promotions = dsts & (RANK1 | RANK8);
    nMoves += 3 * popCount(promotions);

    // generate castling moves
    if (chance == WHITE)
    {
        if ((pos->whiteCastle & CASTLE_FLAG_KING_SIDE) &&   // castle flag is set
            !(F1G1 & allPieces) &&                          // squares between king and rook are empty
            !(F1G1 & threatened))                           // and not in threat from enemy pieces
        {
            // white king side castle
            nMoves++;
        }
        if ((pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE) &&  // castle flag is set
            !(B1D1 & allPieces) &&                          // squares between king and rook are empty
            !(C1D1 & threatened))                           // and not in threat from enemy pieces
        {
            // white queen side castle
            nMoves++;
        }
    }
    else
    {
        if ((pos->blackCastle & CASTLE_FLAG_KING_SIDE) &&   // castle flag is set
            !(F8G8 & allPieces) &&                          // squares between king and rook are empty
            !(F8G8 & threatened))                           // and not in threat from enemy pieces
        {
            // black king side castle
            nMoves++;
        }
        if ((pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE) &&  // castle flag is set
            !(B8D8 & allPieces) &&                          // squares between king and rook are empty
            !(C8D8 & threatened))                           // and not in threat from enemy pieces
        {
            // black queen side castle
            nMoves++;
        }
    }

    // generate king moves
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(kingIndex);
#else
    uint64 kingMoves = kingAttacks(myKing);
#endif

    kingMoves &= ~(threatened | myPieces);  // king can't move to a square under threat or a square containing piece of same side
    nMoves += popCount(kingMoves);

    // generate knight moves (only non-pinned knights can move)
    uint64 myKnights = (pos->knights & myPieces) & ~pinned;
    while (myKnights)
    {
        uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
        uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~myPieces;
#else
        uint64 knightMoves = knightAttacks(knight) & ~myPieces;
#endif
        nMoves += popCount(knightMoves);
        myKnights ^= knight;
    }


    // generate bishop (and queen) moves
    uint64 myBishops = pos->bishopQueens & myPieces;

    // first deal with pinned bishops
    uint64 bishops = myBishops & pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        // TODO: bishopAttacks() function uses a kogge-stone sliding move generator. Switch to magics!
        uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & ~myPieces;
        bishopMoves &= sqsInLine(bitScan(bishop), kingIndex);    // pined sliding pieces can move only along the line

        nMoves += popCount(bishopMoves);
        bishops ^= bishop;
    }

    // remaining bishops/queens
    bishops = myBishops & ~pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & ~myPieces;

        nMoves += popCount(bishopMoves);
        bishops ^= bishop;

    }

    // rook/queen moves
    uint64 myRooks = pos->rookQueens & myPieces;

    // first deal with pinned rooks
    uint64 rooks = myRooks & pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~allPieces) & ~myPieces;
        rookMoves &= sqsInLine(bitScan(rook), kingIndex);    // pined sliding pieces can move only along the line

        nMoves += popCount(rookMoves);
        rooks ^= rook;
    }

    // remaining rooks/queens
    rooks = myRooks & ~pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~allPieces) & ~myPieces;

        nMoves += popCount(rookMoves);
        rooks ^= rook;
    }

    return nMoves;
}


void BitBoardUtils::generateSlidingCapturesForSquare(uint64 square, uint8 sqIndex, uint64 slidingSources, uint64 pinned, uint8 kingIndex, int *nMoves, CMove **genMoves)
{
    while (slidingSources)
    {
        uint64 piece = getOne(slidingSources);
        int index = bitScan(piece);
        if (piece & pinned)
        {
            // this lookup can probably be avoided ? (replacing this with a check to see if the piece lies in the line between the 'square' and 'king')
            uint64 line = sqsInLine(index, kingIndex);
            if (square & line)
            {
                addCompactMove(nMoves, genMoves, index, sqIndex, CM_FLAG_CAPTURE);
            }
        }
        else
        {
            addCompactMove(nMoves, genMoves, index, sqIndex, CM_FLAG_CAPTURE);
        }
        slidingSources ^= piece;
    }
}

bool BitBoardUtils::generateFirstSlidingCapturesForSquare(uint64 square, uint8 sqIndex, uint64 slidingSources, uint64 pinned, uint8 kingIndex, CMove *genMove)
{
    while (slidingSources)
    {
        uint64 piece = getOne(slidingSources);
        int index = bitScan(piece);
        if (piece & pinned)
        {
            // this lookup can probably be avoided ? (replacing this with a check to see if the piece lies in the line between the 'square' and 'king')
            uint64 line = sqsInLine(index, kingIndex);
            if (square & line)
            {
                CMove move(index, sqIndex, CM_FLAG_CAPTURE);
                *genMove = move;
                return true;
            }
        }
        else
        {
            CMove move(index, sqIndex, CM_FLAG_CAPTURE);
            *genMove = move;
            return true;
        }
        slidingSources ^= piece;
    }
    return false;
}


template<uint8 chance>
bool BitBoardUtils::generateFirstLVACaptureForSquare(uint64 square, uint64 pinned, uint64 threatened, uint64 myKing, uint8 kingIndex, uint64 allPieces,
                                                     uint64 myPawns, uint64 myNonPinnedKnights, uint64 myBishops, uint64 myRooks, uint64 myQueens, CMove *genMove)
{
    uint8 sqIndex = bitScan(square);

    // pawn captures

    uint64 pawn = ((chance == WHITE) ? southWestOne(square) : northWestOne(square));
    if (pawn & myPawns)
    {
        uint8 pawnIndex = bitScan(pawn);
        if (pinned & pawn)
        {
            uint64 line = sqsInLine(pawnIndex, kingIndex);
            if (square & line)
            {
                CMove move(pawnIndex, sqIndex, CM_FLAG_CAPTURE);
                *genMove = move;
                return true;
            }
        }
        else
        {
            CMove move(pawnIndex, sqIndex, CM_FLAG_CAPTURE);
            *genMove = move;
            return true;
        }
    }

    pawn = ((chance == WHITE) ? southEastOne(square) : northEastOne(square));
    if (pawn & myPawns)
    {
        uint8 pawnIndex = bitScan(pawn);
        if (pinned & pawn)
        {
            uint64 line = sqsInLine(pawnIndex, kingIndex);
            if (square & line)
            {
                CMove move(pawnIndex, sqIndex, CM_FLAG_CAPTURE);
                *genMove = move;
                return true;
            }
        }
        else
        {
            CMove move(pawnIndex, sqIndex, CM_FLAG_CAPTURE);
            *genMove = move;
            return true;
        }
    }


    // knight captures (only non-pinned knights can move)
    uint64 knightSources = sqKnightAttacks(sqIndex) & myNonPinnedKnights;
    if (knightSources)
    {
        uint64 knight = getOne(knightSources);

        CMove move(bitScan(knight), sqIndex, CM_FLAG_CAPTURE);
        *genMove = move;
        return true;
    }

    // bishop attacks
    uint64 bishopAttackers = bishopAttacks(square, ~allPieces);
    uint64 bishopSources = bishopAttackers & myBishops;
    if (generateFirstSlidingCapturesForSquare(square, sqIndex, bishopSources, pinned, kingIndex, genMove))
        return true;

    // rook attacks
    uint64 rookAttackers = rookAttacks(square, ~allPieces);
    uint64 rookSources = rookAttackers & myRooks;
    if (generateFirstSlidingCapturesForSquare(square, sqIndex, rookSources, pinned, kingIndex, genMove))
        return true;

    // queen attacks
    uint64 queenSources = (bishopAttackers | rookAttackers) & myQueens;
    if (generateFirstSlidingCapturesForSquare(square, sqIndex, queenSources, pinned, kingIndex, genMove))
        return true;

    // king attacks
    if (square & (~threatened)) // king can't capture pieces that have support
    {
        uint64 kingSources = sqKingAttacks(sqIndex) & myKing;
        if (kingSources)
        {
            CMove move(kingIndex, sqIndex, CM_FLAG_CAPTURE);
            *genMove = move;
            return true;
        }
    }

    return false;
}


template<uint8 chance>
void BitBoardUtils::generateLVACapturesForSquare(uint64 square, uint64 pinned, uint64 threatened, uint64 myKing, uint8 kingIndex, uint64 allPieces,
                                                 uint64 myPawns, uint64 myNonPinnedKnights, uint64 myBishops, uint64 myRooks, uint64 myQueens, int *nMoves, CMove **genMoves)
{
    uint8 sqIndex = bitScan(square);

    // pawn captures

    uint64 pawn = ((chance == WHITE) ? southWestOne(square) : northWestOne(square));
    if (pawn & myPawns)
    {
        uint8 pawnIndex = bitScan(pawn);
        if (pinned & pawn)
        {
            uint64 line = sqsInLine(pawnIndex, kingIndex);
            if (square & line)
            {
                addCompactPawnMoves(nMoves, genMoves, pawnIndex, square, CM_FLAG_CAPTURE);
            }
        }
        else
        {
            addCompactPawnMoves(nMoves, genMoves, pawnIndex, square, CM_FLAG_CAPTURE);
        }
    }

    pawn = ((chance == WHITE) ? southEastOne(square) : northEastOne(square));
    if (pawn & myPawns)
    {
        uint8 pawnIndex = bitScan(pawn);
        if (pinned & pawn)
        {
            uint64 line = sqsInLine(pawnIndex, kingIndex);
            if (square & line)
            {
                addCompactPawnMoves(nMoves, genMoves, pawnIndex, square, CM_FLAG_CAPTURE);
            }
        }
        else
        {
            addCompactPawnMoves(nMoves, genMoves, pawnIndex, square, CM_FLAG_CAPTURE);
        }
    }


    // knight captures (only non-pinned knights can move)
    uint64 knightSources = sqKnightAttacks(sqIndex) & myNonPinnedKnights;
    while (knightSources)
    {
        uint64 knight = getOne(knightSources);
        addCompactMove(nMoves, genMoves, bitScan(knight), sqIndex, CM_FLAG_CAPTURE);
        knightSources ^= knight;
    }

    // bishop attacks
    uint64 bishopAttackers = bishopAttacks(square, ~allPieces);
    uint64 bishopSources = bishopAttackers & myBishops;
    generateSlidingCapturesForSquare(square, sqIndex, bishopSources, pinned, kingIndex, nMoves, genMoves);

    // rook attacks
    uint64 rookAttackers = rookAttacks(square, ~allPieces);
    uint64 rookSources   = rookAttackers & myRooks;
    generateSlidingCapturesForSquare(square, sqIndex, rookSources, pinned, kingIndex, nMoves, genMoves);

    // queen attacks
    uint64 queenSources = (bishopAttackers | rookAttackers) & myQueens;
    generateSlidingCapturesForSquare(square, sqIndex, queenSources, pinned, kingIndex, nMoves, genMoves);

    // king attacks
    if (square & (~threatened)) // king can't capture pieces that have support
    {
        uint64 kingSources = sqKingAttacks(sqIndex) & myKing;
        if (kingSources)
        {
            addCompactMove(nMoves, genMoves, kingIndex, sqIndex, CM_FLAG_CAPTURE);
        }
    }

}

// This also generates promotions (non capture promotions are generated first before any other move - even though promotion+capture would have been the best move)
// it's assumed that the current position is not in check (otherwise generateMovesOutOfCheck is called directly)
// generates captures in MVV-LVA order
template<uint8 chance>
int BitBoardUtils::generateCaptures(const ExpandedBitBoard *bb, CMove *captures)
{
    int nMoves = 0;


    // select pawns that are going to be promoted
    uint64 myPromotionPawns = (chance == WHITE) ? bb->myPawns & RANK7 : bb->myPawns & RANK2;

    // pinned pawns can't be promoted (without capturing enemy piece) - so deal with only non-pinned pawns
    myPromotionPawns &= ~bb->pinned;

    while (myPromotionPawns)
    {
        uint64 pawn = getOne(myPromotionPawns);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~bb->allPieces);
        if (dst)
        {
            addCompactPawnMoves(&nMoves, &captures, bitScan(pawn), dst, 0);
        }

        myPromotionPawns ^= pawn;
    }



    uint64 myNonPinnedKnights = bb->myKnights & (~(bb->pinned));

    uint64 allQueens    = bb->bishopQueens & bb->rookQueens;

    uint64 myBishops    = bb->myBishopQueens & (~allQueens);
    uint64 myRooks      = bb->myRookQueens & (~allQueens);
    uint64 myQueens     = allQueens & bb->myPieces;

    uint64 enemyQueens  = bb->enemyBishopQueens & bb->enemyRookQueens;
    uint64 enemyRooks   = bb->enemyRookQueens ^ enemyQueens;
    uint64 enemyBishops = bb->enemyBishopQueens ^ enemyQueens;

    // try capturing enemy queens
    while (enemyQueens)
    {
        uint64 queen = getOne(enemyQueens);
        generateLVACapturesForSquare<chance>(queen, bb->pinned, bb->threatened, bb->myKing, bb->myKingIndex, bb->allPieces, bb->myPawns, myNonPinnedKnights, myBishops, myRooks, myQueens, &nMoves, &captures);
        enemyQueens ^= queen;
    }

    // capture enemy rooks
    while (enemyRooks)
    {
        uint64 rook = getOne(enemyRooks);
        generateLVACapturesForSquare<chance>(rook, bb->pinned, bb->threatened, bb->myKing, bb->myKingIndex, bb->allPieces, bb->myPawns, myNonPinnedKnights, myBishops, myRooks, myQueens, &nMoves, &captures);
        enemyRooks ^= rook;
    }

    // capture enemy bishops
    while (enemyBishops)
    {
        uint64 bishop = getOne(enemyBishops);
        generateLVACapturesForSquare<chance>(bishop, bb->pinned, bb->threatened, bb->myKing, bb->myKingIndex, bb->allPieces, bb->myPawns, myNonPinnedKnights, myBishops, myRooks, myQueens, &nMoves, &captures);
        enemyBishops ^= bishop;
    }

    // capture enemy knights
    // TODO: for knight captures by pawns, we might not need to check for pinned ? (pinned pawn can never capture a knight)
    uint64 enemyKnights = bb->enemyKnights;
    while (enemyKnights)
    {
        uint64 knight = getOne(enemyKnights);
        generateLVACapturesForSquare<chance>(knight, bb->pinned, bb->threatened, bb->myKing, bb->myKingIndex, bb->allPieces, bb->myPawns, myNonPinnedKnights, myBishops, myRooks, myQueens, &nMoves, &captures);
        enemyKnights ^= knight;
    }

    // generate en-passent move
    if (bb->enPassent)
    {
        uint64 enPassentTarget = 0;

        if (chance == BLACK)
        {
            enPassentTarget = BIT(bb->enPassent - 1) << (8 * 2);
        }
        else
        {
            enPassentTarget = BIT(bb->enPassent - 1) << (8 * 5);
        }

        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);
        uint64 epSources = (eastOne(enPassentCapturedPiece) | westOne(enPassentCapturedPiece)) & bb->myPawns;

        while (epSources)
        {
            uint64 pawn = getOne(epSources);
            if (pawn & bb->pinned)
            {
                // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
                uint64 line = sqsInLine(bitScan(pawn), bb->myKingIndex);

                if (enPassentTarget & line)
                {
                    addCompactMove(&nMoves, &captures, bitScan(pawn), bitScan(enPassentTarget), CM_FLAG_EP_CAPTURE);
                }
            }
            else
            {
                uint64 propogator = (~bb->allPieces) | enPassentCapturedPiece | pawn;
                uint64 causesCheck = (eastAttacks(bb->enemyRookQueens, propogator) | westAttacks(bb->enemyRookQueens, propogator)) &
                                     (bb->myKing);
                if (!causesCheck)
                {
                    addCompactMove(&nMoves, &captures, bitScan(pawn), bitScan(enPassentTarget), CM_FLAG_EP_CAPTURE);
                }
            }
            epSources ^= pawn;
        }
    }

    // capture enemy pawns
    // TODO: for pawn captures by pawns also, we might not need to check for pinned ? (pinned pawn can never capture a pawn)
    uint64 enemyPawns = bb->enemyPawns;
    while (enemyPawns)
    {
        uint64 pawn = getOne(enemyPawns);
        generateLVACapturesForSquare<chance>(pawn, bb->pinned, bb->threatened, bb->myKing, bb->myKingIndex, bb->allPieces, bb->myPawns, myNonPinnedKnights, myBishops, myRooks, myQueens, &nMoves, &captures);
        enemyPawns ^= pawn;
    }

    return nMoves;
}

template<uint8 chance>
int BitBoardUtils::generateNonCaptures(const ExpandedBitBoard *bb, CMove *genMoves)
{
    int nMoves = 0;

    // 1. pawn moves

    // checking rank for pawn double pushes
    uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

    // remove pawns that are going to be promoted (those moves are generated by generateCaptures)
    uint64 myNonPromotionPawns = (chance == WHITE) ? bb->myPawns & ~RANK7 : bb->myPawns & ~RANK2;

    // first deal with pinned pawns
    uint64 pinnedPawns = myNonPromotionPawns & bb->pinned;

    while (pinnedPawns)
    {
        uint64 pawn = getOne(pinnedPawns);
        uint8 pawnIndex = bitScan(pawn);    // same as bitscan on pinnedPawns

        // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
        uint64 line = sqsInLine(pawnIndex, bb->myKingIndex);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & line & (~bb->allPieces);
        if (dst)
        {
            addCompactMove(&nMoves, &genMoves, pawnIndex, bitScan(dst), 0);

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                   southOne(dst & checkingRankDoublePush)) & (~bb->allPieces);
            if (dst)
            {
                addCompactMove(&nMoves, &genMoves, pawnIndex, bitScan(dst), CM_FLAG_DOUBLE_PAWN_PUSH);
            }
        }

        pinnedPawns ^= pawn;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
    }

    uint64 myPawns = myNonPromotionPawns & ~bb->pinned;

    while (myPawns)
    {
        uint64 pawn = getOne(myPawns);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~bb->allPieces);
        if (dst)
        {
            addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, 0);

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                southOne(dst & checkingRankDoublePush)) & (~bb->allPieces);

            if (dst) addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_DOUBLE_PAWN_PUSH);
        }

        myPawns ^= pawn;
    }

    // generate castling moves
    if (chance == WHITE)
    {
        if ((bb->whiteCastle & CASTLE_FLAG_KING_SIDE) &&        // castle flag is set
            !(F1G1 & bb->allPieces) &&                          // squares between king and rook are empty
            !(F1G1 & bb->threatened))                           // and not in threat from enemy pieces
        {
            // white king side castle
            addCompactMove(&nMoves, &genMoves, E1, G1, CM_FLAG_KING_CASTLE);
        }
        if ((bb->whiteCastle & CASTLE_FLAG_QUEEN_SIDE) &&       // castle flag is set
            !(B1D1 & bb->allPieces) &&                          // squares between king and rook are empty
            !(C1D1 & bb->threatened))                           // and not in threat from enemy pieces
        {
            // white queen side castle
            addCompactMove(&nMoves, &genMoves, E1, C1, CM_FLAG_QUEEN_CASTLE);
        }
    }
    else
    {
        if ((bb->blackCastle & CASTLE_FLAG_KING_SIDE) &&        // castle flag is set
            !(F8G8 & bb->allPieces) &&                          // squares between king and rook are empty
            !(F8G8 & bb->threatened))                           // and not in threat from enemy pieces
        {
            // black king side castle
            addCompactMove(&nMoves, &genMoves, E8, G8, CM_FLAG_KING_CASTLE);
        }
        if ((bb->blackCastle & CASTLE_FLAG_QUEEN_SIDE) &&       // castle flag is set
            !(B8D8 & bb->allPieces) &&                          // squares between king and rook are empty
            !(C8D8 & bb->threatened))                           // and not in threat from enemy pieces
        {
            // black queen side castle
            addCompactMove(&nMoves, &genMoves, E8, C8, CM_FLAG_QUEEN_CASTLE);
        }
    }

    // generate king moves
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(bb->myKingIndex);
#else
    uint64 kingMoves = kingAttacks(bb->myKing);
#endif

    kingMoves &= ~(bb->threatened | bb->allPieces);  // generate only non-captures
    while (kingMoves)
    {
        uint64 dst = getOne(kingMoves);
        addCompactMove(&nMoves, &genMoves, bb->myKingIndex, bitScan(dst), 0);
        kingMoves ^= dst;
    }

    // generate knight moves (only non-pinned knights can move)
    uint64 myKnights = bb->myKnights & ~bb->pinned;
    while (myKnights)
    {
        uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
        uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~bb->allPieces;
#else
        uint64 knightMoves = knightAttacks(knight) & ~allPieces;
#endif
        while (knightMoves)
        {
            uint64 dst = getOne(knightMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(knight), bitScan(dst), 0);
            knightMoves ^= dst;
        }
        myKnights ^= knight;
    }

    // generate bishop (and queen) moves

    // first deal with pinned bishops
    uint64 bishops = bb->myBishopQueens & bb->pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~bb->allPieces) & ~bb->allPieces;
        bishopMoves &= sqsInLine(bitScan(bishop), bb->myKingIndex);    // pined sliding pieces can move only along the line

        while (bishopMoves)
        {
            uint64 dst = getOne(bishopMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(bishop), bitScan(dst), 0);
            bishopMoves ^= dst;
        }
        bishops ^= bishop;
    }

    // remaining bishops/queens
    bishops = bb->myBishopQueens & ~bb->pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~bb->allPieces) & ~bb->allPieces;

        while (bishopMoves)
        {
            uint64 dst = getOne(bishopMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(bishop), bitScan(dst), 0);
            bishopMoves ^= dst;
        }
        bishops ^= bishop;

    }


    // rook/queen moves
    // first deal with pinned rooks
    uint64 rooks = bb->myRookQueens & bb->pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~bb->allPieces) & ~bb->allPieces;
        rookMoves &= sqsInLine(bitScan(rook), bb->myKingIndex);    // pined sliding pieces can move only along the line

        while (rookMoves)
        {
            uint64 dst = getOne(rookMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(rook), bitScan(dst), 0);
            rookMoves ^= dst;
        }
        rooks ^= rook;
    }

    // remaining rooks/queens
    rooks = bb->myRookQueens & ~bb->pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~bb->allPieces) & ~bb->allPieces;

        while (rookMoves)
        {
            uint64 dst = getOne(rookMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(rook), bitScan(dst), 0);
            rookMoves ^= dst;
        }
        rooks ^= rook;

    }

    return nMoves;
}


// generate moves giving check to the opponent side
// this function is used only by q-search for check extension (need to treat checks as captures)
// only generates moves that are not captures (as captures are already taken care of in q-search using generateCaptures)

template<uint8 chance>
int BitBoardUtils::generateMovesCausingCheck(const ExpandedBitBoard *bb, CMove *genMoves)
{
    int nMoves = 0;
    // 1. discovered checks
    uint8 enemyKingIndex = bitScan(bb->enemyKing);
    uint8 myKingIndex    = bitScan(bb->myKing);
    
    // my pieces that are preventing check to the enemy king
    // moving these pieces would ensure a check (except for pawns and kings which might still block the check if it moves in the same direction)
    uint64 myBlockingPieces = findPinnedPieces(bb->enemyKing, bb->myBishopQueens, bb->myRookQueens, bb->allPieces, enemyKingIndex) & bb->myPieces;

    if (myBlockingPieces)
    {
        // checking rank for pawn double pushes
        uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

        // first deal with pinned pawns (pawns that are blocking checks to both my and enemy kings!)
        // such pawns when moved (in the direction of pin) will definately cause check to opponent king
        uint64 myBlockingPawns = bb->myPawns & myBlockingPieces;

        // promotions are also taken care by generateCaptures so remove them
        if (chance == WHITE)
            myBlockingPawns &= ~RANK7;
        else
            myBlockingPawns &= ~RANK2;


        uint64 pinnedBlockingPawns = myBlockingPawns & bb->pinned;

        while (pinnedBlockingPawns)
        {
            uint64 pawn = getOne(pinnedBlockingPawns);
            uint8 pawnIndex = bitScan(pawn);

            // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
            uint64 line = sqsInLine(pawnIndex, myKingIndex);

            // pawn push
            uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & line & (~bb->allPieces);
            if (dst)
            {
                addCompactMove(&nMoves, &genMoves, pawnIndex, bitScan(dst), 0);

                // double push (only possible if single push was possible)
                dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                    southOne(dst & checkingRankDoublePush)) & (~bb->allPieces);
                if (dst)
                {
                    addCompactMove(&nMoves, &genMoves, pawnIndex, bitScan(dst), CM_FLAG_DOUBLE_PAWN_PUSH);
                }
            }
            pinnedBlockingPawns ^= pawn;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
        }

        uint64 otherBlockingPawns = myBlockingPawns & ~bb->pinned;

        while (otherBlockingPawns)
        {
            uint64 pawn = getOne(otherBlockingPawns);

            // pawn push
            uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~bb->allPieces);

            // moving on the same line won't cause check        
            uint64 line = sqsInLine(bitScan(pawn), enemyKingIndex);
            dst &= ~line;

            if (dst)
            {
                addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, 0);

                // double push (only possible if single push was possible)
                dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                    southOne(dst & checkingRankDoublePush)) & (~bb->allPieces);

                if (dst) addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_DOUBLE_PAWN_PUSH);
            }
            otherBlockingPawns ^= pawn;
        }

        // king
        if (bb->myKing & myBlockingPieces)
        {
            uint64 kingMoves = sqKingAttacks(myKingIndex);

            // moving on the same line won't cause check
            uint64 line = sqsInLine(myKingIndex, enemyKingIndex);
            kingMoves &= ~line;
            kingMoves &= ~(bb->threatened | bb->allPieces);  // king can't move to a square under threat, also don't capture enemy piece (as we are interested in checks that are not captures)

            while (kingMoves)
            {
                uint64 dst = getOne(kingMoves);
                addCompactMove(&nMoves, &genMoves, myKingIndex, bitScan(dst), 0);
                kingMoves ^= dst;
            }
        }

        uint64 myBlockingKnights = bb->myKnights & myBlockingPieces & ~bb->pinned;
        while (myBlockingKnights)
        {
            uint64 knight = getOne(myBlockingKnights);
            uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~bb->allPieces;
            while (knightMoves)
            {
                uint64 dst = getOne(knightMoves);
                addCompactMove(&nMoves, &genMoves, bitScan(knight), bitScan(dst), 0);
                knightMoves ^= dst;
            }
            myBlockingKnights ^= knight;
        }

        // bishops (both pinned and non-pinned)
        uint64 myBlockingBishops = bb->myBishopQueens & myBlockingPieces;
        while (myBlockingBishops)
        {
            uint64 bishop = getOne(myBlockingBishops);
            uint8 index = bitScan(bishop);
            uint64 bishopMoves = bishopAttacks(bishop, ~bb->allPieces) & ~bb->allPieces;

            if (bishop & bb->pinned)
            {
                // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
                uint64 line = sqsInLine(index, myKingIndex);
                bishopMoves &= line;
            }

            while (bishopMoves)
            {
                uint64 dst = getOne(bishopMoves);
                addCompactMove(&nMoves, &genMoves, index, bitScan(dst), 0);
                bishopMoves ^= dst;
            }

            myBlockingBishops ^= bishop;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
        }

        // rooks
        uint64 myBlockingRooks = bb->myRookQueens & myBlockingPieces;
        while (myBlockingRooks)
        {
            uint64 rook = getOne(myBlockingRooks);
            uint8 index = bitScan(rook);
            uint64 rookMoves = rookAttacks(rook, ~bb->allPieces) & ~bb->allPieces;

            if (rook & bb->pinned)
            {
                // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
                uint64 line = sqsInLine(index, myKingIndex);
                rookMoves &= line;
            }

            while (rookMoves)
            {
                uint64 dst = getOne(rookMoves);
                addCompactMove(&nMoves, &genMoves, index, bitScan(dst), 0);
                rookMoves ^= dst;
            }

            myBlockingRooks ^= rook;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
        }
    }

    // 2. direct checks
    // check if all attack squares to the king has any piece that can attack from there... + extra complication of pinned, etc

    // checks by pawns (non-promoting pawns)

    // pawn destination on west side for causing check (should be empty)
    uint64 dstw = ((chance == WHITE) ? southWestOne(bb->enemyKing) : northWestOne(bb->enemyKing)) & (~bb->allPieces);

    if (dstw)
    {

        // pawn push
        uint64 scrwest = ((chance == WHITE) ? southOne(dstw) : northOne(dstw));
        uint64 srcw = scrwest & bb->myPawns;
        if (srcw & bb->pinned)
        {
            uint64 line = sqsInLine(bitScan(srcw), myKingIndex);
            if ((dstw & line) == 0)
                srcw = 0;
        }
        if (srcw)
        {
            addCompactMove(&nMoves, &genMoves, bitScan(srcw), bitScan(dstw), 0);
        }

        // double push
        if (scrwest & ~bb->allPieces)
        {
            srcw = 0;
            if (chance == WHITE)
            {
                if (dstw & RANK4)
                    srcw = southOne(scrwest) & bb->myPawns;
            }
            else
            {
                if (dstw & RANK5)
                    srcw = northOne(scrwest) & bb->myPawns;
            }


            if (srcw & bb->pinned)
            {
                uint64 line = sqsInLine(bitScan(srcw), myKingIndex);
                if ((dstw & line) == 0)
                    srcw = 0;
            }
            if (srcw)
            {
                addCompactMove(&nMoves, &genMoves, bitScan(srcw), bitScan(dstw), 0);
            }
        }
    }

    // pawn destination on east side for causing check (should be empty)
    uint64 dste = ((chance == WHITE) ? southEastOne(bb->enemyKing) : northEastOne(bb->enemyKing)) & (~bb->allPieces);

    if (dste)
    {
        uint64 screast = ((chance == WHITE) ? southOne(dste) : northOne(dste));
        uint64 srce = screast & bb->myPawns;
        if (srce & bb->pinned)
        {
            uint64 line = sqsInLine(bitScan(srce), myKingIndex);
            if ((dste & line) == 0)
                srce = 0;
        }
        if (srce)
        {
            addCompactMove(&nMoves, &genMoves, bitScan(srce), bitScan(dste), 0);
        }

        // double push
        if (screast & ~bb->allPieces)
        {
            srce = 0;
            if (chance == WHITE)
            {
                if (dste & RANK4)
                    srce = southOne(screast) & bb->myPawns;
            }
            else
            {
                if (dste & RANK5)
                    srce = northOne(screast) & bb->myPawns;
            }


            if (srce & bb->pinned)
            {
                uint64 line = sqsInLine(bitScan(srce), myKingIndex);
                if ((dste & line) == 0)
                    srce = 0;
            }
            if (srce)
            {
                addCompactMove(&nMoves, &genMoves, bitScan(srce), bitScan(dste), 0);
            }
        }
    }

    // checks by knights
    uint64 knightDests = sqKnightAttacks(enemyKingIndex) & ~bb->allPieces;

    while (knightDests)
    {
        uint64 dst = getOne(knightDests);
        uint8 dstIndex = bitScan(dst);
        uint64 sources = sqKnightAttacks(dstIndex) & bb->myKnights & (~bb->pinned) & (~myBlockingPieces);
        while (sources)
        {
            uint64 src = getOne(sources);
            addCompactMove(&nMoves, &genMoves, bitScan(src), dstIndex, 0);
            sources ^= src;
        }

        knightDests ^= dst;
    }

    // checks by bishop (or queen)
    uint64 bishopDests = bishopAttacks(bb->enemyKing, ~bb->allPieces) & ~bb->allPieces;
    while (bishopDests)
    {
        uint64 dst = getOne(bishopDests);
        uint8 dstIndex = bitScan(dst);

        uint64 sources = bishopAttacks(dst, ~bb->allPieces) & bb->myBishopQueens & (~myBlockingPieces);

        // queens can reach these destinations even using rook moves!
        sources |= rookAttacks(dst, ~bb->allPieces) & bb->myRookQueens & bb->myBishopQueens & (~myBlockingPieces);

        while (sources)
        {
            uint64 src = getOne(sources);
            uint8 srcIndex = bitScan(src);
            if ( (!(src & bb->pinned)) ||
                 (dst & sqsInLine(srcIndex, bb->myKingIndex)))
            {
                addCompactMove(&nMoves, &genMoves, srcIndex, dstIndex, 0);
            }
            sources ^= src;
        }

        bishopDests ^= dst;
    }

    // checks by rook (or queen)
    uint64 rookDests = rookAttacks(bb->enemyKing, ~bb->allPieces) & ~bb->allPieces;
    uint64 rookDestsCopy = rookDests;
    while (rookDests)
    {
        uint64 dst = getOne(rookDests);
        uint8 dstIndex = bitScan(dst);

        uint64 sources = rookAttacks(dst, ~bb->allPieces) & bb->myRookQueens  & (~myBlockingPieces);

        // queens can reach these destinations even using bishop moves!
        sources |= bishopAttacks(dst, ~bb->allPieces) & bb->myRookQueens & bb->myBishopQueens  & (~myBlockingPieces);

        while (sources)
        {
            uint64 src = getOne(sources);
            uint8 srcIndex = bitScan(src);
            if ((!(src & bb->pinned)) ||
                (dst & sqsInLine(srcIndex, bb->myKingIndex)))
            {
                addCompactMove(&nMoves, &genMoves, srcIndex, dstIndex, 0);
            }
            sources ^= src;
        }

        rookDests ^= dst;
    }

    // check due to castling
    if (chance == WHITE)
    {
        if ((bb->whiteCastle & CASTLE_FLAG_KING_SIDE) &&    // castle flag is set
            (rookDestsCopy & BIT(F1)) &&                    // rook at square F1 causes a check to opponent king
            !(F1G1 & bb->allPieces) &&                      // squares between king and rook are empty
            !(F1G1 & bb->threatened))                       // and not in threat from enemy pieces
        {
            // white king side castle
            addCompactMove(&nMoves, &genMoves, E1, G1, CM_FLAG_KING_CASTLE);
        }
        if ((bb->whiteCastle & CASTLE_FLAG_QUEEN_SIDE) &&   // castle flag is set
            (rookDestsCopy & BIT(D1)) &&                    // rook at square D1 causes a check to opponent king
            !(B1D1 & bb->allPieces) &&                      // squares between king and rook are empty
            !(C1D1 & bb->threatened))                       // and not in threat from enemy pieces
        {
            // white queen side castle
            addCompactMove(&nMoves, &genMoves, E1, C1, CM_FLAG_QUEEN_CASTLE);
        }
    }
    else
    {
        if ((bb->blackCastle & CASTLE_FLAG_KING_SIDE) &&    // castle flag is set
            (rookDestsCopy & BIT(F8)) &&                    // rook at square F8 causes a check to opponent king
            !(F8G8 & bb->allPieces) &&                      // squares between king and rook are empty
            !(F8G8 & bb->threatened))                       // and not in threat from enemy pieces
        {
            // black king side castle
            addCompactMove(&nMoves, &genMoves, E8, G8, CM_FLAG_KING_CASTLE);
        }
        if ((bb->blackCastle & CASTLE_FLAG_QUEEN_SIDE) &&   // castle flag is set
            (rookDestsCopy & BIT(D8)) &&                     // rook at square D8 causes a check to opponent king
            !(B8D8 & bb->allPieces) &&                      // squares between king and rook are empty
            !(C8D8 & bb->threatened))                       // and not in threat from enemy pieces
        {
            // black queen side castle
            addCompactMove(&nMoves, &genMoves, E8, C8, CM_FLAG_QUEEN_CASTLE);
        }
    }


    return nMoves;
}

template<uint8 chance>
int BitBoardUtils::generateMovesOutOfCheck(const ExpandedBitBoard *bb, CMove *genMoves)
{
    int nMoves = 0;

    uint64 king = bb->myKing;
    // figure out the no. of attackers 
    uint64 attackers = 0;

    // pawn attacks
    attackers |= ((chance == WHITE) ? (northEastOne(king) | northWestOne(king)) :
                  (southEastOne(king) | southWestOne(king))) & bb->enemyPawns;

    // knight attackers
    attackers |= knightAttacks(king) & bb->enemyKnights;

    // bishop attackers
    attackers |= bishopAttacks(king, ~bb->allPieces) & bb->enemyBishopQueens;

    // rook attackers
    attackers |= rookAttacks(king, ~bb->allPieces) & bb->enemyRookQueens;

    uint64 safePieces = bb->myPieces;
    // A. try moves to kill/block attacking pieces
    if (isSingular(attackers))
    {
        // Find the safe squares - i.e, if a dst square of a move is any of the safe squares, 
        // it will take king out of check

        // for pawn and knight attack, the only option is to kill the attacking piece
        // for bishops rooks and queens, it's the line between the attacker and the king, including the attacker
        uint64 safeSquares = attackers | sqsInBetween(bb->myKingIndex, bitScan(attackers));

        // pieces that are pinned don't have any hope of saving the king
        // TODO: Think more about it
        safePieces &= ~bb->pinned;

        // 1. pawn moves
        // checking rank for pawn double pushes
        uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

        uint64 enPassentTarget = 0;
        if (bb->enPassent)
        {
            if (chance == BLACK)
            {
                enPassentTarget = BIT(bb->enPassent - 1) << (8 * 2);
            }
            else
            {
                enPassentTarget = BIT(bb->enPassent - 1) << (8 * 5);
            }
        }

        // en-passent can only save the king if the piece captured is the attacker
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);
        if (enPassentCapturedPiece != attackers)
            enPassentTarget = 0;

        uint64 myPawns = bb->myPawns & safePieces;
        while (myPawns)
        {
            uint64 pawn = getOne(myPawns);
            uint64 dst = 0;

            // captures (only one of the two captures will save the king.. if at all it does)
            uint64 westCapture = (chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn);
            uint64 eastCapture = (chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn);
            dst = (westCapture | eastCapture) & bb->enemyPieces & safeSquares;
            if (dst)
            {
                addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_CAPTURE);
            }

            // en-passent 
            dst = (westCapture | eastCapture) & enPassentTarget;
            if (dst)
            {
                addCompactMove(&nMoves, &genMoves, bitScan(pawn), bitScan(dst), CM_FLAG_EP_CAPTURE);
            }

            // pawn push
            dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~bb->allPieces);
            if (dst)
            {
                if (dst & safeSquares)
                {
                    addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, 0);
                }
                else
                {
                    // double push (only possible if single push was possible and single push didn't save the king)
                    dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                        southOne(dst & checkingRankDoublePush)) & (safeSquares)&(~bb->allPieces);

                    if (dst)
                    {
                        addCompactMove(&nMoves, &genMoves, bitScan(pawn), bitScan(dst), CM_FLAG_DOUBLE_PAWN_PUSH);
                    }
                }
            }

            myPawns ^= pawn;
        }

        // 2. knight moves
        uint64 myKnights = bb->myKnights & safePieces;
        while (myKnights)
        {
            uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
            uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & safeSquares;
#else
            uint64 knightMoves = knightAttacks(knight) & safeSquares;
#endif
            while (knightMoves)
            {
                uint64 dst = getOne(knightMoves);
                // TODO: set capture flag correctly
                addCompactMove(&nMoves, &genMoves, bitScan(knight), bitScan(dst), 0);
                knightMoves ^= dst;
            }
            myKnights ^= knight;
        }

        // 3. bishop moves
        uint64 bishops = bb->myBishopQueens & safePieces;
        while (bishops)
        {
            uint64 bishop = getOne(bishops);
            uint64 bishopMoves = bishopAttacks(bishop, ~bb->allPieces) & safeSquares;

            while (bishopMoves)
            {
                uint64 dst = getOne(bishopMoves);
                // TODO: set capture flag correctly
                addCompactMove(&nMoves, &genMoves, bitScan(bishop), bitScan(dst), 0);
                bishopMoves ^= dst;
            }
            bishops ^= bishop;
        }

        // 4. rook moves
        uint64 rooks = bb->myRookQueens & safePieces;
        while (rooks)
        {
            uint64 rook = getOne(rooks);
            uint64 rookMoves = rookAttacks(rook, ~bb->allPieces) & safeSquares;

            while (rookMoves)
            {
                uint64 dst = getOne(rookMoves);
                // TODO: set capture flag correctly
                addCompactMove(&nMoves, &genMoves, bitScan(rook), bitScan(dst), 0);
                rookMoves ^= dst;
            }
            rooks ^= rook;
        }

    }   // end of if single attacker
    else
    {
        // multiple threats => only king moves possible
    }


    // B. Try king moves to get the king out of check
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(bb->myKingIndex);
#else
    uint64 kingMoves = kingAttacks(bb->myKing);
#endif

    kingMoves &= ~(bb->threatened | bb->myPieces);  // king can't move to a square under threat or a square containing piece of same side

    // first try captures
    uint64 kingCaptures = kingMoves & bb->enemyPieces;
    while (kingCaptures)
    {
        uint64 dst = getOne(kingCaptures);
        addCompactMove(&nMoves, &genMoves, bb->myKingIndex, bitScan(dst), CM_FLAG_CAPTURE);
        kingCaptures ^= dst;
    }


    // then regular moves
    kingMoves &= (~bb->enemyPieces);
    while (kingMoves)
    {
        uint64 dst = getOne(kingMoves);
        addCompactMove(&nMoves, &genMoves, bb->myKingIndex, bitScan(dst), 0);
        kingMoves ^= dst;
    }

    return nMoves;
}



template<uint8 chance>
int BitBoardUtils::generateMovesOutOfCheck(HexaBitBoardPosition *pos, CMove *genMoves, uint64 allPawns, uint64 allPieces, uint64 myPieces, uint64 enemyPieces, uint64 pinned, uint64 threatened, uint8 kingIndex)
{
    int nMoves = 0;
    uint64 king = pos->kings & myPieces;

    // figure out the no. of attackers 
    uint64 attackers = 0;

    // pawn attacks
    uint64 enemyPawns = allPawns & enemyPieces;
    attackers |= ((chance == WHITE) ? (northEastOne(king) | northWestOne(king)) :
        (southEastOne(king) | southWestOne(king))) & enemyPawns;

    // knight attackers
    uint64 enemyKnights = pos->knights & enemyPieces;
    attackers |= knightAttacks(king) & enemyKnights;

    // bishop attackers
    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    attackers |= bishopAttacks(king, ~allPieces) & enemyBishops;

    // rook attackers
    uint64 enemyRooks = pos->rookQueens & enemyPieces;
    attackers |= rookAttacks(king, ~allPieces) & enemyRooks;


    // A. try moves to kill/block attacking pieces
    if (isSingular(attackers))
    {
        // Find the safe squares - i.e, if a dst square of a move is any of the safe squares, 
        // it will take king out of check

        // for pawn and knight attack, the only option is to kill the attacking piece
        // for bishops rooks and queens, it's the line between the attacker and the king, including the attacker
        uint64 safeSquares = attackers | sqsInBetween(kingIndex, bitScan(attackers));

        // pieces that are pinned don't have any hope of saving the king
        // TODO: Think more about it
        myPieces &= ~pinned;

        // 1. pawn moves
        uint64 myPawns = allPawns & myPieces;

        // checking rank for pawn double pushes
        uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

        uint64 enPassentTarget = 0;
        if (pos->enPassent)
        {
            if (chance == BLACK)
            {
                enPassentTarget = BIT(pos->enPassent - 1) << (8 * 2);
            }
            else
            {
                enPassentTarget = BIT(pos->enPassent - 1) << (8 * 5);
            }
        }

        // en-passent can only save the king if the piece captured is the attacker
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);
        if (enPassentCapturedPiece != attackers)
            enPassentTarget = 0;

        while (myPawns)
        {
            uint64 pawn = getOne(myPawns);
            uint64 dst = 0;

            // captures (only one of the two captures will save the king.. if at all it does)
            uint64 westCapture = (chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn);
            uint64 eastCapture = (chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn);
            dst = (westCapture | eastCapture) & enemyPieces & safeSquares;
            if (dst)
            {
                addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_CAPTURE);
            }

            // en-passent 
            dst = (westCapture | eastCapture) & enPassentTarget;
            if (dst)
            {
                addCompactMove(&nMoves, &genMoves, bitScan(pawn), bitScan(dst), CM_FLAG_EP_CAPTURE);
            }

            // pawn push
            dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~allPieces);
            if (dst)
            {
                if (dst & safeSquares)
                {
                    addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, 0);
                }
                else
                {
                    // double push (only possible if single push was possible and single push didn't save the king)
                    dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                        southOne(dst & checkingRankDoublePush)) & (safeSquares)&(~allPieces);

                    if (dst)
                    {
                        addCompactMove(&nMoves, &genMoves, bitScan(pawn), bitScan(dst), CM_FLAG_DOUBLE_PAWN_PUSH);
                    }
                }
            }

            myPawns ^= pawn;
        }

        // 2. knight moves
        uint64 myKnights = (pos->knights & myPieces);
        while (myKnights)
        {
            uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
            uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & safeSquares;
#else
            uint64 knightMoves = knightAttacks(knight) & safeSquares;
#endif
            while (knightMoves)
            {
                uint64 dst = getOne(knightMoves);
                // TODO: set capture flag correctly
                addCompactMove(&nMoves, &genMoves, bitScan(knight), bitScan(dst), 0);
                knightMoves ^= dst;
            }
            myKnights ^= knight;
        }

        // 3. bishop moves
        uint64 bishops = pos->bishopQueens & myPieces;
        while (bishops)
        {
            uint64 bishop = getOne(bishops);
            uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & safeSquares;

            while (bishopMoves)
            {
                uint64 dst = getOne(bishopMoves);
                // TODO: set capture flag correctly
                addCompactMove(&nMoves, &genMoves, bitScan(bishop), bitScan(dst), 0);
                bishopMoves ^= dst;
            }
            bishops ^= bishop;
        }

        // 4. rook moves
        uint64 rooks = pos->rookQueens & myPieces;
        while (rooks)
        {
            uint64 rook = getOne(rooks);
            uint64 rookMoves = rookAttacks(rook, ~allPieces) & safeSquares;

            while (rookMoves)
            {
                uint64 dst = getOne(rookMoves);
                // TODO: set capture flag correctly
                addCompactMove(&nMoves, &genMoves, bitScan(rook), bitScan(dst), 0);
                rookMoves ^= dst;
            }
            rooks ^= rook;
        }

    }   // end of if single attacker
    else
    {
        // multiple threats => only king moves possible
    }


    // B. Try king moves to get the king out of check
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(kingIndex);
#else
    uint64 kingMoves = kingAttacks(king);
#endif

    kingMoves &= ~(threatened | myPieces);  // king can't move to a square under threat or a square containing piece of same side

    // first try captures
    uint64 kingCaptures = kingMoves & enemyPieces;
    while (kingCaptures)
    {
        uint64 dst = getOne(kingCaptures);
        addCompactMove(&nMoves, &genMoves, kingIndex, bitScan(dst), CM_FLAG_CAPTURE);
        kingCaptures ^= dst;
    }


    // then regular moves
    kingMoves &= (~enemyPieces);
    while (kingMoves)
    {
        uint64 dst = getOne(kingMoves);
        addCompactMove(&nMoves, &genMoves, kingIndex, bitScan(dst), 0);
        kingMoves ^= dst;
    }

    return nMoves;
}


// generates moves for the given board position
// returns the no of moves generated
// genMoves contains the generated moves
template <uint8 chance>
int BitBoardUtils::generateMoves(HexaBitBoardPosition *pos, CMove *genMoves)
{
    int nMoves = 0;

    uint64 allPawns = pos->pawns & RANKS2TO7;    // get rid of game state variables

    uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;
    uint64 blackPieces = allPieces & (~pos->whitePieces);

    uint64 myPieces = (chance == WHITE) ? pos->whitePieces : blackPieces;
    uint64 enemyPieces = (chance == WHITE) ? blackPieces : pos->whitePieces;

    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    uint64 enemyRooks = pos->rookQueens & enemyPieces;

    uint64 myKing = pos->kings & myPieces;
    uint8  kingIndex = bitScan(myKing);

    uint64 pinned = findPinnedPieces(pos->kings & myPieces, enemyBishops, enemyRooks, allPieces, kingIndex);

    uint64 threatened = findAttackedSquares(~allPieces, enemyBishops, enemyRooks, allPawns & enemyPieces,
                                            pos->knights & enemyPieces, pos->kings & enemyPieces, myKing, !chance);


    // king is in check: call special generate function to generate only the moves that take king out of check
    if (threatened & (pos->kings & myPieces))
    {
        return generateMovesOutOfCheck<chance>(pos, genMoves, allPawns, allPieces, myPieces, enemyPieces, pinned, threatened, kingIndex);
    }

    uint64 myPawns = allPawns & myPieces;

    // 0. generate en-passent moves first
    uint64 enPassentTarget = 0;
    if (pos->enPassent)
    {
        if (chance == BLACK)
        {
            enPassentTarget = BIT(pos->enPassent - 1) << (8 * 2);
        }
        else
        {
            enPassentTarget = BIT(pos->enPassent - 1) << (8 * 5);
        }
    }

    if (enPassentTarget)
    {
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(enPassentTarget) : northOne(enPassentTarget);

        uint64 epSources = (eastOne(enPassentCapturedPiece) | westOne(enPassentCapturedPiece)) & myPawns;
        /*
        uint64 epSources = ((chance == WHITE) ? southEastOne(enPassentTarget) | southWestOne(enPassentTarget) :
        northEastOne(enPassentTarget) | northWestOne(enPassentTarget)) & myPawns;
        */

        while (epSources)
        {
            uint64 pawn = getOne(epSources);
            if (pawn & pinned)
            {
                // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
                uint64 line = sqsInLine(bitScan(pawn), kingIndex);

                if (enPassentTarget & line)
                {
                    addCompactMove(&nMoves, &genMoves, bitScan(pawn), bitScan(enPassentTarget), CM_FLAG_EP_CAPTURE);
                }
            }
            else
                /*if (!(enPassentCapturedPiece & pinned))*/
                // the captured pawn should not be pinned in diagonal direction but it can be in vertical dir.
                // the diagonal pinning can't happen for enpassent in real chess game, so anyways it's not vaild
            {
                uint64 propogator = (~allPieces) | enPassentCapturedPiece | pawn;
                uint64 causesCheck = (eastAttacks(enemyRooks, propogator) | westAttacks(enemyRooks, propogator)) &
                    (pos->kings & myPieces);
                if (!causesCheck)
                {
                    addCompactMove(&nMoves, &genMoves, bitScan(pawn), bitScan(enPassentTarget), CM_FLAG_EP_CAPTURE);
                }
            }
            epSources ^= pawn;
        }
    }

    // 1. pawn moves

    // checking rank for pawn double pushes
    uint64 checkingRankDoublePush = RANK3 << (chance * 24);           // rank 3 or rank 6

    // first deal with pinned pawns
    uint64 pinnedPawns = myPawns & pinned;

    while (pinnedPawns)
    {
        uint64 pawn = getOne(pinnedPawns);
        uint8 pawnIndex = bitScan(pawn);    // same as bitscan on pinnedPawns

        // the direction of the pin (mask containing all squares in the line joining the king and the current piece)
        uint64 line = sqsInLine(pawnIndex, kingIndex);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & line & (~allPieces);
        if (dst)
        {
            addCompactMove(&nMoves, &genMoves, pawnIndex, bitScan(dst), 0);

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                southOne(dst & checkingRankDoublePush)) & (~allPieces);
            if (dst)
            {
                addCompactMove(&nMoves, &genMoves, pawnIndex, bitScan(dst), CM_FLAG_DOUBLE_PAWN_PUSH);
            }
        }

        // captures
        // (either of them will be valid - if at all)
        dst = ((chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn)) & line;
        dst |= ((chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn)) & line;

        if (dst & enemyPieces)
        {
            addCompactPawnMoves(&nMoves, &genMoves, pawnIndex, dst, CM_FLAG_CAPTURE);
        }

        pinnedPawns ^= pawn;  // same as &= ~pawn (but only when we know that the first set contain the element we want to clear)
    }

    myPawns = myPawns & ~pinned;

    while (myPawns)
    {
        uint64 pawn = getOne(myPawns);

        // pawn push
        uint64 dst = ((chance == WHITE) ? northOne(pawn) : southOne(pawn)) & (~allPieces);
        if (dst)
        {
            addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, 0);

            // double push (only possible if single push was possible)
            dst = ((chance == WHITE) ? northOne(dst & checkingRankDoublePush) :
                southOne(dst & checkingRankDoublePush)) & (~allPieces);

            if (dst) addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_DOUBLE_PAWN_PUSH);
        }

        // captures
        uint64 westCapture = (chance == WHITE) ? northWestOne(pawn) : southWestOne(pawn);
        dst = westCapture & enemyPieces;
        if (dst) addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_CAPTURE);

        uint64 eastCapture = (chance == WHITE) ? northEastOne(pawn) : southEastOne(pawn);
        dst = eastCapture & enemyPieces;
        if (dst) addCompactPawnMoves(&nMoves, &genMoves, bitScan(pawn), dst, CM_FLAG_CAPTURE);

        myPawns ^= pawn;
    }

    // generate castling moves
    if (chance == WHITE)
    {
        if ((pos->whiteCastle & CASTLE_FLAG_KING_SIDE) &&   // castle flag is set
            !(F1G1 & allPieces) &&                          // squares between king and rook are empty
            !(F1G1 & threatened))                           // and not in threat from enemy pieces
        {
            // white king side castle
            addCompactMove(&nMoves, &genMoves, E1, G1, CM_FLAG_KING_CASTLE);
        }
        if ((pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE) &&  // castle flag is set
            !(B1D1 & allPieces) &&                          // squares between king and rook are empty
            !(C1D1 & threatened))                           // and not in threat from enemy pieces
        {
            // white queen side castle
            addCompactMove(&nMoves, &genMoves, E1, C1, CM_FLAG_QUEEN_CASTLE);
        }
    }
    else
    {
        if ((pos->blackCastle & CASTLE_FLAG_KING_SIDE) &&   // castle flag is set
            !(F8G8 & allPieces) &&                          // squares between king and rook are empty
            !(F8G8 & threatened))                           // and not in threat from enemy pieces
        {
            // black king side castle
            addCompactMove(&nMoves, &genMoves, E8, G8, CM_FLAG_KING_CASTLE);
        }
        if ((pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE) &&  // castle flag is set
            !(B8D8 & allPieces) &&                          // squares between king and rook are empty
            !(C8D8 & threatened))                           // and not in threat from enemy pieces
        {
            // black queen side castle
            addCompactMove(&nMoves, &genMoves, E8, C8, CM_FLAG_QUEEN_CASTLE);
        }
    }

    // generate king moves
#if USE_KING_LUT == 1
    uint64 kingMoves = sqKingAttacks(kingIndex);
#else
    uint64 kingMoves = kingAttacks(myKing);
#endif

    kingMoves &= ~(threatened | myPieces);  // king can't move to a square under threat or a square containing piece of same side
    while (kingMoves)
    {
        uint64 dst = getOne(kingMoves);
        addCompactMove(&nMoves, &genMoves, kingIndex, bitScan(dst), 0); // TODO: correctly update capture flag
        kingMoves ^= dst;
    }

    // generate knight moves (only non-pinned knights can move)
    uint64 myKnights = (pos->knights & myPieces) & ~pinned;
    while (myKnights)
    {
        uint64 knight = getOne(myKnights);
#if USE_KNIGHT_LUT == 1
        uint64 knightMoves = sqKnightAttacks(bitScan(knight)) & ~myPieces;
#else
        uint64 knightMoves = knightAttacks(knight) & ~myPieces;
#endif
        while (knightMoves)
        {
            uint64 dst = getOne(knightMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(knight), bitScan(dst), 0); // TODO: correctly update capture flag
            knightMoves ^= dst;
        }
        myKnights ^= knight;
    }



    // generate bishop (and queen) moves
    uint64 myBishops = pos->bishopQueens & myPieces;

    // first deal with pinned bishops
    uint64 bishops = myBishops & pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        // TODO: bishopAttacks() function uses a kogge-stone sliding move generator. Switch to magics!
        uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & ~myPieces;
        bishopMoves &= sqsInLine(bitScan(bishop), kingIndex);    // pined sliding pieces can move only along the line

        while (bishopMoves)
        {
            uint64 dst = getOne(bishopMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(bishop), bitScan(dst), 0); // TODO: correctly update capture flag
            bishopMoves ^= dst;
        }
        bishops ^= bishop;
    }

    // remaining bishops/queens
    bishops = myBishops & ~pinned;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        uint64 bishopMoves = bishopAttacks(bishop, ~allPieces) & ~myPieces;

        while (bishopMoves)
        {
            uint64 dst = getOne(bishopMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(bishop), bitScan(dst), 0); // TODO: correctly update capture flag
            bishopMoves ^= dst;
        }
        bishops ^= bishop;

    }


    // rook/queen moves
    uint64 myRooks = pos->rookQueens & myPieces;

    // first deal with pinned rooks
    uint64 rooks = myRooks & pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~allPieces) & ~myPieces;
        rookMoves &= sqsInLine(bitScan(rook), kingIndex);    // pined sliding pieces can move only along the line

        while (rookMoves)
        {
            uint64 dst = getOne(rookMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(rook), bitScan(dst), 0); // TODO: correctly update capture flag
            rookMoves ^= dst;
        }
        rooks ^= rook;
    }

    // remaining rooks/queens
    rooks = myRooks & ~pinned;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        uint64 rookMoves = rookAttacks(rook, ~allPieces) & ~myPieces;

        while (rookMoves)
        {
            uint64 dst = getOne(rookMoves);
            addCompactMove(&nMoves, &genMoves, bitScan(rook), bitScan(dst), 0); // TODO: correctly update capture flag
            rookMoves ^= dst;
        }
        rooks ^= rook;

    }

    return nMoves;
}



//----------------------------------------- Public functions -------------------------------------------//


int BitBoardUtils::GenerateBoards(HexaBitBoardPosition *pos, HexaBitBoardPosition *newPositions)
{
    int nMoves;
    int chance = pos->chance;
    if (chance == BLACK)
    {
        nMoves = generateBoards<BLACK>(pos, newPositions);
    }
    else
    {
        nMoves = generateBoards<WHITE>(pos, newPositions);
    }
    return nMoves;
}

int BitBoardUtils::CountMoves(HexaBitBoardPosition *pos)
{
    int nMoves;
    int chance = pos->chance;
    if (chance == BLACK)
    {
        nMoves = countMoves<BLACK>(pos);
    }
    else
    {
        nMoves = countMoves<WHITE>(pos);
    }
    return nMoves;
}

int BitBoardUtils::GenerateMoves(HexaBitBoardPosition *pos, CMove *genMoves)
{
    int nMoves;
    int chance = pos->chance;
    if (chance == BLACK)
    {
        nMoves = generateMoves<BLACK>(pos, genMoves);
    }
    else
    {
        nMoves = generateMoves<WHITE>(pos, genMoves);
    }
    return nMoves;
}


template int BitBoardUtils::generateCaptures<WHITE>(const ExpandedBitBoard *bb, CMove *genMoves);
template int BitBoardUtils::generateCaptures<BLACK>(const ExpandedBitBoard *bb, CMove *genMoves);


template int BitBoardUtils::generateNonCaptures<WHITE>(const ExpandedBitBoard *bb, CMove *genMoves);
template int BitBoardUtils::generateNonCaptures<BLACK>(const ExpandedBitBoard *bb, CMove *genMoves);

template int BitBoardUtils::generateMovesOutOfCheck<WHITE>(const ExpandedBitBoard *bb, CMove *genMoves);
template int BitBoardUtils::generateMovesOutOfCheck<BLACK>(const ExpandedBitBoard *bb, CMove *genMoves);


template int BitBoardUtils::generateMovesCausingCheck<WHITE>(const ExpandedBitBoard *bb, CMove *genMoves);
template int BitBoardUtils::generateMovesCausingCheck<BLACK>(const ExpandedBitBoard *bb, CMove *genMoves);


template bool BitBoardUtils::generateFirstLVACaptureForSquare<WHITE>(uint64 square, uint64 pinned, uint64 threatened, uint64 myKing, uint8 kingIndex, uint64 allPieces,
    uint64 myPawns, uint64 myNonPinnedKnights, uint64 myBishops, uint64 myRooks, uint64 myQueens, CMove *genMove);

template bool BitBoardUtils::generateFirstLVACaptureForSquare<BLACK>(uint64 square, uint64 pinned, uint64 threatened, uint64 myKing, uint8 kingIndex, uint64 allPieces,
    uint64 myPawns, uint64 myNonPinnedKnights, uint64 myBishops, uint64 myRooks, uint64 myQueens, CMove *genMove);
