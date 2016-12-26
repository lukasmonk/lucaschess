#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

#define m "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
//6k1/7p/4p3/2r5/P3p3/8/2r1B1PP/R2K4 b - - 0 32

using namespace std;

s_Move newmove;

/*
I haven't commented this file at all - whenever I need a debug function,
I write it in here. Otherwise, these functions play no part in the program
*/

int plyok()
{
    if(histply >= 1024)  return TRUE;
    if(p->side < 0) return TRUE;
    if(p->side > 1) return TRUE;
    if(p->ply > 31) return TRUE;

    return FALSE;
}

int moveok(int move)
{
    int from = FROM(move);
    int to = TO(move);
    if(from > 144) return FALSE;
    if(to > 144) return FALSE;
    if(from <0 ) return FALSE;
    if(to <0 )  return FALSE;

    return TRUE;
}
//6rk/5pbp/3p2b1/4p3/8/2P5/PP3PPP/R4RK1 b - - 1 22
string test[2] = {
"6k1/4b3/p3p1r1/2p2pp1/2P2B2/PP6/8/1K4R1 b - - 0 43 ",
"r5k1/6bp/p3p3/3pp3/8/5P2/PPr1BKPP/RR6 b - - 0 1"
};

//this a q explosion test position
string qexp[1] = {
"1B1Q2K1/q1p4P/4P3/3Pk1p1/1r1NrR1b/4pn1P/1pRp2n1/1B2N2b w - - 0 1"
};


string positional[20] = {
"1qr3k1/p2nbppp/bp2p3/3p4/3P4/1P2PNP1/P2Q1PBP/1N2R1K1 b - - 0 1",
"1r2r1k1/3bnppp/p2q4/2RPp3/4P3/6P1/2Q1NPBP/2R3K1 w - - 0 1",
"2b1k2r/2p2ppp/1qp4n/7B/1p2P3/5Q2/PPPr2PP/R2N1R1K b k - 0 1",
"2b5/1p4k1/p2R2P1/4Np2/1P3Pp1/1r6/5K2/8 w - - 0 1",
"2brr1k1/ppq2ppp/2pb1n2/8/3NP3/2P2P2/P1Q2BPP/1R1R1BK1 w - - 0 1",
"2kr2nr/1pp3pp/p1pb4/4p2b/4P1P1/5N1P/PPPN1P2/R1B1R1K1 b - - 0 1",
"2r1k2r/1p1qbppp/p3pn2/3pBb2/3P4/1QN1P3/PP2BPPP/2R2RK1 b k - 0 1",
"2r1r1k1/pbpp1npp/1p1b3q/3P4/4RN1P/1P4P1/PB1Q1PB1/2R3K1 w - - 0 1",
"2r2k2/r4p2/2b1p1p1/1p1p2Pp/3R1P1P/P1P5/1PB5/2K1R3 w - - 0 1",
"2r3k1/5pp1/1p2p1np/p1q5/P1P4P/1P1Q1NP1/5PK1/R7 w - - 0 1",
"2r3qk/p5p1/1n3p1p/4PQ2/8/3B4/5P1P/3R2K1 w - - 0 1",
"3b4/3k1pp1/p1pP2q1/1p2B2p/1P2P1P1/P2Q3P/4K3/8 w - - 0 1",
"3n1r1k/2p1p1bp/Rn4p1/6N1/3P3P/2N1B3/2r2PP1/5RK1 w - - 0 1",
"3q1rk1/3rbppp/ppbppn2/1N6/2P1P3/BP6/P1B1QPPP/R3R1K1 w - - 0 1",
"3r1rk1/p1q4p/1pP1ppp1/2n1b1B1/2P5/6P1/P1Q2PBP/1R3RK1 w - - 0 1",
"3r2k1/2q2p1p/5bp1/p1pP4/PpQ5/1P3NP1/5PKP/3R4 b - - 0 1",
"3r2k1/p1q1npp1/3r1n1p/2p1p3/4P2B/P1P2Q1P/B4PP1/1R2R1K1 w - - 0 1",
"3r4/2k5/p3N3/4p3/1p1p4/4r3/PPP3P1/1K1R4 b - - 0 1",
"3r4/2R1np1p/1p1rpk2/p2b1p2/8/PP2P3/4BPPP/2R1NK2 w - - 0 1",
"3rk2r/1b2bppp/p1qppn2/1p6/4P3/PBN2PQ1/1PP3PP/R1B1R1K1 b k - 0 1"
};


