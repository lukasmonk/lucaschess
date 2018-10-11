#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "defs.h"
#include "protos.h"
#include "globals.h"

FILE * fpgn;

char * pgn;
char * w_pgn;
char * line;
int max_pgn;
char * pos_body;

char * pv;
char fen[64];
char * labels[256];
char * values[256];
int pos_label;
int raw;

char * fens[256];
int pos_fens;
int max_depth;
int max_line;


/*d = {"B":"WHITE_BISHOP", "P":"WHITE_PAWN", "Q":"WHITE_QUEEN", "R":"WHITE_ROOK", "N":"WHITE_KNIGHT", "K":"WHITE_KING"}
for x in "pnbrqk":
    d[x] = d[x.upper()].replace("WHITE", "BLACK")


for x in range(0, 115):
    n = d.get(chr(x),0)
    if x%12== 0:
        print
    print "%12s,"%n,
*/
unsigned PZNAME[] =
{
    0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,
    0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,
    0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,
    0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,
    0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,
    0,            0,            0,            0,            0,            0, WHITE_BISHOP,            0,            0,            0,            0,            0,
    0,            0,            0,   WHITE_KING,            0,            0, WHITE_KNIGHT,            0,   WHITE_PAWN,  WHITE_QUEEN,   WHITE_ROOK,            0,
    0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,            0,
    0,            0, BLACK_BISHOP,            0,            0,            0,            0,            0,            0,            0,            0,   BLACK_KING,
    0,            0, BLACK_KNIGHT,            0,   BLACK_PAWN,  BLACK_QUEEN,   BLACK_ROOK
};

char * pgn_game(void)
{
    return pgn;
}

void pgn_start(char * fich, int depth)
{
    int i;
    int c;

    if( depth > 256 ) depth = 256;
    max_depth = depth;

    fpgn = fopen(fich, "rb");
    max_pgn = 64*1024;
    max_line = 64*1024;
    pgn = (char *)malloc(max_pgn);
    pv = (char *)malloc(5*1024);
    line = (char *)malloc(max_line);
    for( i=0; i < 256; i++)
    {
        labels[i] = (char *) malloc(256);
        values[i] = (char *) malloc(256);
        fens[i] = (char *) malloc(128);
    }
    c = fgetc(fpgn);
    if( c == 0xef ) { // UTF-BOM
        c = fgetc(fpgn);
        if( c == 0xbb ) c = fgetc(fpgn);
        else rewind(fpgn);
    }
    else rewind(fpgn);
    w_pgn = pgn;
}

void pgn_stop( void )
{
    int i;
    fclose(fpgn);
    free(pgn);
    free(pv);
    free(line);
    for( i=0; i < 256; i++)
    {
        free(labels[i]);
        free(values[i]);
        free(fens[i]);
    }
}

bool empty_line()
{
    char * c;
    for(c = w_pgn; *c; c++ )
    {
        if(!isspace((int) (*c))) return false;
    }
    return true;
}

bool test_label()
{
    char * c;
    char * end;
    int n;
    size_t size;

    if(w_pgn[0] != '[') return false;
    size = strlen(w_pgn);

    end = w_pgn + size - 1;
    while (end >= w_pgn && isspace(*end))
        end--;
    if( end[0] != ']') return false;

    c = w_pgn;
    n = 0;
    while( *c ) {
        if( *c == '\\') c++;
        else {
            if( *c == '"') n++;
        }
        c++;
    }
    return n == 2;
}

void test_size_pgn(void)
{
    int tam_line, dif;
    tam_line = strlen(w_pgn);
    w_pgn += tam_line;
    dif = w_pgn-pgn;
    if((dif+1024) > max_pgn)
    {
        max_pgn += 64*1024;
        pgn = realloc(pgn, max_pgn);
        w_pgn = pgn + dif;
    }
}

