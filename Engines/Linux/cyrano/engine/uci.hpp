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

#ifndef _UCI_HPP_
#define _UCI_HPP_

#include <vector>

typedef enum UCI_TYPES {SPIN, STRING, CHECK, COMBO, BUTTON } UCI_TYPE;

class uciOption {
private:
    char    name[32];
    UCI_TYPE type;
    bool    check_value;
    bool    check_default;
    char    string_value[256];
    char    string_default[64];
    int     spin_value;
    int     spin_default;
    int     spin_min;
    int     spin_max;
    bool    changed;
public:
    UCI_TYPE get_type() const {
        return type;
    }
    bool    has_changed() {
        bool _has_changed = changed;
        changed = false;
        return _has_changed;
    }
    int get_spin() const {
        return spin_value;
    }
    int get_spin_min() const {
        return spin_min;
    }
    int get_spin_max() const {
        return spin_max;
    }
    void set_spin(int _val) {
        if(_val >= spin_min && _val <= spin_max)
            spin_value = _val;
        changed = true;
    }
    void set_spin(const char *_val);
    bool get_check() const {
        return check_value;
    }
    void set_check(const char *_val);
    void set_check(bool _val) {
        check_value = _val;
        changed = true;
    }
    const char *get_name() const {
        return name;
    }
    void set_name(const char *_name) {
        strncpy(name, _name, 32);
    }
    const char *get_string() const {
        return string_value;
    }
    void set_string(const char *_val) {
        strncpy(string_value, _val, 256);
        changed = true;
    }
    void set_string_default(const char *_val) {
        strncpy(string_default, _val, 64);
    }
    uciOption(UCI_TYPE _type, const char *_name) {};
    uciOption(UCI_TYPE _type, const char *_name, int _spin_default, int _spin_min, int _spin_max) :
        type(_type),
        spin_value(_spin_default),
        spin_default(_spin_default),
        spin_min(_spin_min),
        spin_max(_spin_max) {
            set_name( _name );
    };
    uciOption(UCI_TYPE _type, const char *_name, bool _check_default) :
        type(_type),
        check_value(_check_default),
        check_default(_check_default) {
            set_name( _name );
    };
    uciOption(UCI_TYPE _type, const char *_name, const char * _string_default) :
        type(_type) {
            set_name( _name );
            set_string(_string_default);
            set_string_default(_string_default);
    };
};

class uciOptions {
private:
    std::vector <uciOption *> uciOptionList;
public:
    uciOptions() {};
    void init();
    void print();
    void set(const char *_name, const char *_value);
};

extern uciOptions cyranoOptions;
extern uciOption uciHash;
extern uciOption uciOwnBook;
extern uciOption uciBookName;
extern uciOption uciEGBBDir;
extern uciOption uciUseEGBB;
extern uciOption uciShowCurrLine;

#endif // _UCI_HPP_
