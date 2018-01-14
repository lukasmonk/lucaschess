#include "chess.h"


// Utilsity functions for reading FEN String, EPD file, displaying board, etc


// used by the getPieceChar function
static char pieceCharMapping[] = { '.', 'P', 'N', 'B', 'R', 'Q', 'K' };

// gets the numeric code of the piece represented by a character
uint8 Utils::getPieceCode(char piece)
{
    switch (piece) {
    case 'p':
        return COLOR_PIECE(BLACK, PAWN);
    case 'n':
        return COLOR_PIECE(BLACK, KNIGHT);
    case 'b':
        return COLOR_PIECE(BLACK, BISHOP);
    case 'r':
        return COLOR_PIECE(BLACK, ROOK);
    case 'q':
        return COLOR_PIECE(BLACK, QUEEN);
    case 'k':
        return COLOR_PIECE(BLACK, KING);
    case 'P':
        return COLOR_PIECE(WHITE, PAWN);
    case 'N':
        return COLOR_PIECE(WHITE, KNIGHT);
    case 'B':
        return COLOR_PIECE(WHITE, BISHOP);
    case 'R':
        return COLOR_PIECE(WHITE, ROOK);
    case 'Q':
        return COLOR_PIECE(WHITE, QUEEN);
    case 'K':
        return COLOR_PIECE(WHITE, KING);
    default:
        return EMPTY_SQUARE;

    }
}

// Gets char representation of a piece code
char Utils::getPieceChar(uint8 code)
{
    uint8 color = COLOR(code);
    uint8 piece = PIECE(code);
    char pieceChar = '.';

    if (code != EMPTY_SQUARE)
    {
        assert((color == WHITE) || (color == BLACK));
        assert((piece >= PAWN) && (piece <= KING));
        pieceChar = pieceCharMapping[piece];
    }

    if (color == BLACK)
    {
        pieceChar += ('p' - 'P');
    }
    return pieceChar;
}


/*
Format of board in text file (e.g. Starting Position):

rnbqkbnr
pppppppp
........
........
........
........
PPPPPPPP
RNBQKBNR
*/

// methods to read a board from text file
void Utils::readBoardFromFile(char filename[], char board[8][8]) {
    FILE * fp = fopen(filename, "r");
    char buf[100];
    for (int i = 0; i<8; i++) {
        fscanf(fp, "%s", buf);
        for (int j = 0; j<8; j++)
            board[i][j] = buf[j];
    }
    fclose(fp);
}

// convert to char board
void Utils::boardCharTo088(BoardPosition088 *pos, char board[8][8])
{
    int i, j;
    int index088 = 0;

    for (i = 7; i >= 0; i--)
    {
        for (j = 0; j < 8; j++)
        {
            char piece = board[i][j];
            pos->board[index088] = getPieceCode(piece);
            index088++;
        }
        // skip 8 cells of padding
        index088 += 8;
    }
}

void Utils::readBoardFromFile(char filename[], BoardPosition088 *pos)
{
    char board[8][8];
    readBoardFromFile(filename, board);
    boardCharTo088(pos, board);
}

// methods to display the board (in the above form)

void Utils::dispBoard(char board[8][8])
{
    int i, j;
    for (i = 0; i<8; i++) {
        for (j = 0; j<8; j++)
            printf("%c", board[i][j]);
        printf("\n");
    }
}

// convert to char board
void Utils::board088ToChar(char board[8][8], BoardPosition088 *pos)
{
    int i, j;
    int index088 = 0;

    for (i = 7; i >= 0; i--)
    {
        for (j = 0; j < 8; j++)
        {
            char piece = getPieceChar(pos->board[index088]);
            board[i][j] = piece;
            index088++;
        }
        // skip 8 cells of padding
        index088 += 8;
    }
}

