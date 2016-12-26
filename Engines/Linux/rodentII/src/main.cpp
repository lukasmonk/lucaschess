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

#include "rodent.h"
#include "timer.h"
#include "book.h"
#include "param.h"

cBitBoard BB;
cMask Mask;
sTimer Timer; // class for setting and observing time limits
sBook  MainBook;  // opening book
sBook  GuideBook;
cParam Param;
cEval Eval;
POS p;

// Macros required for setting paths with compiler option -D
#define XSTR(x1) #x1
#define STR(x1) XSTR(x1)
#include <math.h>

int EloToSpeed(int elo) {

  if (elo >= 1800) {
    int mul = (elo - 1600) / 2;
	return (10 * mul);
  }

  if (elo >= 1000) {
    return 50 + (80 * (elo - 1000) / 100);
  }

  int result = 300 + pow(2, (elo - 799) / 85);
  result *= 0.23;

  if (elo < 1400) result *= 0.95;
  if (elo < 1300) result *= 0.95;
  if (elo < 1200) result *= 0.95;
  if (elo < 1100) result *= 0.95;
  if (elo < 1000) result *= 0.95;
  if (elo <  900) result *= 0.95;

  return result;
}

int EloToBlur(int elo) {
  if (elo < 2000) return (2000 - elo) / 5;
  return 0;
}

int main() {

  fl_reading_personality = 0;
  fl_separate_books = 0; // opening book files can be defined in a personality description
  fl_elo_slider = 0;
  time_percentage = 100;
  use_book = 1;
  panel_style = 0;
  verbose = 1;
  hist_limit = 24576;
  hist_perc = 175;

  Timer.Init();
  BB.Init();
  Mask.Init();
  Init();
  InitWeights();
  Param.Default();
  Param.DynamicInit();
  InitSearch();
#ifdef _WIN32 || _WIN64
  // if we are on Windows search for books and settings in same directory as rodentII.exe
  MainBook.bookName = "books/rodent.bin";
  GuideBook.bookName = "books/guide.bin";
  ReadPersonality("basic.ini");
#elif __linux || __unix
  // if we are on Linux
  // first check, if compiler got told where books and settings are stored
#ifdef BOOKPATH
  char path[255]; // space for complete path and filename
  char nameMainbook[20] = "/rodent.bin";
  char nameGuidebook[20]= "/guide.bin";
  char namePersonality[20]= "/basic.ini";
  // process Mainbook
  strcpy(path, ""); // first clear
  strcpy(path, STR(BOOKPATH)); // copy path from c preprocessor here
  strcat(path, nameMainbook); // append bookname
  MainBook.bookName = path; // store it
  // process Guidebook
  strcpy(path, "");
  strcpy(path, STR(BOOKPATH));
  strcat(path, nameGuidebook);
  GuideBook.bookName = nameGuidebook;
  // process Personality file
  strcpy(path, "");
  strcpy(path, STR(BOOKPATH));
  strcat(path, namePersonality);
  ReadPersonality(path);
#else // if no path was given than we assume that files are stored at /usr/share/rodentII
  MainBook.bookName = "/usr/share/rodentII/rodent.bin";
  GuideBook.bookName = "/usr/share/rodentII/guide.bin";
  ReadPersonality("/usr/share/rodentII/basic.ini");
#endif

#else
  // a platform we have not tested yet. We assume that opening books and 
  // settings are stored within the same directory. Similiar to Windows.
  printf("Platform unknown. We assume that opening books and settings are stored within RodentII path");
  MainBook.bookName = "books/rodent.bin";
  GuideBook.bookName = "books/guide.bin";
  ReadPersonality("basic.ini");
#endif
  MainBook.OpenPolyglot();
  GuideBook.OpenPolyglot();
  UciLoop();
  MainBook.ClosePolyglot();
  GuideBook.ClosePolyglot();
  return 0;
}
