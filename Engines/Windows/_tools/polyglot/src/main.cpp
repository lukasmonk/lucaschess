
// main.cpp

// includes

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "adapter.h"
#include "attack.h"
#include "board.h"
#include "book.h"
#include "book_make.h"
#include "book_merge.h"
#include "engine.h"
#include "epd.h"
#include "fen.h"
#include "hash.h"
#include "list.h"
#include "main.h"
#include "move.h"
#include "move_gen.h"
#include "option.h"
#include "uci_options.h"
#include "piece.h"
#include "search.h"
#include "square.h"
#include "uci.h"
#include "util.h"
// constants

static const char * const Version = "1.4W27";

static const int SearchDepth = 63;
static const double SearchTime = 3600.0;
static const int StringSize = 4096;
// variables
static bool Init;

// prototypes

static void parse_option ();
static bool parse_line   (char line[], char * * name_ptr, char * * value_ptr);

static void stop_search  ();

#ifdef _WIN32
HANDLE Engine_ready_ok;
HANDLE Engine_sync_stop;
char *sCommand="";
DWORD WINAPI  ThreadFunc(LPVOID lpParam)
{
	if(WaitForSingleObject(Engine_ready_ok,INFINITE)!= WAIT_OBJECT_0){
		my_log("what happened?\n");//i have no idea!
		return 0;
	}
	//seed the random number generator!
	my_random_init();
	//got a "readyok" so we are done done initializing the engine
	my_log("Started xboard_step() thread \n");
	xboard_step();//runs in a loop
	return 0;//
}
#endif

int main(int argc, char * argv[]) {

	//   board_t board[1];

	// init

	Init = false;
#ifdef _WIN32
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
#ifdef SIGPIPE
	signal(SIGPIPE,SIG_IGN);
#endif
#endif

	util_init();
	printf("PolyGlot %s by Fabien Letouzey\n",Version);

	option_init();
	uci_options_init();
	square_init();
	piece_init();
	attack_init();

	hash_init();

	my_random_init();

	// build book

	if (argc >= 2 && my_string_equal(argv[1],"make-book")) {
		book_make(argc,argv);
		return EXIT_SUCCESS;
	}

	if (argc >= 2 && my_string_equal(argv[1],"merge-book")) {
		book_merge(argc,argv);
		return EXIT_SUCCESS;
	}

	// read options

	if (argc == 2) option_set("OptionFile",argv[1]); // HACK for compatibility

	parse_option(); // HACK: also launches the engine

	// EPD test

	if (argc >= 2 && my_string_equal(argv[1],"epd-test")) {
		epd_test(argc,argv);
		return EXIT_SUCCESS;
	}

	// opening book

	book_clear();
	if (option_get_bool("Book")){
		if (book_open(option_get_string("BookFile")))
			option_set("Book","false");//some error,set to false
	}
	//adapter_loop();
#ifdef _WIN32
	Engine_ready_ok=CreateEvent(NULL,FALSE,FALSE,NULL);
	Engine_sync_stop = CreateEvent(NULL,FALSE,FALSE,NULL);
	// all set and done.lets open a thread for fun
	DWORD dwThreadID;
	HANDLE hThread;
	hThread= CreateThread(
		NULL,	//default security attributes
		0,	//default stacksize
		ThreadFunc,	//The thread function!
		&sCommand,	//the commands for the thread function
		0,			//default creation flags
		&dwThreadID);	//the thread identifier
	Idle500msecs();//
	if(hThread==NULL){
		my_log("CreateThread failed\n");
	}

#endif
	adapter_loop();
	engine_send(Engine,"quit");
	engine_close(Engine);
#ifdef _WIN32
	CloseHandle(hThread); //close the thread;
#endif
	return EXIT_SUCCESS;
}
#if 0
// parse_option()