// convert 088 board to hex bit board
void Utils::board088ToHexBB(HexaBitBoardPosition *posBB, BoardPosition088 *pos088)
{
    memset(posBB, 0, sizeof(HexaBitBoardPosition));

    for (uint8 i = 0; i<64; i++)
    {
        uint8 rank = i >> 3;
        uint8 file = i & 7;
        uint8 index088 = INDEX088(rank, file);
        uint8 colorpiece = pos088->board[index088];
        if (colorpiece != EMPTY_SQUARE)
        {
            uint8 color = COLOR(colorpiece);
            uint8 piece = PIECE(colorpiece);
            if (color == WHITE)
            {
                posBB->whitePieces |= BIT(i);
            }
            switch (piece)
            {
            case PAWN:
                posBB->pawns |= BIT(i);
                break;
            case KNIGHT:
                posBB->knights |= BIT(i);
                break;
            case BISHOP:
                posBB->bishopQueens |= BIT(i);
                break;
            case ROOK:
                posBB->rookQueens |= BIT(i);
                break;
            case QUEEN:
                posBB->bishopQueens |= BIT(i);
                posBB->rookQueens |= BIT(i);
                break;
            case KING:
                posBB->kings |= BIT(i);
                break;
            }
        }
    }

    posBB->chance = pos088->chance;
    posBB->blackCastle = pos088->blackCastle;
    posBB->whiteCastle = pos088->whiteCastle;
    posBB->enPassent = pos088->enPassent;
    posBB->halfMoveCounter = pos088->halfMoveCounter;
}

// convert bitboard to 088 board
void Utils::boardHexBBTo088(BoardPosition088 *pos088, HexaBitBoardPosition *posBB)
{
    memset(pos088, 0, sizeof(BoardPosition088));


    uint64 queens = posBB->bishopQueens & posBB->rookQueens;

    uint64 pawns = posBB->pawns & RANKS2TO7;

    uint64 allPieces = posBB->kings | posBB->knights | pawns | posBB->rookQueens | posBB->bishopQueens;

    for (uint8 i = 0; i<64; i++)
    {
        uint8 rank = i >> 3;
        uint8 file = i & 7;
        uint8 index088 = INDEX088(rank, file);

        if (allPieces & BIT(i))
        {
            uint8 color = (posBB->whitePieces & BIT(i)) ? WHITE : BLACK;
            uint8 piece = 0;
            if (posBB->kings & BIT(i))
            {
                piece = KING;
            }
            else if (posBB->knights & BIT(i))
            {
                piece = KNIGHT;
            }
            else if (pawns & BIT(i))
            {
                piece = PAWN;
            }
            else if (queens & BIT(i))
            {
                piece = QUEEN;
            }
            else if (posBB->bishopQueens & BIT(i))
            {
                piece = BISHOP;
            }
            else if (posBB->rookQueens & BIT(i))
            {
                piece = ROOK;
            }
            assert(piece);

            pos088->board[index088] = COLOR_PIECE(color, piece);
        }
    }

    pos088->chance = posBB->chance;
    pos088->blackCastle = posBB->blackCastle;
    pos088->whiteCastle = posBB->whiteCastle;
    pos088->enPassent = posBB->enPassent;
    pos088->halfMoveCounter = posBB->halfMoveCounter;
}

void Utils::dispBoard(HexaBitBoardPosition *pos)
{
    BoardPosition088 board;
    boardHexBBTo088(&board, pos);
    dispBoard(&board);
}

void Utils::dispBoard(BoardPosition088 *pos)
{
    char board[8][8];
    board088ToChar(board, pos);

    printf("\nBoard Position: \n");
    dispBoard(board);
    printf("\nGame State: \n");

    if (pos->chance == WHITE)
    {
        printf("White to move\n");
    }
    else
    {
        printf("Black to move\n");
    }

    if (pos->enPassent)
    {
        printf("En passent allowed for file: %d\n", pos->enPassent);
    }

    printf("Allowed Castlings:\n");

    if (pos->whiteCastle & CASTLE_FLAG_KING_SIDE)
        printf("White King Side castle\n");

    if (pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE)
        printf("White Queen Side castle\n");

    if (pos->blackCastle & CASTLE_FLAG_KING_SIDE)
        printf("Black King Side castle\n");

    if (pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE)
        printf("Black Queen Side castle\n");
}

