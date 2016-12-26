
// parse.cpp

// includes

#include <cstring>

#include "parse.h"
#include "util.h"

// constants

static const int StringSize = 256;

// variables

char * Star[STAR_NUMBER];

// prototypes

static bool match_rec (char string[], const char pattern[], char * star[]);

// functions

// match()

bool match(char string[], const char pattern[]) {

   ASSERT(string!=NULL);
   ASSERT(pattern!=NULL);

   ASSERT(strstr(pattern,"**")==NULL);

   return match_rec(string,pattern,Star);
}

// match_rec()

static bool match_rec(char string[], const char pattern[], char * star[]) {

   int c;

   ASSERT(string!=NULL);
   ASSERT(pattern!=NULL);
   ASSERT(star!=NULL);

   // iterative matches

   while ((c=*pattern++) != '*') {
      if (false) {
      } else if (c == '\0') { // end of pattern
         while (*string == ' ') string++; // skip trailing spaces
         return *string == '\0';
      } else if (c == ' ') { // spaces
         if (*string++ != ' ') return false; // mismatch
         while (*string == ' ') string++; // skip trailing spaces
      } else { // normal character
         if (*string++ != c) return false; // mismatch
      }
   }

   // recursive wildcard match

   ASSERT(c=='*');

   while (*string == ' ') string++; // skip leading spaces
   *star++ = string; // remember beginning of star

   while ((c=*string++) != '\0') { // reject empty-string match
      if (c != ' ' && match_rec(string,pattern,star)) { // shortest match
         ASSERT(string>star[-1]);
         *string = '\0'; // truncate star
         return true;
      }
   }

   return false;
}

// parse_is_ok()

bool parse_is_ok(const parse_t * parse) {

   if (parse == NULL) return false;
   if (parse->string == NULL) return false;
   if (parse->pos < 0 || (size_t) parse->pos > strlen(parse->string)) return false;
   if (parse->keyword_nb < 0 || parse->keyword_nb >= KEYWORD_NUMBER) return false;

   return true;
}

// parse_open()

void parse_open(parse_t * parse, const char string[]) {

   ASSERT(parse!=NULL);
   ASSERT(string!=NULL);

   parse->string = string;
   parse->pos = 0;
   parse->keyword_nb = 0;
}

// parse_close()

void parse_close(parse_t * parse) {

   int i;

   ASSERT(parse_is_ok(parse));

   parse->string = NULL;
   parse->pos = 0;

   for (i = 0; i < parse->keyword_nb; i++) {
      my_string_clear(&parse->keyword[i]);
   }

   parse->keyword_nb = 0;
}

// parse_add_keyword()

void parse_add_keyword(parse_t * parse, const char keyword[]) {

   const char * * string;

   ASSERT(parse_is_ok(parse));
   ASSERT(keyword!=NULL);

   if (parse->keyword_nb < KEYWORD_NUMBER) {

      string = &parse->keyword[parse->keyword_nb];
      parse->keyword_nb++;

      *string = NULL;
      my_string_set(string,keyword);
   }
}

// parse_get_word()

bool parse_get_word(parse_t * parse, char string[], int size) {

   int pos;
   int c;

   ASSERT(parse!=NULL);
   ASSERT(string!=NULL);
   ASSERT(size>=256);

   // skip blanks

   for (; parse->string[parse->pos] == ' '; parse->pos++)
      ;

   ASSERT(parse->string[parse->pos]!=' ');

   // copy word

   pos = 0;

   while (true) {

      c = parse->string[parse->pos];
      if (c == ' ' || pos >= size-1) c = '\0';

      string[pos] = c;
      if (c == '\0') break;

      parse->pos++;
      pos++;
   }

   ASSERT(strchr(string,' ')==NULL);

   return pos > 0; // non-empty word?
}

// parse_get_string()

bool parse_get_string(parse_t * parse, char string[], int size) {

   int pos;
   parse_t parse_2[1];
   char word[StringSize];
   int i;
   int c;

   ASSERT(parse!=NULL);
   ASSERT(string!=NULL);
   ASSERT(size>=256);

   // skip blanks

   for (; parse->string[parse->pos] == ' '; parse->pos++)
      ;

   ASSERT(parse->string[parse->pos]!=' ');

   // copy string

   pos = 0;

   while (true) {

      parse_open(parse_2,&parse->string[parse->pos]);

      if (!parse_get_word(parse_2,word,sizeof(word))) {
         string[pos] = '\0';
         parse_close(parse_2);
         goto finished;
      }

      for (i = 0; i < parse->keyword_nb; i++) {
         if (my_string_equal(parse->keyword[i],word)) {
            string[pos] = '\0';
            parse_close(parse_2);
            goto finished;
         }
      }

      parse_close(parse_2);

      // copy spaces

      while (true) {

         c = parse->string[parse->pos];
         if (c != ' ') break;

         if (pos >= size-1) c = '\0';

         string[pos] = c;
         if (c == '\0') break;

         parse->pos++;
         pos++;
      }

      // copy non spaces

      while (true) {

         c = parse->string[parse->pos];
         if (c == ' ' || pos >= size-1) c = '\0';

         string[pos] = c;
         if (c == '\0') break;

         parse->pos++;
         pos++;
      }

      string[pos] = '\0';
   }

finished: ;

   return pos > 0; // non-empty string?
}

// end of parse.cpp

