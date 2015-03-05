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

/// Some code is released in the public domain by Michel Van den Bergh.

#include "OpenBook.h"

OpenBook::OpenBook()  : openBookFile ( nullptr ), Random64 ( nullptr ) {
    srand ( time ( NULL ) );
}

void OpenBook::dispose() {
    if ( openBookFile ) { fclose ( openBookFile ); }

    if ( Random64 ) { free ( Random64 ); }

    openBookFile = nullptr;
    Random64 = nullptr;
}

OpenBook::~OpenBook() {
    dispose();
}

u64 OpenBook::createKey ( string fen1 ) {
    const char* fen = ( const char* ) fen1.c_str() ;
    u64 *RandomPiece     = Random64;
    u64 *RandomCastle    = Random64 + 768;
    u64 *RandomEnPassant = Random64 + 772;
    u64 *RandomTurn      = Random64 + 780;
    static const char *piece_names = "pPnNbBrRqQkK";
    char board_s[72 + 1];
    char to_move_c;
    char castle_flags_s[4 + 1];
    char ep_square_s[2 + 1];
    char board[8][8];
    char c;
    int r, f, i, p_enc;
    unsigned p;
    u64 key = 0;
    sscanf ( fen, "%72s %c %4s %2s",
             board_s,
             &to_move_c,
             castle_flags_s,
             ep_square_s );
    r = 7;
    f = 0;
    p = 0;

    while ( 1 ) {
        if ( p >= strlen ( board_s ) ) { break; }

        c = board_s[p++];

        if ( c == '/' ) {
            r--;
            f = 0;
            continue;
        }

        if ( ( '1' <= c ) && ( '8' >= c ) ) {
            for ( i = 0; i <= c - '1'; i++ ) {
                board[f++][r] = '-';
            }

            continue;
        }

        board[f++][r] = c;
    }

    for ( f = 0; f <= 7; f++ ) {
        for ( r = 0; r <= 7; r++ ) {
            c = board[f][r];

            if ( c != '-' ) {
                p_enc = strchr ( piece_names, c ) - piece_names;
                key ^= RandomPiece[64 * p_enc + 8 * r + f];
            }
        }
    }

    p = 0;

    while ( 1 ) {
        if ( p >= strlen ( castle_flags_s ) ) { break; }

        c = castle_flags_s[p++];

        switch ( c ) {
            case '-' :
                break;

            case 'K' :
                key ^= RandomCastle[0];
                break;

            case 'Q' :
                key ^= RandomCastle[1];
                break;

            case 'k' :
                key ^= RandomCastle[2];
                break;

            case 'q' :
                key ^= RandomCastle[3];
                break;

            default:
                break;
        }
    }

    if ( ep_square_s[0] != '-' ) {
        f = ep_square_s[0] - 'a';

        if ( to_move_c == 'b' ) {
            if ( ( f > 0 && board[f - 1][3] == 'p' ) ||
                    ( f < 7 && board[f + 1][3] == 'p' ) ) {
                key ^= RandomEnPassant[f];
            }
        }
        else {
            if ( ( f > 0 && board[f - 1][4] == 'P' ) ||
                    ( f < 7 && board[f + 1][4] == 'P' ) ) {
                key ^= RandomEnPassant[f];
            }
        }
    }

    if ( to_move_c == 'w' ) {
        key ^= RandomTurn[0];
    }

    return key;
}

bool OpenBook::load ( string fileName ) {
    dispose();

    if ( !_file::fileExists ( fileName ) ) {
        cout << fileName << " not found" << endl;
        return false;
    }

    openBookFile = fopen ( fileName.c_str(), "rb" );
    Random64 = ( u64* ) malloc ( 781 * sizeof ( u64 ) );
    int k = 0;

    for ( int i = 0; i < 15 && k < 781; i++ ) {
        for ( int j = 0; j < 64 && k < 781; j++ ) {
            Random64[k++] = _random::RANDOM_KEY[i][j];
        }
    }

    return true;
}

int OpenBook::intFromFile ( int l, u64 *r ) {
    int c;

    for ( int i = 0; i < l; i++ ) {
        c = fgetc ( openBookFile );

        if ( c == EOF ) {
            return 1;
        }

        ( *r ) = ( ( *r ) << 8 ) + c;
    }

    return 0;
}

int OpenBook::entryFromFile ( entry_t *entry ) {
    u64 r = 0;

    if ( intFromFile ( 8, &r ) ) {
        return 1;
    }

    entry->key = r;

    if ( intFromFile ( 2, &r ) ) {
        return 1;
    }

    entry->move = r;

    if ( intFromFile ( 2, &r ) ) {
        return 1;
    }

    entry->weight = r;

    if ( intFromFile ( 4, &r ) ) {
        return 1;
    }

    entry->learn = r;
    return 0;
}

int OpenBook::findKey ( u64 key, entry_t *entry ) {
    int first, last, middle;
    static const entry_t entry_none = { 0, 0, 0, 0 };
    entry_t first_entry = entry_none, last_entry, middle_entry;
    first = -1;

    if ( fseek ( openBookFile, -16, SEEK_END ) ) {
        *entry = entry_none;
        entry->key = key + 1;
        return -1;
    }

    last = ftell ( openBookFile ) / 16;
    entryFromFile ( &last_entry );

    while ( 1 ) {
        if ( last - first == 1 ) {
            *entry = last_entry;
            return last;
        }

        middle = ( first + last ) / 2;
        fseek ( openBookFile, 16 * middle, SEEK_SET );
        entryFromFile ( &middle_entry );

        if ( key <= middle_entry.key ) {
            last = middle;
            last_entry = middle_entry;
        }
        else {
            first = middle;
            first_entry = middle_entry;
        }
    }
}

void OpenBook::moveToString ( char move_s[6], unsigned short move ) {
    const static char *promote_pieces = " nbrq";
    int f, fr, ff, t, tr, tf, p;
    f = ( move >> 6 ) & 077;
    fr = ( f >> 3 ) & 0x7;
    ff = f & 0x7;
    t = move & 077;
    tr = ( t >> 3 ) & 0x7;
    tf = t & 0x7;
    p = ( move >> 12 ) & 0x7;
    move_s[0] = ff + 'a';
    move_s[1] = fr + '1';
    move_s[2] = tf + 'a';
    move_s[3] = tr + '1';

    if ( p ) {
        move_s[4] = promote_pieces[p];
        move_s[5] = '\0';
    }
    else {
        move_s[4] = '\0';
    }

    if ( !strcmp ( move_s, "e1h1" ) ) {
        strcpy ( move_s, "e1g1" );
    }
    else  if ( !strcmp ( move_s, "e1a1" ) ) {
        strcpy ( move_s, "e1c1" );
    }
    else  if ( !strcmp ( move_s, "e8h8" ) ) {
        strcpy ( move_s, "e8g8" );
    }
    else  if ( !strcmp ( move_s, "e8a8" ) ) {
        strcpy ( move_s, "e8c8" );
    }
}

string OpenBook::search ( string fen ) {
    u64 key = createKey ( fen );
    entry_t entry;
    char move_s[6];
    findKey ( key, &entry );

    if ( entry.key != key ) { return string(); }

    moveToString ( move_s, entry.move );
    return move_s;
}