void mas_label(void)
{
    char *c, *lk, *lv;

    if( pos_label > 255 || !test_label()) return;

    lk = labels[pos_label];
    lv = values[pos_label];

    c = w_pgn+1;
    while( *c && *c == ' ') c++; // fuera espacios
    while( *c && *c != '"' )     // hasta las "
    {
        if( *c != ' ' )
        {
            *lk = *c;
            lk++;
        }
        c++;
    }
    *lk = 0; // FDL
    if( *c == '"' )
    {
        c++;
        while( *c && *c != '"' )
        {
            if( *c == '\\' )
            {
                c++;
                if( ! *c ) break;
            }
            *lv = *c;
            lv++;
            c++;
        }
    }
    *lv = 0; // FDL

    if( !strcmp("FEN", labels[pos_label] ) )
    {
        strncpy(fen, values[pos_label], 63);
    }

    ++pos_label;
}


int pgn_read( void )
{
    long int max_tam;
    w_pgn = pgn;
    fen[0] = 0;
    pos_label = 0;
    pos_fens = 0;

    /* leemos primer label*/
    do
    {
        max_tam = max_pgn - (w_pgn-pgn);
        if(!fgets(w_pgn, max_tam, fpgn)) return false;
//        printf("PL:[%s]", w_pgn);

        if(w_pgn[0] == '[' )
        {
            mas_label();
            test_size_pgn();
            break;
        }
    }
    while(1);

    /* leemos resto labels */
    do
    {
        max_tam = max_pgn - (w_pgn-pgn);
        if(!fgets(w_pgn, max_tam, fpgn)) return false; /*EOF*/
        if(w_pgn[0] != '[') break;
//        printf("+L:[%s]", w_pgn);
        mas_label();
        test_size_pgn();
    }
    while(1);
//    printf("PR:[%s]", w_pgn);

    pos_body = w_pgn;
    test_size_pgn();

    /* leemos hasta linea en blanco */
    do
    {
        max_tam = max_pgn - (w_pgn-pgn);
        if(!fgets(w_pgn, max_tam, fpgn)) break; /*EOF*/
        if(w_pgn[0] == '[' && test_label()) {
            fseek( fpgn, -strlen(w_pgn)-1, SEEK_CUR );
//            printf("FR:[%s,%d]", w_pgn, -strlen(w_pgn)-1);
            w_pgn[0] = '\0';
            break;
        }
//        printf("+R:[%s]", w_pgn);
        test_size_pgn();
    }
    while(1);

    return true;
}