string evalsee[10] = {
"4r1k1/p1Rp1npp/1p6/3b4/5P1q/1P6/PB1Q1PB1/6K1 w - - 0 1  ",
"5k2/8/5r2/8/8/8/2R5/5K2 w - - 0 1 ",
"5k2/8/5q2/8/8/8/2Q5/5K2 w - - 0 1 ",
"5k2/8/5b2/8/8/8/2B5/5K2 w - - 0 1 ",
"3B4/8/2B5/1K6/8/8/3p4/3k4 w - - ",
"1k1r4/1pp4p/2n5/P6R/2R1p1r1/2P2p2/1PP2B1P/4K3 b - - ",
"6k1/p3q2p/1nr3pB/8/3Q1P2/6P1/PP5P/3R2K1 b - - ",
"2krr3/1p4pp/p1bRpp1n/2p5/P1B1PP2/8/1PP3PP/R1K3B1 w - - ",
"r5k1/pp2p1bp/6p1/n1p1P3/2qP1NP1/2PQB3/P5PP/R4K2 b - - ",
"2r3k1/1qr1b1p1/p2pPn2/nppPp3/8/1PP1B2P/P1BQ1P2/5KRR w - - "
};

string bt2630[30] =
{
"rq2r1k1/5pp1/p7/4bNP1/1p2P2P/5Q2/PP4K1/5R1R w - - ",
"6k1/2b2p1p/ppP3p1/4p3/PP1B4/5PP1/7P/7K w - - ",
"5r1k/p1q2pp1/1pb4p/n3R1NQ/7P/3B1P2/2P3P1/7K w - - ",
"5r1k/1P4pp/3P1p2/4p3/1P5P/3q2P1/Q2b2K1/B3R3 w - - ",
"3B4/8/2B5/1K6/8/8/3p4/3k4 w - - ",
"1k1r4/1pp4p/2n5/P6R/2R1p1r1/2P2p2/1PP2B1P/4K3 b - - ",
"6k1/p3q2p/1nr3pB/8/3Q1P2/6P1/PP5P/3R2K1 b - - ",
"2krr3/1p4pp/p1bRpp1n/2p5/P1B1PP2/8/1PP3PP/R1K3B1 w - - ",
"r5k1/pp2p1bp/6p1/n1p1P3/2qP1NP1/2PQB3/P5PP/R4K2 b - - ",
"2r3k1/1qr1b1p1/p2pPn2/nppPp3/8/1PP1B2P/P1BQ1P2/5KRR w - - ",
"1br3k1/p4p2/2p1r3/3p1b2/3Bn1p1/1P2P1Pq/P3Q1BP/2R1NRK1 b - - ",
"8/pp3k2/2p1qp2/2P5/5P2/1R2p1rp/PP2R3/4K2Q b - - ",
"2bq3k/2p4p/p2p4/7P/1nBPPQP1/r1p5/8/1K1R2R1 b - -",
"3r1rk1/1p3pnp/p3pBp1/1qPpP3/1P1P2R1/P2Q3R/6PP/6K1 w - - ",
"2b1q3/p7/1p1p2kb/nPpN3p/P1P1P2P/6P1/5R1K/5Q2 w - - ",
"2krr3/pppb1ppp/3b4/3q4/3P3n/2P2N1P/PP2B1P1/R1BQ1RK1 b - - ",
"4r1k1/p1qr1p2/2pb1Bp1/1p5p/3P1n1R/3B1P2/PP3PK1/2Q4R w - - ",
"8/4p3/8/3P3p/P2pK3/6P1/7b/3k4 w - - ",
"3r2k1/pp4B1/6pp/PP1Np2n/2Pp1p2/3P2Pq/3QPPbP/R4RK1 b - - ",
"r4rk1/5p2/1n4pQ/2p5/p5P1/P4N2/1qb1BP1P/R3R1K1 w - - ",
"k7/8/PP1b2P1/K2Pn2P/4R3/8/6np/8 w - - ",
"rnb1k2r/pp2qppp/3p1n2/2pp2B1/1bP5/2N1P3/PP2NPPP/R2QKB1R w KQkq - ",
"8/7p/8/p4p2/5K2/Bpk3P1/4P2P/8 w - - ",
"R7/3p3p/8/3P2P1/3k4/1p5p/1P1NKP1P/7q w - - ",
"8/8/3k1p2/p2BnP2/4PN2/1P2K1p1/8/5b2 b - - ",
"2r3k1/pbr1q2p/1p2pnp1/3p4/3P1P2/1P1BR3/PB1Q2PP/5RK1 w - -",
"3r2k1/p2r2p1/1p1B2Pp/4PQ1P/2b1p3/P3P3/7K/8 w - - ",
"rnb1k1nr/p2p1ppp/3B4/1p1N1N1P/4P1P1/3P1Q2/PqP5/R4Kb1 w kq - ",
"r1b1kb1r/pp1n1ppp/2q5/2p3B1/Q1B5/2p2N2/PP3PPP/R3K2R w KQkq -",
"2k5/2p3Rp/p1pb4/1p2p3/4P3/PN1P1P2/1P2KP1r/8 w - - "
};

