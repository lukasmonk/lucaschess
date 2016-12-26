
// uci_option.cpp
// a very lazy adaption of option.cpp
// used for storing the ini file [Engine] section

// includes

#include <cstdlib>
#include <cstring>

#include "uci_options.h"
#include "util.h"

// constants

static const bool UseDebug = false;

// variables

static uci_option_t UCI_Options;

// prototypes

static uci_option_t * uci_option_find (const char var[]);

// functions

// uci_options_init()

void uci_options_init() {
UCI_Options.next=NULL;
UCI_Options.var=NULL;
UCI_Options.val=NULL;
}

// option_set()

bool uci_option_store(const char var[], const char val[]) {

   uci_option_t * opt;

   ASSERT(var!=NULL);
   ASSERT(val!=NULL);

   opt = uci_option_find(var);

   if (opt == NULL){ // not in the list
	   opt=&UCI_Options;
	   while(opt->var!=NULL){//walk down to end of list
		   if(opt->next!=NULL)opt=opt->next; else break;
	   }
	   if(opt->var==NULL) my_string_set(&opt->var,var);//case when opt==Option;
	   else{
		opt->next=(uci_option_t *)my_malloc(sizeof(struct uci_option_t ));
		//initialize
		opt=opt->next;
		opt->next=NULL;
		opt->val=NULL;
		opt->var=NULL;//!
		my_string_set(&opt->var,var);
	   }
	}

   my_string_set(&opt->val,val);

   if (UseDebug) my_log("POLYGLOT UCI_OPTION SET \"%s\" -> \"%s\"\n",opt->var,opt->val);

   return true;
}

// option_find()

static uci_option_t * uci_option_find(const char var[]) {

   uci_option_t * opt;

   ASSERT(var!=NULL);
	   opt = &UCI_Options;
	   while(true){
		if(opt->var==NULL) break;
		if (my_string_case_equal(opt->var,var)) return opt;
		opt=opt->next;
		if(opt==NULL) break;
	   }
   return NULL;
}

void init_uci_list(uci_option_t **next){
	*next=&UCI_Options;
}

// end of option.cpp

