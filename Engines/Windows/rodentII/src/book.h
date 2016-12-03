/*
  Rodent, a UCI chess playing engine derived from Sungorus 1.4
  Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
  Copyright (C) 2011-2016 Pawel Koziol

  Rodent is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  Rodent is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

struct sBookEntry {
    U64 hash;
    int move;
    int freq;
};

struct polyglot_move {
    U64  key;
    int  move;
    int weight;
    int  n;
    int  learn;
};

#include<stdio.h>

struct sBook {
private:
    int bookSize;
    FILE * bookFile;
    int moves[100];
    int nOfChoices;
    char testString [12];
    int IsInfrequent(int val, int maxFreq);
    void ParseBookEntry(char * ptr, int line_no);
    int FindPos(U64 key);
    void ReadEntry(polyglot_move * entry, int n);
    U64 ReadInteger(int size);
public:
    char *bookName;
    int GetPolyglotMove(POS *p, int printOutput);
    U64 GetPolyglotKey(POS *p);
    void OpenPolyglot(void);
    void ClosePolyglot(void);
    void Init(POS *p);
};

extern sBook GuideBook;
extern sBook MainBook;