string mirrorevaltestpositions[44] =
{
"r1bqk2r/1pppbppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 4 6",
"rnbq1rk1/pppp1ppp/5n2/b3p3/4P3/P1N2N2/1PPPBPPP/R1BQK2R b KQ - 0 1",
"r5k1/1pbRRbpp/2p1p3/1PPp1pq1/3P4/4P3/2Q2PPP/r3N1K1 w - - 0 1 ",
"R3n1k1/2q2ppp/4p3/3p4/1ppP1PQ1/2P1P3/1PBrrBPP/R5K1 b - - 0 1 ",
"r2q1rk1/1bp1bppp/p2p1n2/4p3/1n2P3/1B1P1N1P/PP3PP1/RNBQR1K1 w - - 0 12",
"rnbqr1k1/pp3pp1/1b1p1n1p/1N2p3/4P3/P2P1N2/1BP1BPPP/R2Q1RK1 b - - 0 1",
"r1b3k1/4b1pp/p2p1nq1/2p1p3/1n2P3/2NPB2P/PP3PP1/R2QR1K1 w - - 2 18",
"r2qr1k1/pp3pp1/2npb2p/1N2p3/2P1P3/P2P1NQ1/4B1PP/R1B3K1 b - - 0 1",
"1r4k1/4b1pp/p2pbnq1/4p3/1nP1P3/2N1B2P/PP1Q1PP1/R3R2K b - - 0 21",
"r3r2k/pp1q1pp1/2n1b2p/1Np1p3/4P3/P2PBNQ1/4B1PP/1R4K1 w - - 0 1",
"4q1k1/4b1pp/Q1npbn2/2p1p3/4P3/P1NPB2P/1r3PP1/2RR3K b - - 1 23",
"2rr3k/1R3pp1/p1npb2p/4p3/2P1P3/q1NPBN2/4B1PP/4Q1K1 w - - 0 1",
"2b2qk1/PQ2b1pp/3p1n2/2p1p3/3nP3/2NPB2P/5PP1/1R5K w - - 1 33",
"1r5k/5pp1/2npb2p/3Np3/2P1P3/3P1N2/pq2B1PP/2B2QK1 b - - 0 1",
"rnbq1rk1/ppp2pbp/3p1np1/4p3/2PPP3/2N2N2/PP2BPPP/R1BQ1RK1 b - - 1 7",
"r1bq1rk1/pp2bppp/2n2n2/2ppp3/4P3/3P1NP1/PPP2PBP/RNBQ1RK1 w - - 0 1",
"r1bq1rk1/1pp1npbp/n2p2p1/3Pp3/2P1P3/B1N5/P2NBPPP/R2Q1RK1 b - - 2 14",
"r2q1rk1/p2nbppp/b1n5/2p1p3/3pP3/N2P2P1/1PP1NPBP/R1BQ1RK1 w - - 0 1",
"r1bq1rk1/2p4p/3p2pb/2pPpn2/P1P1N3/3B4/1B3PPP/R2Q1RK1 w - - 2 21",
"r2q1rk1/1b3ppp/3b4/p1p1n3/2PpPN2/3P2PB/2P4P/R1BQ1RK1 b - - 0 1",
"5rk1/2p2q1p/b2p2pb/P1pP4/2PpN3/1Q6/5PPP/4RBK1 w - - 0 28",
"4rbk1/5ppp/1q6/2pPn3/p1Pp4/B2P2PB/2P2Q1P/5RK1 b - - 0 1",
"5rk1/2p4p/b2p2p1/P1NP4/2Pp1bq1/3B2P1/4QP1P/4R1K1 b - - 2 34",
"4r1k1/4qp1p/3b2p1/2pP1BQ1/p1np4/B2P2P1/2P4P/5RK1 w - - 0 1",
"5rk1/1p3p1p/1qn1p1p1/1N2P3/2P2Q2/7P/1n1R2PK/4R3 b - - 3 34",
"4r3/1N1r2pk/7p/2p2q2/1n2p3/1QN1P1P1/1P3P1P/5RK1 w - - 0 1",
"5rk1/1p3p1p/1qn1p1p1/p1n1P3/2PN4/1P3Q1P/6PK/3RR3 w - - 7 31",
"3rr3/6pk/1p3q1p/2pn4/P1N1p3/1QN1P1P1/1P3P1P/5RK1 b - - 0 1",
"5rk1/1p1bpp1p/3p1np1/p1q1n3/P2NP3/1BP1QP2/2P3PP/2KR3R w - - 3 17",
"2kr3r/2p3pp/1bp1qp2/p2np3/P1Q1N3/3P1NP1/1P1BPP1P/5RK1 b - - 0 1",
"5rk1/1p1npp1p/5np1/p1q5/3NP3/1PP1Q2P/3K2P1/R6R w - - 1 22",
"r6r/3k2p1/1pp1q2p/3np3/P1Q5/5NP1/1P1NPP1P/5RK1 b - - 0 1",
"8/pp5k/3p3b/3P1n1p/PP2pB2/6P1/2N2K1P/8 b - - 0 33",
"8/2n2k1p/6p1/pp2Pb2/3p1N1P/3P3B/PP5K/8 w - - 0 1",
"rn2r1k1/pb1Rqpp1/1p5p/2P1p3/8/2Q1PN2/PP2BPPP/3R2K1 w - - 0 1",
"3r2k1/pp2bppp/2q1pn2/8/2p1P3/1P5P/PB1rQPP1/RN2R1K1 b - - 0 1 ",
"rnbqkbnr/pp2pppp/3p4/2p5/4P3/3B1N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
"rnbqk2r/pppp1ppp/3b1n2/4p3/2P5/3P4/PP2PPPP/RNBQKBNR b KQkq - 0 1",
"r1r1r1k1/3n1ppp/pq4n1/2Pp4/1P6/P2BpN1P/6PB/R5K1 w - b3 0 16",
"r5k1/6pb/p2bPn1p/1p6/2pP4/PQ4N1/3N1PPP/R1R1R1K1 b - b6 0 1",
"rn1q1rk1/4ppbp/5npP/2p2PP1/pp1P4/1QN1Pb2/PPPBB3/2KR1R2 w - - 0 1",
"2kr1r2/pppbb3/1qn1pB2/PP1p4/2P2pp1/5NPp/4PPBP/RN1Q1RK1 b - - 0 1",
"1kr1q1nr/pbpp4/Ppn5/1PP2p2/4P1pp/2bP1NQ1/3BBPPP/2R1R1K1 w - - 0 1",
"2r1r1k1/3bbppp/2Bp1nq1/4p1PP/1pp2P2/pPN5/PBPP4/1KR1Q1NR b - - 0 1"
};



