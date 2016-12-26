//
// Cyrano Chess engine
//
// Copyright (C) 2007  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#include <string.h>
#include <vector>
#include "uci.hpp"
#include "util.hpp"


void uciOption::set_spin(const char *_val) {
    int iVal = atoi(_val);
    set_spin(iVal);
}
void uciOption::set_check(const char *_val) {
    if( !my_stricmp(_val, "true") )
        set_check( true );
    else
        set_check( false );
}

uciOption uciHash(SPIN, "Hash", 32, 8, 1024);
//uciOption uciPonder(CHECK, "Ponder", false);
uciOption uciOwnBook(CHECK, "OwnBook", true);
uciOption uciShowCurrLine(CHECK, "UCI_ShowCurrLine", false);
uciOption uciShowRefutations(CHECK, "UCI_ShowRefutations", false);
uciOption uciLimitStrength(CHECK, "UCI_LimitStrength", false);
uciOption uciElo(SPIN, "UCI_Elo", 2000, 2000, 2000);
uciOption uciAnalyseMode(CHECK, "UCI_AnalyseMode", false);
uciOption uciMultiPV(SPIN, "MultiPV", 1, 1, 1);
uciOption uciNalimovCache(SPIN, "NalimovCache", 0, 1, 32);
uciOption uciBookName(STRING, "BookName", "test.pbk");
uciOption uciEGBBDir(STRING, "egbb_dir", "");
// bb disabled because this does not work correctly
uciOption uciUseEGBB(CHECK, "Use Scorpio bitbases", false);

uciOptions cyranoOptions;

void uciOptions::init() {
    uciOptionList.push_back( &uciHash );
//    uciOptionList.push_back( &uciPonder );
    uciOptionList.push_back( &uciOwnBook );
    uciOptionList.push_back( &uciShowCurrLine );
    uciOptionList.push_back( &uciShowRefutations );
    uciOptionList.push_back( &uciLimitStrength );
    uciOptionList.push_back( &uciElo );
    uciOptionList.push_back( &uciAnalyseMode );
//    uciOptionList.push_back( &uciMultiPV );
//    uciOptionList.push_back( &uciNalimovCache );
    uciOptionList.push_back( & uciBookName );
    uciOptionList.push_back( & uciEGBBDir );
    uciOptionList.push_back( & uciUseEGBB );
}

void uciOptions::print() {
    for(unsigned int i = 0; i < this->uciOptionList.size() ; i++) {
        uciOption *thisOption = this->uciOptionList.at(i);
        printf("option name %s type", thisOption->get_name());
        switch( thisOption->get_type() ) {
            // option name Hash type spin default 1 min 1 max 128
            case SPIN :
                printf(" spin default %d min %d max %d\n", 
                    thisOption->get_spin(), thisOption->get_spin_min(), thisOption->get_spin_max());
                break;
            case CHECK : 
                printf(" check default %s\n", thisOption->get_check() ? "true" : "false");
                break;
            case STRING :
                printf(" string default %s\n", thisOption->get_string());
                break;
        }
    }
    fflush(stdout);
}

//option name NalimovPath type string default 
//option name NalimovCache type spin default 1 min 1 max 32

void uciOptions::set(const char *_name, const char *_value) {
//    printf("# setting option %s to %s\n", _name, _value);
//    fflush(stdout);
    for(unsigned int i = 0; i < this->uciOptionList.size() ; i++) {
        uciOption *thisOption = this->uciOptionList.at(i);
//        printf("# ---option '%s' '%s'\n", thisOption->get_name(), _name);
        if( !my_stricmp(thisOption->get_name(), _name) ) {
            printf("# changing option '%s'\n", _name);
            fflush(stdout);
            switch( thisOption->get_type() ) {
                case SPIN :
                    thisOption->set_spin( _value);
                    break;
                case CHECK :
                    thisOption->set_check( _value );
                    break;
                case STRING :
                    thisOption->set_string( _value );
                    break;
            }
            return;
        }
    }
    // the option was not found
}