void Utils::clearBoard(BoardPosition088 *pos)
{
    for (int i = 0; i<8; i++)
        for (int j = 0; j<8; j++)
            pos->board[INDEX088(i, j)] = EMPTY_SQUARE;
}

// displays a move object
void Utils::displayMove(Move move)
{
    char dispString[10];

    uint8 r1, c1, r2, c2;
    r1 = RANK(move.src) + 1;
    c1 = FILE(move.src);

    r2 = RANK(move.dst) + 1;
    c2 = FILE(move.dst);

    char sep = move.capturedPiece ? '*' : '-';

    sprintf(dispString, "%c%d%c%c%d ",
        c1 + 'a',
        r1,
        sep,
        c2 + 'a',
        r2);

    printf(dispString);
}

void Utils::displayCompactMove(CMove move)
{
#if 0
    Move move2;
    move2.capturedPiece = (move.getFlags() & CM_FLAG_CAPTURE);
    move2.src = move.getFrom();
    move2.dst = move.getTo();
    displayMoveBB(move2);
#endif

    char dispString[10];

    uint8 src = move.getFrom();
    uint8 dst = move.getTo();

    uint8 r1, c1, r2, c2;
    r1 = (src >> 3) + 1;
    c1 = (src) & 0x7;

    r2 = (dst >> 3) + 1;
    c2 = (dst) & 0x7;

    char promo;

    if (move.getFlags() & CM_FLAG_PROMOTION)
    {
        if ((move.getFlags() & CM_FLAG_QUEEN_PROMOTION) == CM_FLAG_QUEEN_PROMOTION)
            promo = 'q';
        else if ((move.getFlags() & CM_FLAG_KNIGHT_PROMOTION) == CM_FLAG_KNIGHT_PROMOTION)
            promo = 'n';
        else if ((move.getFlags() & CM_FLAG_ROOK_PROMOTION) == CM_FLAG_ROOK_PROMOTION)
            promo = 'r';
        else if ((move.getFlags() & CM_FLAG_BISHOP_PROMOTION) == CM_FLAG_BISHOP_PROMOTION)
            promo = 'b';

        sprintf(dispString, "%c%d%c%d%c ",
            c1 + 'a',
            r1,
            c2 + 'a',
            r2,
            promo);
    }
    else
    {
        sprintf(dispString, "%c%d%c%d ",
            c1 + 'a',
            r1,
            c2 + 'a',
            r2);
    }
    printf(dispString);

}

void Utils::displayMoveBB(Move move)
{
    char dispString[10];

    uint8 r1, c1, r2, c2;
    r1 = (move.src >> 3) + 1;
    c1 = (move.src) & 0x7;

    r2 = (move.dst >> 3) + 1;
    c2 = (move.dst) & 0x7;

    char sep = move.capturedPiece ? '*' : '-';

    /*
    sprintf(dispString, "%c%d%c%c%d \n",
        c1 + 'a',
        r1,
        sep,
        c2 + 'a',
        r2);
    */
    // seperator not allowed in uci protocol
    // TODO: also handle promotion-capture moves correctly?
    sprintf(dispString, "%c%d%c%d ",
        c1 + 'a',
        r1,
        c2 + 'a',
        r2);

    printf(dispString);
}


// reads a FEN string into the given BoardPosition088 object