void printboard()
{
     for (int r = 7; r >= 0; r--)
     {
         cout<<"\n";;
         for (int f = 0; f < 8; ++f)
         {
             if (p->board[fileranktosquare(f, r)].typ != ety)
             {
              cout<<setw(3)<<piecetochar[p->board[fileranktosquare(f, r)].typ];
             }
             else
             {
                 cout<<setw(3)<<".";
             }

         }
     }

     /**/
      cout<<"\n Castle Flags ";
      if(p->castleflags & 8) cout<<"K";
      if(p->castleflags & 4) cout<<"Q";
      if(p->castleflags & 2) cout<<"k";
      if(p->castleflags & 1) cout<<"q";
      cout<<"\n en_pas "<<returnsquare(p->en_pas);
      cout<<"\n hashkey "<<p->hashkey;
      cout<<"\n side = "<<colours[p->side];
      cout<<"\n majors = "<<p->majors;
      cout<<"\n ply = "<<p->ply;
      cout<<"\n wk = "<<returnsquare(p->k[white]);
      cout<<"\n bk = "<<returnsquare(p->k[black]);
      cout<<"\n white material = "<<p->material[white];
      cout<<" black material = "<<p->material[black];
      cout<<" (diff = "<<p->material[white]-p->material[black]<<")";
      cout<<"\n fifty = "<<p->fifty;
      cout<<"\n\n";
}