static void parse_option() {

	const char * file_name;
	FILE * file;
	char line[256];
	char * name, * value;

	file_name = option_get_string("OptionFile");

	file = fopen(file_name,"r");
	if (file == NULL) my_fatal("Can't open file \"%s\": %s\n",file_name,strerror(errno));

	// PolyGlot options (assumed first)

	while (true) {

		if (!my_file_read_line(file,line,256)) {
			my_fatal("parse_option(): missing [Engine] section\n");
		}

		if (my_string_case_equal(line,"[engine]")) break;

		if (parse_line(line,&name,&value)) option_set(name,value);
	}

	if (option_get_bool("Log")) {
		my_log_open(option_get_string("LogFile"));
	}

	my_log("POLYGLOT %s *** START ***\n",Version);
	my_log("POLYGLOT INI file \"%s\"\n",file_name);
	engine_open(Engine);

	Init = true; 
	uci_open(Uci,Engine);

	while (my_file_read_line(file,line,256)) {

		if (line[0] == '[') my_fatal("parse_option(): unknown section %s\n",line);

		if (parse_line(line,&name,&value)) {

			uci_send_option(Uci,name,"%s",value);
			//to get a decent display in winboard_x we need to now if an engine really is doing multipv analysis
			// "multipv 1" in the pv is meaningless,f.i. toga sends that all the time
			//therefore check if MultiPV is set to a decent value in the polyglot ini file
			if(my_string_case_equal(name,"MultiPV") && atoi(value)>1)  Uci->multipv_mode=true;
		}
	}

	uci_send_isready(Uci);

	fclose(file);

	if (my_string_equal(option_get_string("EngineName"),"<empty>")) {
		option_set("EngineName",Uci->name);
	}
}
#else
static void parse_option() {

	const char * file_name;
	FILE * file;
	char line[256];
	char * name, * value;

	file_name = option_get_string("OptionFile");

	file = fopen(file_name,"r");
	if (file == NULL)
		my_fatal("Can't open file \"%s\": %s\n",file_name,strerror(errno));

	// PolyGlot options (assumed first)

	//read the ini file,and store the name/value pairs
	while (true) {

		if (!my_file_read_line(file,line,sizeof(line))) {
			my_fatal("parse_option(): missing [Engine] section\n");
		}

		if (my_string_case_equal(line,"[engine]")) break;

		if (parse_line(line,&name,&value)) option_set(name,value);

	}
	if (option_get_bool("Log")) { my_log_open(option_get_string("LogFile"));}

	while (my_file_read_line(file,line,sizeof(line))) {

		if (line[0] == '[') my_fatal("parse_option(): unknown section %s\n",line);

		if (parse_line(line,&name,&value)) {
			uci_option_store(name,value);
		}
	}
	fclose(file);
	//read the optional global.ini file
	if(!option_get_bool("NoGlobals")){
		file=fopen("globals.ini","r");
		//override settings,if any
		if(file!=NULL){
			while (true) {
				if (!my_file_read_line(file,line,sizeof(line))) {
					my_fatal("parse_option(): missing [Engine] section\n");
				}

				if (my_string_case_equal(line,"[engine]")) break;

				if (parse_line(line,&name,&value)){
					if(!my_string_case_equal(name,"LogFile"))
						option_set(name,value);
				}
			}

			if (option_get_bool("Log")) {
				my_log_open(option_get_string("LogFile"));
			}
			else my_log_close(); //close it 

			while (my_file_read_line(file,line,sizeof(line))) {

				if (line[0] == '[') my_fatal("parse_option(): unknown section %s\n",line);

				if (parse_line(line,&name,&value)) {
					uci_option_store(name,value);
				}
			}
			fclose(file);
		}
	}
	my_log("POLYGLOT %s *** START ***\n",Version);
	my_log("POLYGLOT INI file \"%s\"\n",file_name);
	//do the dump:
	engine_open(Engine);
	Init = true; 
	uci_open(Uci,Engine);
	uci_option_t *next;
	init_uci_list(&next);
	while(next!=NULL){
		if(next->var==NULL) break;
		uci_send_option(Uci,next->var,"%s",next->val);
		if(my_string_case_equal(next->var,"MultiPV") && atoi(next->val)>1)  Uci->multipv_mode=true;
		next=next->next;
	}
	uci_send_isready(Uci);

	if (my_string_equal(option_get_string("EngineName"),"<empty>")) {
		option_set("EngineName",Uci->name);
	}
}
#endif

// parse_line()

