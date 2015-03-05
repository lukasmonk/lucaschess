/*
    Cinnamon is a UCI chess engine
    Copyright (C) 2011-2014 Giuseppe Cannella

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ChessBoard.h"

ChessBoard::ChessBoard() {
    fenString = string ( STARTPOS );
    memset ( &structure, 0, sizeof ( _Tboard ) );
    sideToMove = loadFen ( fenString );
}

ChessBoard::~ChessBoard() {
}
#ifdef DEBUG_MODE
u64 ChessBoard::getBitBoard ( int side ) {
    return side ? getBitBoard<WHITE>() : getBitBoard<BLACK>();
}

int ChessBoard::getPieceAt ( int side, u64 bitmapPos ) {
    return side ? getPieceAt<WHITE> ( bitmapPos ) : getPieceAt<BLACK> ( bitmapPos );
}
#endif

uchar ChessBoard::getRightCastle() {
    return rightCastle;
}

void ChessBoard::setRightCastle ( uchar r ) {
    rightCastle = r;
}

void ChessBoard::makeZobristKey() {
    zobristKey = 0;
    int i = 0;

    for ( u64 c : chessboard ) {
        while ( c ) {
            int position = BITScanForward ( c );
            updateZobristKey ( i, position );
            c &= NOTPOW2[position];
        }

        i++;
    }

    if ( enpassantPosition != NO_ENPASSANT ) {
        updateZobristKey ( 13, enpassantPosition );
    }

    u64 x2 = rightCastle;

    while ( x2 ) {
        int position = BITScanForward ( x2 );
        updateZobristKey ( 14, position );
        x2 &= NOTPOW2[position];
    }
}

string ChessBoard::getFen() {
    return fenString;
}

int ChessBoard::getPieceByChar ( char c ) {
    for ( int i = 0; i < 12; i++ )
        if ( c == FEN_PIECE[i] ) { return i; }

    return -1;
}


void ChessBoard::display() {
    cout  << "\n     a   b   c   d   e   f   g   h";

    for ( int t = 0; t <= 63; t++ ) {
        char x = ' ';

        if ( t % 8 == 0 ) {
            cout  << "\n   ----+---+---+---+---+---+---+----\n";
            cout << " " << 8 - RANK_AT[t] << " | ";
        }

        x = ( x = ( x = FEN_PIECE[getPieceAt<WHITE> ( POW2[63 - t] )] ) != '-' ? x : FEN_PIECE[getPieceAt<BLACK> ( POW2[63 - t] )] ) == '-' ? ' ' : x;
        x != ' ' ? cout << x : POW2[t] & WHITE_SQUARES ? cout << " " : cout << ".";
        cout << " | ";
    };

    cout << "\n   ----+---+---+---+---+---+---+----\n";

    cout << "     a   b   c   d   e   f   g   h\n\n\n" << boardToFen() << "\n" << endl;
}


string ChessBoard::boardToFen() {
    string fen;
    int x, y, l = 0, i = 0, sq;
    char row[8];
    int q;

    for ( y = 0; y < 8; y++ ) {
        i = l = 0;
        strcpy ( row, "" );

        for ( x = 0; x < 8; x++ ) {
            sq = ( y * 8 ) + x;
            q = getPieceAt<BLACK> ( POW2[63 - sq] );

            if ( q == SQUARE_FREE ) {
                q = getPieceAt<WHITE> ( POW2[63 - sq] );
            }

            if ( q == SQUARE_FREE ) {
                l++;
            }
            else {
                if ( l > 0 ) {
                    row[i] = ( char ) ( l + 48 );
                    i++;
                }

                l = 0;
                row[i] = FEN_PIECE[q];
                i++;
            }
        }

        if ( l > 0 ) {
            row[i] = ( char ) ( l + 48 );
            i++;
        }

        fen.append ( row, i );

        if ( y < 7 ) {
            fen.append ( "/" );
        }
    }

    if ( sideToMove == BLACK ) {
        fen.append ( " b " );
    }
    else {
        fen.append ( " w " );
    }

    int cst = 0;

    if ( rightCastle & RIGHT_KING_CASTLE_WHITE_MASK ) {
        fen.append ( "K" );
        cst++;
    }

    if ( rightCastle & RIGHT_QUEEN_CASTLE_WHITE_MASK ) {
        fen.append ( "Q" );
        cst++;
    }

    if ( rightCastle & RIGHT_KING_CASTLE_BLACK_MASK ) {
        fen.append ( "k" );
        cst++;
    }

    if ( rightCastle & RIGHT_QUEEN_CASTLE_BLACK_MASK ) {
        fen.append ( "q" );
        cst++;
    }

    if ( !cst ) {
        fen.append ( "-" );
    }

    if ( enpassantPosition == NO_ENPASSANT ) {
        fen.append ( " -" );
    }
    else {
        fen.append ( " " );
        sideToMove ? fen.append ( BOARD[enpassantPosition + 8] ) : fen.append ( BOARD[enpassantPosition - 8] );
    }

    return fen;
}

string ChessBoard::decodeBoardinv ( const uchar type, const int a, const int side ) {
    if ( type & QUEEN_SIDE_CASTLE_MOVE_MASK && side == WHITE ) {
        return "e1c1";
    }

    if ( type & KING_SIDE_CASTLE_MOVE_MASK && side == WHITE ) {
        return "e1g1";
    }

    if ( type & QUEEN_SIDE_CASTLE_MOVE_MASK && side == BLACK ) {
        return "e8c8";
    }

    if ( type & KING_SIDE_CASTLE_MOVE_MASK && side == BLACK ) {
        return "e8g8";
    }

    ASSERT ( ! ( type & 0xC ) );

    if ( a >= 0 && a < 64 ) {
        return BOARD[a];
    }

    assert ( 0 );
    return "";
}

char ChessBoard::decodeBoard ( string a ) {
    for ( int i = 0; i < 64; i++ ) {
        if ( !a.compare ( BOARD[i] ) ) {
            return i;
        }
    }

    cout << "\n" << a << endl;
    ASSERT ( 0 );
    return -1;
}

int ChessBoard::loadFen() {
    return loadFen ( fenString );
}

int ChessBoard::loadFen ( string fen ) {
    if ( fen.empty() ) { return loadFen(); }

    istringstream iss ( fen );
    string pos, castle, enpassant, side;
    iss >> pos;
    iss >> side;
    iss >> castle;
    iss >> enpassant;
    int ix = 0;
    int s[64];

    for ( unsigned ii = 0; ii < pos.length(); ii++ ) {
        uchar ch = pos.at ( ii );

        if ( ch != '/' ) {
            if ( INV_FEN[ch] != 0xFF ) {
                s[ix++] = INV_FEN[ch];
            }
            else if ( ch > 47 && ch < 58 ) {
                for ( int t = 0; t < ch - 48; t++ ) {
                    s[ix++] = SQUARE_FREE;
                }
            }
            else {
                cout << "Bad FEN position format (" << ( char ) ch << ") " << fen << endl;
                return sideToMove;
            };
        }
    }

    if ( ix != 64 ) {
        cout << "Bad FEN position format " << fen << endl;
        return sideToMove;
    }

    if ( side == "b" ) {
        sideToMove = BLACK;
    }
    else if ( side == "w" ) {
        sideToMove = WHITE;
    }
    else {
        cout << "Bad FEN position format " << fen << endl;
        return sideToMove;
    }

    memset ( chessboard, 0, sizeof ( chessboard ) );

    for ( int i = 0; i < 64; i++ ) {
        int p = s[63 - i];

        if ( p != SQUARE_FREE ) {
            chessboard[p] |= POW2[i];
        }
    };

    rightCastle = 0;

    for ( unsigned e = 0; e < castle.length(); e++ ) {
        switch ( castle.at ( e ) ) {
            case 'K':
                rightCastle |= RIGHT_KING_CASTLE_WHITE_MASK;
                break;

            case 'k':
                rightCastle |= RIGHT_KING_CASTLE_BLACK_MASK;
                break;

            case 'Q':
                rightCastle |= RIGHT_QUEEN_CASTLE_WHITE_MASK;
                break;

            case 'q':
                rightCastle |= RIGHT_QUEEN_CASTLE_BLACK_MASK;
                break;

            default:
                ;
        };
    };

    friendKing[WHITE] = BITScanForward ( chessboard[KING_WHITE] );
    friendKing[BLACK] = BITScanForward ( chessboard[KING_BLACK ] );
    enpassantPosition = NO_ENPASSANT;

    for ( int i = 0; i < 64; i++ ) {
        if ( enpassant == BOARD[i] ) {
            enpassantPosition = i;

            if ( sideToMove ) {
                enpassantPosition -= 8;
            }
            else {
                enpassantPosition += 8;
            }

            break;
        }
    }

    makeZobristKey();
    return sideToMove;
}