void debugatt()
{
     cout<<"\n white attackers...";
     for (int r = 7; r >= 0; r--)
     {
         cout<<"\n";;
         for (int f = 0; f < 8; ++f)
         {
             if (isattacked(fileranktosquare(f, r), white))
             {
              cout<<setw(3)<<"y";
             }
             else
             {
                 cout<<setw(3)<<".";
             }

         }
     }

     cout<<"\n\n black attackers...";
     for (int r = 7; r >= 0; r--)
     {
         cout<<"\n";;
         for (int f = 0; f < 8; ++f)
         {
             if (isattacked(fileranktosquare(f, r), black))
             {
              cout<<setw(3)<<"y";
             }
             else
             {
                 cout<<setw(3)<<".";
             }

         }
     }
}
     /**/
void printnums()
{
     cout<<endl;
     for (int r = 0; r < 144; r++)
     {
              if(r%12 == 0)
               cout<<endl;

       cout<<setw(4)<<p->board[r].typ;
     }

      cout<<"\n\n";

     for (int r = 0; r < 144; r++)
     {
              if(r%12 == 0)
               cout<<endl;

       cout<<setw(4)<<p->board[r].col;
     }

     for (int r = 0; r < 144; r++)
     {
              if(r%12 == 0)
               cout<<endl;

       cout<<setw(4)<<castlebits[r];
     }
      cout<<"\n\n";
}