int pgn_gen_pv(void)
{
    char *c;
    char piece;
    char promotion;
    char from_AH, from_18;
    char to_AH, to_18;
    char *p_pv;
    bool ok;

    int par;
    int to;
    unsigned k;
    Move move;

    p_pv = pv;
    *p_pv = 0;

    raw = true;

    if( *fen ) fen_board( fen );
    else init_board();

    pos_fens = 0;

    c = pos_body;
    piece = 'P';
    from_AH = 0;
    from_18 = 0;
    to_AH = 0;
    to_18 = 0;
    promotion = 0;

    while(*c)
    {
        switch(*c)
        {
        case ' ':
        case '\r':
        case '\n':
            c++;
            break;

        case 'R':
        case 'K':
        case 'Q':
        case 'B':
        case 'N':
            piece = *c;
            promotion = 0;
            c++;
            if( *c >= '1' && *c <= '8' )
            {
                from_18 = *c;
                c++;
                if( *c == 'x' ) c++;
                if( *c >= 'a' && *c <= 'h' )
                {
                    to_AH = *c;
                    c++;
                    if( *c >= '1' && *c <= '8' )
                    {
                        to_18 = *c;
                        c++;
                    }
                }
            }
            break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
            from_AH = *c;
            c++;
            if( *c && *c >= '1' && *c <= '8' )
            {
                from_18 = *c;
                c++;
            }
            if( *c == 'x' ) c++;
            if( *c >= 'a' && *c <= 'h' )
            {
                to_AH = *c;
                c++;
                if( *c >= '1' && *c <= '8' )
                {
                    to_18 = *c;
                    c++;
                }
            }
            else
            {
                to_AH = from_AH;
                to_18 = from_18;
                from_AH = 0;
                from_18 = 0;
            }
            if( to_18 == 0 )
            {
                return false;
            }

            if( piece == 'P' && ( to_18 == '8' || to_18 == '1' ) )
            {
                if(*c == '=')
                {
                    c++;
                    if(*c == 'Q' || *c == 'R' || *c == 'B' || *c == 'N')
                    {
                        promotion = *c;
                        c++;
                    }
                    else if(*c == 'q' || *c == 'r' || *c == 'b' || *c == 'n')
                    {
                        promotion = *c + 'Q' - 'q';
                        c++;
                    }
                    else
                    {
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    {
                        return false;
                    }
                }
            }
            break;

        case '1':
            if( *(c+1) == '-' && *(c+2) == '0' )
            {
                *p_pv = 0;
                return true;
            }
            if( *(c+1) == '/' && *(c+2) == '2' && *(c+3) == '-' && *(c+4) == '1' && *(c+5) == '/' && *(c+6) == '2')
            {
                *p_pv = 0;
                return true;
            }
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            c++;
            while( *c >= '0' && *c <= '9' ) c++;
            while( *c == '.' ) c++;
            break;

        case '0':
            if( *(c+1) == '-' && *(c+2) == '1' )
            {
                *p_pv = 0;
                return true;
            }

        case 'O':
        case 'o':
            c++;
            if(*c == '-')
            {
                c++;
                if( *c == 'O' || *c == '0' || *c == 'o')
                {
                    c++;
                    piece = 'K';
                    from_AH = 'e';
                    from_18 = (board.color) ? '8':'1';
                    to_18 = from_18;
                    if( *c == '-' )
                    {
                        c++;
                        if( *c == 'O' || *c == '0' || *c == 'o')
                        {
                            to_AH = 'c';
                            c++;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        to_AH = 'g';
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
            break;

        case '%':
        case ';':
            while ( *c && !(*c == '\n'||*c == '\r') ) c++;
            if(raw) raw = false;
            break;

        case '(':
            par = 1;
            while ( *c && par )
            {
                c++;
                if( *c == '{' )
                {
                    while ( *c && *c != '}' ) c++;
                }
                if( *c == '(' ) par++;
                else if( *c == ')' ) par--;
            }
            if(raw) raw = false;
            break;

        case '{':
            while ( *c && *c != '}' ) c++;
            if(raw) raw = false;
            break;

        case '$':
            c++;
            if(raw) raw = false;
            break;

        default:
            c++;
        }

        if( to_18 )
        {
            to = (to_AH-'a') + (to_18-'1')*8;

            if(board.color)
            {
                piece +=  'a' - 'A';
                if( promotion ) promotion += 'a' - 'A';
            }

            /*movegen();*/
            /*movegen_piece(PZNAME[piece]);*/
            movegen_piece_to((int)PZNAME[(int)piece], (unsigned)to);
            ok = false;
            for (k = board.ply_moves[board.ply - 1]; k < board.ply_moves[board.ply]; k++)
            {
                move = board.moves[k];

                /*if( NAMEPZ[move.piece] == piece && move.to == to)*/
                if( move.to == to)
                {
                    if(from_AH && (move.from%8 != (from_AH-'a'))) continue;
                    if(from_18 && (move.from/8 != (from_18-'1'))) continue;
                    if( move.promotion && NAMEPZ[move.promotion] != promotion ) continue;
                    if( promotion && !move.promotion ) continue;
                    if( pv != p_pv )
                    {
                        *p_pv = ' ';
                        p_pv++;
                    }
                    strcpy(p_pv, POS_AH[move.from]);
                    p_pv += 2;
                    strcpy(p_pv, POS_AH[move.to]);
                    p_pv += 2;
                    if( promotion )
                    {
                        *p_pv = promotion;
                        p_pv++;
                    }

                    make_move(move);
                    if( pos_fens < max_depth ) board_fenM2( fens[pos_fens++] );
                    ok = true;
                    break;
                }
            }
            if( !ok ) return false;
            piece = 'P';
            from_AH = 0;
            from_18 = 0;
            to_AH = 0;
            to_18 = 0;
            promotion = 0;
        }
    }
    *p_pv = 0;
    return true;
}

char * pgn_pv(void)
{
    if( ! pgn_gen_pv() ) pv[0] = 0;
    return pv;
}

char * pgn_label(int num)
{
    return (char *)labels[num];
}

char * pgn_value(int num)
{
    return (char *)values[num];
}

int pgn_numlabels(void)
{
    return pos_label;
}

int pgn_raw(void)
{
    return raw;
}

char * pgn_fen(int num)
{
    return (char *)fens[num];
}

int pgn_numfens(void)
{
    return pos_fens;
}
