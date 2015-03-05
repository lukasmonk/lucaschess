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

#include "Tablebase.h"

Tablebase::Tablebase() {
    load();
}

Tablebase::~Tablebase() {
    tbcache_done();
    tb_done();
    paths = tbpaths_done ( paths );
}

void Tablebase::load() {
    memset ( installedPieces, 0, sizeof ( installedPieces ) );

    if ( !_file::fileExists ( path ) ) { return; }

    tbstats_reset();
    paths = tbpaths_done ( paths );
    paths = tbpaths_init();
    assert ( paths );
    paths = tbpaths_add ( paths, path.c_str() );
    restart();
    unsigned av = tb_availability();

    if ( 0 != ( av & 2 ) ) {
        setInstalledPieces ( 3 );
        cout << "3-pc TBs complete\n";
    }
    else if ( 0 != ( av & 1 ) ) { cout << "Some 3-pc TBs available\n"; }
    else { cout << "No 3-pc TBs available\n"; }

    if ( 0 != ( av & 8 ) ) {
        setInstalledPieces ( 4 );
        cout << "4-pc TBs complete\n";
    }
    else if ( 0 != ( av & 4 ) ) { cout << "Some 4-pc TBs available\n"; }
    else { cout << "No 4-pc TBs available\n"; }

    if ( 0 != ( av & 32 ) ) {
        setInstalledPieces ( 5 );
        cout << "5-pc TBs complete\n";
    }
    else if ( 0 != ( av & 16 ) ) { cout << "Some 5-pc TBs available\n"; }
    else { cout << "No 5-pc TBs available\n"; }

    cout << endl;

    if ( !getAvailable() ) { return; }

    setCacheSize ( cacheSize );
    tb_init ( verbosity, scheme, paths );
    tbcache_init ( cacheSize * 1024 * 1024, wdl_fraction );
    tbstats_reset();
}

int Tablebase::getCache() {
    return cacheSize;
}

string  Tablebase::getPath() {
    return path;
}

string  Tablebase::getSchema() {
    if ( scheme == tb_CP1 ) { return "cp1"; }

    if ( scheme == tb_CP2 ) { return "cp2"; }

    if ( scheme == tb_CP3 ) { return "cp3"; }

    if ( scheme == tb_CP4 ) { return "cp4"; }

    return "tb_UNCOMPRESSED";
}

bool Tablebase::getAvailable() {
    for ( int i = 3; i < 6; i++ )
        if ( installedPieces[i] ) { return true; }

    return false;
}

void Tablebase::print ( unsigned stm1, unsigned info1, unsigned pliestomate1 ) {
    if ( info1 == tb_DRAW ) {
        cout << "Draw";
    }
    else if ( info1 == tb_WMATE && stm1 == tb_WHITE_TO_MOVE ) {
        cout << "White mates, plies=" << pliestomate1;
    }
    else if ( info1 == tb_BMATE && stm1 == tb_BLACK_TO_MOVE ) {
        cout << "Black mates, plies=" << pliestomate1;
    }
    else if ( info1 == tb_WMATE && stm1 == tb_BLACK_TO_MOVE ) {
        cout << "Black is mated, plies=" << pliestomate1;
    }
    else if ( info1 == tb_BMATE && stm1 == tb_WHITE_TO_MOVE ) {
        cout << "White is mated, plies=" << pliestomate1;
    }
    else {
        cout << "none";
    }
}

bool Tablebase::setCacheSize ( int mb ) {
    if ( mb < 1 || mb > 1024 ) { return false; }

    cacheSize = mb ;
    tbcache_init ( cacheSize * 1024 * 1024, wdl_fraction );
    restart();
    return true;
}

bool Tablebase::setScheme ( string s ) {
    bool res = false;

    if ( s == "cp1" ) { scheme = tb_CP1; res = true; }
    else if ( s == "cp2" ) { scheme = tb_CP2; res = true; }
    else if ( s == "cp3" ) { scheme = tb_CP3; res = true;  }
    else if ( s == "cp4" ) { scheme = tb_CP4; res = true;  }

    if ( res ) { restart(); }

    return res;
}

void Tablebase::restart() {
    tb_restart ( verbosity, scheme, paths );
    tbcache_restart ( cacheSize * 1024 * 1024, wdl_fraction );
}

bool Tablebase::setProbeDepth ( int d ) {
    if ( d < 0 || d > 5 ) { return false; }

    probeDepth = d;
    return true;
}

bool Tablebase::setInstalledPieces ( int n ) {
    if ( n < 3 || n > 5 ) { return false; }

    installedPieces[n] = true;
    return true;
}

void Tablebase::setPath ( string path1 ) {
    path = path1;
    load();
}