void debugpiecelists()
{
      cout<<"\n\npiece lists..\n\n";

   /*  int sq;

     //old from the b&w lists, for the future

     for (sq = 0; sq < 144; ++sq)
     {
         if(p->board[sq].typ == edge) continue;
         cout<<setw(5);
         cout<<"sq "<<returnsquare(sq);
         cout<<setw(3);

         cout<<"  black piece num = "<<p->sqtopcenum[black][sq];
         cout<<setw(5);
         cout<<"  white piece num = "<<p->sqtopcenum[white][sq];
         cout<<endl;
     }

     for (sq = 0; sq <= p->pcenum[black]; ++sq)
     {
         cout<<setw(5);
        cout<<"piece "<<sq;
         cout<<setw(3);

         cout<<"  black, square = ";
         cout<<returnsquare(p->pcenumtosq[black][sq]);
         cout<<endl;
     }
     cout<<endl;
     for (sq = 0; sq <= p->pcenum[white]; ++sq)
     {
         cout<<setw(5);
         cout<<"piece "<<sq;
         cout<<setw(3);

         cout<<"  white, square = ";
         cout<<returnsquare(p->pcenumtosq[white][sq]);
         cout<<endl;
     }

     cout<<"\n\n";
     cout<<" black pieces = "<<p->pcenum[black];
     cout<<" white pieces = "<<p->pcenum[white];
     cout<<" \n\n ";*/

     cout<<"\n total pieces = "<<p->pcenum;
     for (int i = 0; i <= p->pcenum; ++i)
     {
         cout<<"\npiece "<<i<<" square "<<returnsquare(p->pcenumtosq[i]);
     }
     cout<<endl<<endl;


}



void debugeval()
{

     /*
     set i and j for the number of loops
     */
     char position[256];
     int i = 0, j = 2;
     int vals[j];
     memset(vals, 0 , sizeof(vals));

    for (i = 0; i < j; ++i)
    {
     if(j == 2)
     {
      test[i].copy(position, 512);
     }
     else
     mirrorevaltestpositions[i].copy(position, 512);

     cout<<"\n"<<position;
     /*
     set up the position
     */
     setboard(position);
     if (j < 4)
     {
      printboard();
     }
     cout<<"\nEVAL....";
     int evali = gameeval();
     cout<<endl<<endl<<evali<<endl;
     cout<<endl<<"-------------------------------------------------";
     vals[i] = evali;
    }
    for (i=0; i < j; ++i)
    {
        if (i == 0 ) {continue;}
        if(i==1)
        {
            if(vals[i] != vals[i-1])
            {cout<<"\nEval error, position "<<i;}
            continue;
        }
        if(i%2 == 1)
        {
             if(vals[i] != vals[i-1])
            {cout<<"\nEval error, position "<<mirrorevaltestpositions[i-1];}
            continue;
        }
    }

}