static bool parse_line(char line[], char * * name_ptr, char * * value_ptr) {

	char * ptr;
	char * name, * value;

	ASSERT(line!=NULL);
	ASSERT(name_ptr!=NULL);
	ASSERT(value_ptr!=NULL);

	// remove comments

	ptr = strchr(line,';');
	//if (ptr != NULL) *ptr = '\0';

	//funny,but unfortunatly the ; is sometimes used as a seperator
	//often to seperate multiple paths for the egtbases

	//solution:
	// put "" around the paths in the ini file(s)
	// kludge code:
	// if  ; is encountered check if its between "s,(anywhere and any number(>0) will do)
	// if not,treat as comment as usual.

	// if it is,it is not removed,but the "" 
	// not that this implies that " " without any ; inbetween will be left untouched as it in the old days

	// eg Path = "C:\bases\m4" -> Path = "C:\bases\m4"
	// Path = C:\bases\m4;C:\bases\m5;C:\bases\m6  -> Path = C:\Bases\m5
	// Path = "C:\bases\m4;C:\bases\m5;C:\bases\m6" -> Path = C:\bases\m4;C:\bases\m5;C:\bases\m6

	// if outer " are needed for whatever reason: a pair of " right next to the inner ones are a special case: 
	// Path = ""C:\bases\m4;C:\bases\m5;C:\bases\m6"" -> Path = "C:\bases\m4;C:\bases\m5;C:\bases\m6"


	//The next chunk of code is not ugly,but better skip it anyway
	// [BeginChunk/]
	if(ptr!=NULL){
		char *n1,*n2;
		*ptr='\0';//split already and keeps strrchr working as the way we want
		n1=strchr(ptr+1,'"');
		n2=strrchr(line,'"');
		if(n1==NULL || n2==NULL){//old style comment
			/**ptr='\0'*/; 
		}else if(n1!= NULL && n2!=NULL){//  "...;....;...." seperator!
			*ptr=';'; //put it back!
			//if( n1[1]==n2[-1]=='"') {n1++;n2--;}  lousy c
			if(n1[1]=='"' && n2[-1]=='"') {n1++;n2--;}//special ""...."" case,remove the outer "
			*n1=' ';//remove 
			*n2=' ';//remove
		}
	}
	//[/EndChunk]

	ptr = strchr(line,'#');
	if (ptr != NULL) *ptr = '\0';

	// split at '='

	ptr = strchr(line,'=');
	if (ptr == NULL) return false;

	name = line;
	value = ptr+1;

	// cleanup name

	while (*name == ' ') name++; // remove leading spaces

	while (ptr > name && ptr[-1] == ' ') ptr--; // remove trailing spaces
	*ptr = '\0';

	if (*name == '\0') return false;

	// cleanup value

	ptr = &value[strlen(value)]; // pointer to string terminator

	while (*value == ' ') value++; // remove leading spaces

	while (ptr > value && ptr[-1] == ' ') ptr--; // remove trailing spaces
	*ptr = '\0';

	if (*value == '\0') return false;

	// end

	*name_ptr = name;
	*value_ptr = value;

	return true;
}

// quit()

void quit() {

	char string[StringSize];

	my_log("POLYGLOT *** QUIT ***\n");

	if (Init) {

		stop_search();
		engine_send(Engine,"quit");

		// wait for the engine to quit
#ifndef _WIN32 
		while (true) {
			engine_get(Engine,string,sizeof(string)); // HACK: calls exit() on receiving EOF
		}
#else     
		Idle500msecs();//
		while(peek_engine_get(Engine,string,StringSize))
			Idle();
#endif
		uci_close(Uci);
	}

	exit(EXIT_SUCCESS);
}

// stop_search()

static void stop_search() {

	if (Init && Uci->searching) {

		ASSERT(Uci->searching);
		ASSERT(Uci->pending_nb>=1);

		my_log("POLYGLOT STOP SEARCH\n");

		/*
		engine_send(Engine,"stop");
		Uci->searching = false;
		*/

		if (option_get_bool("SyncStop")) {
			uci_send_stop_sync(Uci);
		} else {
			uci_send_stop(Uci);
		}
	}
}

// end of main.cpp

