
// uci_options.h

#ifndef UCI_OPTIONS_H
#define UCI_OPTIONS_H

#include "util.h"

struct uci_option_t {
   const char * var;
   const char * val;
   struct uci_option_t * next;
};



// functions

extern void         uci_options_init       ();
extern bool         uci_option_store       (const char var[], const char val[]);
extern void         init_uci_list (uci_option_t **next);
#endif // !defined OPTION_H

// end of option.h