void debugevalsee()
{

     char position[256];int i;


     evalsee[0].copy(position, 512);
     cout<<"\n"<<position;
     p->ply = 0;
     setboard(position);
     printboard();
     capgen();
     for(i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	 {
        cout<<endl<<returnmove(p->list[i])<<" : ";
        /*if(see(p->list[i],p->side))
        cout<<"BAD SEE";
        else
        cout<<"GOOD SEE";*/
	 }

}

void debuginiteval()
{
    int i;
    cout<<"\nnum pawns ";
    cout<<" and pawn ranks : \n";
    for (i = 0; i < 10; ++i)
    {
		cout<<" wP file "<<i<<" num = "<<eval->pawns[white][i];
		cout<<" bP file "<<i<<" num = "<<eval->pawns[black][i];
		cout<<" white file "<<i<<" rank "<<eval->pawn_set[white][i];
		cout<<" black file "<<i<<" rank "<<eval->pawn_set[black][i];
		cout<<endl;
	}
	cout<<"\nwkingsq = "<<returnsquare(p->k[white]);
    cout<<" bkingsq = "<<returnsquare(p->k[black]);
    cout<<" wQc = "<<eval->wQc;
    cout<<" bQc = "<<eval->bQc;
    cout<<" wRc = "<<eval->wRc;
    cout<<" bRc = "<<eval->bRc;
    cout<<" wNc = "<<eval->wNc;
    cout<<" bNc = "<<eval->bNc;
    cout<<" wBc = "<<eval->wBc;
    cout<<" bBc = "<<eval->bBc;
    cout<<"\nwmajors = "<<eval->wmajors;
    cout<<" bmajors = "<<eval->bmajors;

}


void debugorder()
{
    char position[256];
    int i = 0, j = 0;
    int depth = 10;

    FILE *testfile;
    char *finddepth;
    char filename[64];
    char fileline[512];


    double start = 0, stop = 0;
    float fhfa[30], fha[30];
    float onodes = 0, nnodes = 0, qqnodes = 0;

    vector<double> mytime;

    /*cout<<"\nEnter filename..";
    cin>>filename;
    cout<<"\nEnter depth..";
    cin>>depth;*/

    testfile = fopen("positional_short.epd", "r");

    start = double(clock());

    while (fgets(fileline, 512, testfile))
    {
     setboard(fileline);
     //printboard();
     cout<<"\nposition "<<j;
     clearhash();
     p->ply = 0;
     initsearch();
     searchparam->inf = false;
     searchparam->depth = depth;
     searchparam->ucimode = 1;
     searchparam->starttime = double(clock());
     searchparam->stoptime = searchparam->starttime+12800000;
     calc();
     fhfa[i] = fhf;
     fha[i] = fh;
     clearhash();
     onodes += (qnodes+nodes);
     nnodes += nodes;
     qqnodes += qnodes;
     cout<<"\n bestmove : "<<returnmove(best);
     stop = double(clock());
     mytime.push_back(stop-searchparam->starttime);
     j++;
    }
    float total,us;
    float average = 0;

    for(i = 0; i < j; ++i)
    {
        cout<<"\nposition "<<i;
        cout<<",fhf,"<<fhfa[i]<<",fh,"<<fha[i]<<",";
        total = fha[i];us = fhfa[i];
        cout<<","<<((us/total)*100);
        cout<<",time,"<<(mytime[i]/1000)<<",";

    fprintf(log_file, "\n position %d time %f",i,(mytime[i]/1000));
    }
    //cout<<"\n\naverage = "<<(average/30);

    cout<<"\n total nodes,"<<onodes;
    cout<<",q nodes,"<<qqnodes;
    cout<<",normal nodes,"<<nnodes;
    cout<<",%qnodes,"<<(qqnodes/onodes)*100;
    cout<<",Total time,"<<((double(clock())-start)/1000);

    fprintf(log_file, "\n\n qnodes %f Total time %f",((qqnodes/onodes)*100),((double(clock())-start)/1000));

}


void debugprofile()
{
     setboard(m);
    // printboard();
     clearhash();
     p->ply = 0;
     initsearch();
     searchparam->inf = false;
     searchparam->depth = 32;
     searchparam->starttime = double(clock());
     cout<<"\nstart time is "<<searchparam->starttime;
     searchparam->stoptime = searchparam->starttime+60000;

     cout<<"\nstop time allocated is "<<searchparam->stoptime;
     calc();

     clearhash();


}


void debugnps()
{
    char position[256];
    int i;
    int anodes[30], aqnodes[30];


    for (i = 10; i < 20; ++i)
    {
     bt2630[i].copy(position, 512);
     cout<<"\n------------------Position "<<i+1<<" -------------------";
     cout<<"\n"<<position;

     setboard(position);
     //printboard();

     p->ply = 0;
     initsearch();
     searchparam->inf = false;
     searchparam->depth = 32;
     searchparam->starttime = double(clock());
     cout<<"\nstart time is "<<searchparam->starttime;
     searchparam->stoptime = searchparam->starttime+7000;

     cout<<"\nstop time allocated is "<<searchparam->stoptime;
     calc();
     anodes[i] = nodes;
     aqnodes[i] = qnodes;
     clearhash();
    }
    float total = 0,time = 70;

    for(i = 10; i < 20; ++i)
    {
        total += anodes[i]+aqnodes[i];
    }
    cout<<"\n\nnps = "<<(total/time)/1000;
    //cout<<"\nnodes = "<<total;
}






