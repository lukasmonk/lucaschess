
// parse.h

#ifndef PARSE_H
#define PARSE_H

// includes

#include "util.h"

// constants

const int STAR_NUMBER = 16;
const int KEYWORD_NUMBER = 256;

// types

struct parse_t {
   const char * string;
   int pos;
   int keyword_nb;
   const char * keyword[KEYWORD_NUMBER];
};

// variables

extern char * Star[STAR_NUMBER];

// functions

extern bool match             (char string[], const char pattern[]);

extern void parse_open        (parse_t * parse, const char string[]);
extern void parse_close       (parse_t * parse);

extern void parse_add_keyword (parse_t * parse, const char keyword[]);

extern bool parse_get_word    (parse_t * parse, char string[], int size);
extern bool parse_get_string  (parse_t * parse, char string[], int size);

#endif // !defined PARSE_H

// end of parse.h