/*
Reference: (Wikipedia)

A FEN record contains 6 fields. The separator between fields is a space. The fields are:

1. Piece placement (from white's perspective). Each rank is described, starting with rank 8 and ending with rank 1; within each rank, the contents of each square are described from file a through file h. White pieces are designated using upper-case letters ("KQRBNP"), Black by lowercase ("kqrbnp"). Blank squares are noted using digits 1 through 8 (the number of blank squares), and "/" separate ranks.
2. Active color. "w" means white moves next, "b" means black.
3. Castling availability. If neither side can castle, this is "-". Otherwise, this has one or more letters: "K" (White can castle kingside), "Q" (White can castle queenside), "k" (Black can castle kingside), and/or "q" (Black can castle queenside).
4. En passant target square in algebraic notation. If there's no en passant target square, this is "-". If a pawn has just made a 2-square move, this is the position "behind" the pawn.
5. Halfmove clock: This is the number of halfmoves since the last pawn advance or capture. This is used to determine if a draw can be claimed under the fifty move rule.
6. Fullmove number: The number of the full move. It starts at 1, and is incremented after Black's move.

*/
int Utils::readFENString(char fen[], BoardPosition088 *pos)
{
    int i, j;
    char curChar;
    int row = 0, col = 0;

    memset(pos, 0, sizeof(BoardPosition088));

    // 1. read the board
    for (i = 0; fen[i]; i++)
    {
        curChar = fen[i];


        if (curChar == '/' || curChar == '\\')
        {
            row++; col = 0;
        }
        else if (curChar >= '1' && curChar <= '8')
        {	// blank squares
            for (j = 0; j < curChar - '0'; j++)
            {
                pos->board[INDEX088(7 - row, col)] = getPieceCode(curChar);
                col++;
            }
        }
        else if (curChar == 'k' || curChar == 'q' || curChar == 'r' || curChar == 'b' || curChar == 'n' || curChar == 'p' ||
            curChar == 'K' || curChar == 'Q' || curChar == 'R' || curChar == 'B' || curChar == 'N' || curChar == 'P')
        {
            pos->board[INDEX088(7 - row, col)] = getPieceCode(curChar);
            col++;
        }

        if (row >= 7 && col == 8) break;		// done with the board, set the flags now

    }

    i++;

    // 2. read the chance
    while (fen[i] == ' ')
        i++;

    if (fen[i] == 'b' || fen[i] == 'B')
    {
        pos->chance = BLACK;
    }
    else
    {
        pos->chance = WHITE;
    }

    i++;

    // 3. read the castle flags
    pos->whiteCastle = pos->blackCastle = 0;

    while (fen[i] == ' ')
        i++;

    while (fen[i] != ' ') {
        switch (fen[i]) {
        case 'k':
            pos->blackCastle |= CASTLE_FLAG_KING_SIDE;
            break;
        case 'q':
            pos->blackCastle |= CASTLE_FLAG_QUEEN_SIDE;
            break;
        case 'K':
            pos->whiteCastle |= CASTLE_FLAG_KING_SIDE;
            break;
        case 'Q':
            pos->whiteCastle |= CASTLE_FLAG_QUEEN_SIDE;
            break;
        }
        i++;
    }

    // 4. read en-passent flag
    pos->enPassent = 0;

    while (fen[i] == ' ')
        i++;

    if (fen[i] >= 'a' && fen[i] <= 'h')
        pos->enPassent = fen[i] - 'a' + 1;

    while (fen[i] != ' ' && fen[i])
        i++;

    while (fen[i] == ' ')
        i++;

    //5. read the half-move and the full move clocks
    int halfMove = atoi(&fen[i]);

    while (fen[i] != ' ' && fen[i])
        i++;

    int fullMove = atoi(&fen[i]);
    pos->halfMoveCounter = halfMove;

    return fullMove;
}

// TODO: move this to bitboard utils
// figure out which pice is present at a square (on bitboard)
uint8 getPieceAtSquare(const HexaBitBoardPosition *pos, int sq)
{
    uint64 src = BIT(sq);

    uint8 piece = 0;

    uint64 queens = pos->bishopQueens & pos->rookQueens;

    if (pos->kings & src)
        piece = KING;
    else if (pos->knights & src)
        piece = KNIGHT;
    else if ((pos->pawns & RANKS2TO7) & src)
        piece = PAWN;
    else if (queens & src)
        piece = QUEEN;
    else if (pos->bishopQueens & src)
        piece = BISHOP;
    else if (pos->rookQueens & src)
        piece = ROOK;

    return piece;
}


#ifndef _WIN64
#ifndef _WIN32
// I get compile error if I try to define abs in win 64 bit build
int abs(int x)
{
    if (x < 0)
        return -x;
    else
        return x;
}
#endif
#endif

// reads a move in algebric notation into a move_list_item
// returns the number of characters read from input string
int Utils::readMove(const char *input, const HexaBitBoardPosition *pos, CMove *move) 
{
    int r1, c1, r2, c2, q;

    int charsRead = 4;  // normally all moves read 4 chars, except for promotion

    // ignore blank spaces
    while (*input == ' ') 
        input++;

    r1 = input[1] - '1';
    c1 = input[0] - 'a';
    r2 = input[3] - '1';
    c2 = input[2] - 'a';
    q = input[4];

    assert(r1 >= 0);
    assert(r2 >= 0);
    assert(c1 >= 0);
    assert(c2 >= 0);

    assert(r1 < 8);
    assert(r2 < 8);
    assert(c1 < 8);
    assert(c2 < 8);
    
    int from = (r1 << 3) | c1;
    int to   = (r2 << 3) | c2;
    int flags = 0;

    uint8 srcPiece = getPieceAtSquare(pos, from);
    uint8 oldPiece = getPieceAtSquare(pos, to);

    if (oldPiece)
    {
        flags |= CM_FLAG_CAPTURE;
    }

    // special moves

    // 1. Promotion
    if ((srcPiece == PAWN) && (r2 == 0 || r2 == 7)) 
    {
        int promotedPiece = PIECE(Utils::getPieceCode(q));
        switch (promotedPiece)
        {
        case KNIGHT:
            flags |= CM_FLAG_KNIGHT_PROMOTION;
            break;
        case BISHOP:
            flags |= CM_FLAG_BISHOP_PROMOTION;
            break;
        case ROOK:
            flags |= CM_FLAG_ROOK_PROMOTION;
            break;
        case QUEEN:
            flags |= CM_FLAG_QUEEN_PROMOTION;
            break;

        }
        // special case, reads 5 characters from input
        charsRead = 5;
    }
    // 2. En-Passent
    else if (srcPiece == PAWN && oldPiece == 0 && c1 != c2)
    {
        flags |= CM_FLAG_EP_CAPTURE;
    }
    // 3. Castling
    else if (srcPiece == KING && abs(c2 - c1) > 1) 
    {
        if (c2 > c1) // king side
        {		
            flags |= CM_FLAG_KING_CASTLE;
        }
        else 
        {
            flags |= CM_FLAG_QUEEN_CASTLE;
        }
    }
    // 4. double pawn push
    else if (srcPiece == PAWN && abs(r2 - r1) == 2)
    {
        flags |= CM_FLAG_DOUBLE_PAWN_PUSH;
    }

    *move = CMove(from, to, flags);
    return charsRead;
}






// reads a epd file with perft values and compares the values with the ones generated by the move generator
/*
void Utils::testPerftFile() {
char buf[200][1000];
unsigned long long realNodes[10];
int n=0;
FILE * fp = fopen("c:\\deltachess\\reference\\perftsuite.epd", "r");
while(fgets(buf[n++], 995, fp));
fclose(fp);

for(maxDepth=1;maxDepth<7;maxDepth++) {
for(int i=0;i<n-1;i++) {
readFENString(buf[i]);
readNodeCounts(buf[i], realNodes);

dispBoard(board);
printf("%d %d %d %d\n\n", chance, castleflagB, castleflagW, enPassentFlag);

nodeCount=0;
minMax(0);
printf("\nNode Counts:  Actual - %llu, Real - %llu", nodeCount, realNodes[maxDepth]);
if(nodeCount!=realNodes[maxDepth])
printf("  #### FAILURE #####\n");
else
printf("  SUCCESS \n");
printf("\n-------------------------------------------------------------------\n\n");
}
printf("\n*********************END OF DEPTH %d **********************\n\n", maxDepth);
}


}
*/