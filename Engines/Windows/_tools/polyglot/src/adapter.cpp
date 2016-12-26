
// adapter.cpp

// includes

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <sys/select.h>
#include <sys/types.h> // Mac OS X needs this one
#include <unistd.h>
#endif

#include "adapter.h"
#include "board.h"
#include "book.h"
#include "colour.h"
#include "engine.h"
#include "fen.h"
#include "game.h"
#include "io.h"
#include "line.h"
#include "main.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "option.h"
#include "parse.h"
#include "san.h"
#include "uci.h"
#include "util.h"

// constants

static const bool UseDebug = false;
static const bool DelayPong = false;

static const int StringSize = 4096;

// types

struct xboard_t {
   io_t io[1];
};

struct state_t {
   int state;
   bool computer[ColourNb];
   int exp_move;
   int resign_nb;
   my_timer_t timer[1];
};

struct xb_t {

   bool analyse;
   bool computer;
   const char * name;
   bool ics;
   bool new_hack; // "new" is a C++ keyword
   bool ponder;
   int ping;
   bool post;
   int proto_ver;
   bool result;

   int mps;
   double base;
   double inc;

   bool time_limit;
   double time_max;

   bool depth_limit;
   int depth_max;

   double my_time;
   double opp_time;
};

enum dummy_state_t { WAIT, THINK, PONDER, ANALYSE };

// variables

static xboard_t XBoard[1];

static state_t State[1];
static xb_t XB[1];

// prototypes


//static bool xboard_step    ();
static void engine_step    ();

static void comp_move      (int move);
static void move_step      (int move);
static void board_update   ();

static void mess           ();
static void no_mess        (int move);

static void search_update  ();
static void search_clear   ();

static bool active         ();
static bool ponder         ();
static bool ponder_ok      (int ponder_move);

static void stop_search    ();

static void send_board     (int extra_move);
static void send_pv        ();
static void tb_to_string   (char *tbstring,int size);

static void xboard_send    (xboard_t * xboard, const char format[], ...);

static void learn          (int result);

#ifndef _WIN32 
static void adapter_step      (); 
static void xboard_get        (xboard_t * xboard, char string[], int size); 
#endif

// functions

// adapter_loop()

void adapter_loop() {

   // init
   game_clear(Game);

   // state

   State->state = WAIT;

   State->computer[White] = false;
   State->computer[Black] = true;

   State->exp_move = MoveNone;
   State->resign_nb = 0;
   my_timer_reset(State->timer);

#ifndef _WIN32
   // xboard

   XBoard->io->in_fd = STDIN_FILENO;
   XBoard->io->out_fd = STDOUT_FILENO;
   XBoard->io->name = "XBOARD";

   io_init(XBoard->io);
#endif
   XB->analyse = false;
   XB->computer = false;
   XB->name = NULL;
   my_string_set(&XB->name,"<empty>");
   XB->ics = false;
   XB->new_hack = true;
   XB->ping = -1;
   XB->ponder = false;
   XB->post = false;
   XB->proto_ver = 1;
   XB->result = false;

   XB->mps = 0;
   XB->base = 300.0;
   XB->inc = 0.0;

   XB->time_limit = false;
   XB->time_max = 5.0;

   XB->depth_limit = false;
   XB->depth_max = 127;

   XB->my_time = 300.0;
   XB->opp_time = 300.0;
#ifdef _WIN32
   // loop
   while(true) 
	   engine_step();
#else
   while (true) adapter_step();
#endif
}

#ifndef _WIN32
// adapter_step()

static void adapter_step() {

   fd_set set[1];
   int fd_max;
   int val;

   // process buffered lines

   while (io_line_ready(XBoard->io)) xboard_step(); // process available xboard lines
   while (io_line_ready(Engine->io)) engine_step(); // process available engine lines

   // init

   FD_ZERO(set);
   fd_max = -1; // HACK

   // add xboard input

   ASSERT(XBoard->io->in_fd>=0);

   FD_SET(XBoard->io->in_fd,set);
   if (XBoard->io->in_fd > fd_max) fd_max = XBoard->io->in_fd;

   // add engine input

   ASSERT(Engine->io->in_fd>=0);

   FD_SET(Engine->io->in_fd,set);
   if (Engine->io->in_fd > fd_max) fd_max = Engine->io->in_fd;

   // wait for something to read (no timeout)

   ASSERT(fd_max>=0);

   val = select(fd_max+1,set,NULL,NULL,NULL);
   if (val == -1 && errno != EINTR) my_fatal("adapter_step(): select(): %s\n",strerror(errno));

   if (val > 0) {
      if (FD_ISSET(XBoard->io->in_fd,set)) io_get_update(XBoard->io); // read some xboard input
      if (FD_ISSET(Engine->io->in_fd,set)) io_get_update(Engine->io); // read some engine input
   }
}
#endif

// xboard_step()

void xboard_step() {

	char string[StringSize];
	int move;
#ifdef _WIN32	
int n;
#endif
	char move_string[256];
	board_t board[1];
#ifndef _WIN32
xboard_get(XBoard,string,sizeof(string));
#else
	 while(fgets(string,StringSize-1,stdin)!=NULL){//stops if error or eof
		 //remove \n 
		 n=0;
		 while(string[n]!= '\0' && n < StringSize){
			 if(string[n]=='\n'){ string[n]='\0'; break;}
			 n++;
		 }
		my_log("Xboard->Adapter: %s\n",string);
#endif
		if (false) {

		} else if (match(string,"accepted *")) {

			// ignore

		} else if (match(string,"analyze")) {

			State->computer[White] = false;
			State->computer[Black] = false;

			XB->analyse = true;
			XB->new_hack = false;
			ASSERT(!XB->result);
			XB->result = false;

			mess();

		} else if (match(string,"bk")) {

			if (option_get_bool("Book")) {
				game_get_board(Game,board);
				book_disp(board);
			}

		} else if (match(string,"black")) {

			if (colour_is_black(game_turn(Game))) {

				State->computer[White] = true;
				State->computer[Black] = false;

				XB->new_hack = true;
				XB->result = false;

				mess();
			}

		} else if (match(string,"computer")) {

			XB->computer = true;

		} else if (match(string,"draw")) {
			if(uci_option_exist(Uci,"UCI_DrawOffers")){
			    my_log("POLYGLOT draw from XB received");
				uci_send_option(Uci,"DrawOffer","%s","draw");}
		} else if (match(string,"easy")) {

			XB->ponder = false;

			mess();

		} else if (match(string,"edit")) {

			// refuse

			xboard_send(XBoard,"Error (unknown command): %s",string);

		} else if (match(string,"exit")) {

			State->computer[White] = false;
			State->computer[Black] = false;

			XB->analyse = false;

			mess();

		} else if (match(string,"force")) {

			State->computer[White] = false;
			State->computer[Black] = false;

			mess();

		} else if (match(string,"go")) {

			State->computer[game_turn(Game)] = true;
			State->computer[colour_opp(game_turn(Game))] = false;

			XB->new_hack = false;
			ASSERT(!XB->result);
			XB->result = false;

			mess();

		} else if (match(string,"hard")) {

			XB->ponder = true;

			mess();

		} else if (match(string,"hint")) {

			if (option_get_bool("Book")) {

				game_get_board(Game,board);
				move = book_move(board,false);

				if (move != MoveNone && move_is_legal(move,board)) {
					move_to_san(move,board,move_string,sizeof(move_string));
					xboard_send(XBoard,"Hint: %s",move_string);
				}
			}

		} else if (match(string,"ics *")) {

			XB->ics = true;

		} else if (match(string,"level * *:* *")) {

			XB->mps  = atoi(Star[0]);
			XB->base = double(atoi(Star[1])) * 60.0 + double(atoi(Star[2]));
			XB->inc  = double(atoi(Star[3]));

		} else if (match(string,"level * * *")) {

			XB->mps  = atoi(Star[0]);
			XB->base = double(atoi(Star[1])) * 60.0;
			XB->inc  = double(atoi(Star[2]));

		} else if (match(string,"name *")) {

			my_string_set(&XB->name,Star[0]);

		} else if (match(string,"new")) {

			my_log("POLYGLOT NEW GAME\n");

			option_set("Chess960","false");

			game_clear(Game);

			if (XB->analyse) {
				State->computer[White] = false;
				State->computer[Black] = false;
			} else {
				State->computer[White] = false;
				State->computer[Black] = true;
			}

			XB->new_hack = true;
			XB->result = false;

			XB->depth_limit = false;

			XB->computer = false;
			my_string_set(&XB->name,"<empty>");

			board_update();
			mess();

			uci_send_ucinewgame(Uci);
			uci_send_isready_sync(Uci);

		} else if (match(string,"nopost")) {

			XB->post = false;

		} else if (match(string,"otim *")) {

			XB->opp_time = double(atoi(Star[0])) / 100.0;
			if (XB->opp_time < 0.0) XB->opp_time = 0.0;

		} else if (match(string,"pause")) {

			// refuse

			xboard_send(XBoard,"Error (unknown command): %s",string);

		} else if (match(string,"ping *")) {

			// HACK; TODO: answer only after an engine move

			if (DelayPong) {
				if (XB->ping >= 0) xboard_send(XBoard,"pong %d",XB->ping); // HACK: get rid of old ping
				XB->ping = atoi(Star[0]);
				uci_send_isready(Uci);
			} else {
				ASSERT(XB->ping==-1);
				xboard_send(XBoard,"pong %s",Star[0]);
			}

		} else if (match(string,"playother")) {

			State->computer[game_turn(Game)] = false;
			State->computer[colour_opp(game_turn(Game))] = true;

			XB->new_hack = false;
			ASSERT(!XB->result);
			XB->result = false;

			mess();

		} else if (match(string,"post")) {

			XB->post = true;

		} else if (match(string,"protover *")) {

			XB->proto_ver = atoi(Star[0]);
			ASSERT(XB->proto_ver>=2);

			xboard_send(XBoard,"feature done=0");

			xboard_send(XBoard,"feature analyze=1");
			xboard_send(XBoard,"feature colors=0");
			xboard_send(XBoard,"feature draw=1");
			xboard_send(XBoard,"feature ics=1");
			xboard_send(XBoard,"feature myname=\"%s\"",option_get_string("EngineName"));
			xboard_send(XBoard,"feature name=1");
			xboard_send(XBoard,"feature pause=0");
			xboard_send(XBoard,"feature ping=1");
			xboard_send(XBoard,"feature playother=1");
			xboard_send(XBoard,"feature reuse=1");
			xboard_send(XBoard,"feature san=0");
			xboard_send(XBoard,"feature setboard=1");
			xboard_send(XBoard,"feature sigint=0");
			xboard_send(XBoard,"feature sigterm=0");
			xboard_send(XBoard,"feature time=1");
			xboard_send(XBoard,"feature usermove=1");
			xboard_send(XBoard,"feature debug=1");

			if (uci_option_exist(Uci,"UCI_Chess960")) {
				xboard_send(XBoard,"feature variants=\"normal,fischerandom\"");
			} else {
				xboard_send(XBoard,"feature variants=\"normal\"");
			}

			if (Uci->ready) xboard_send(XBoard,"feature done=1");

			// otherwise "feature done=1" will be sent when the engine is ready

		} else if (match(string,"quit")) {
			my_log("POLYGLOT *** \"quit\" from XBoard ***\n");
			quit();
		} else if (match(string,"random")) {

			// ignore

		} else if (match(string,"rating * *")) {

			// ignore

		} else if (match(string,"remove")) {

			if (game_pos(Game) >= 2) {

				game_goto(Game,game_pos(Game)-2);

				ASSERT(!XB->new_hack);
				XB->new_hack = false; // HACK?
				XB->result = false;

				board_update();
				mess();
			}

		} else if (match(string,"rejected *")) {

			// ignore

		} else if (match(string,"reset")) { // protover 3?

			// refuse

			xboard_send(XBoard,"Error (unknown command): %s",string);

		} else if (false
			|| match(string,"result * {*}")
			|| match(string,"result * {* }")
			|| match(string,"result * { *}")
			|| match(string,"result * { * }")) {

				my_log("POLYGLOT GAME END\n");

				XB->result = true;

				mess();

				// book learning

				if (option_get_bool("Book") && option_get_bool("BookLearn")) {

					if (false) {
					} else if (my_string_equal(Star[0],"1-0")) {
						learn(+1);
					} else if (my_string_equal(Star[0],"0-1")) {
						learn(-1);
					} else if (my_string_equal(Star[0],"1/2-1/2")) {
						learn(0);
					}
				}
		} else if (match(string,"resume")) {

			// refuse

			xboard_send(XBoard,"Error (unknown command): %s",string);

		} else if (match(string,"sd *")) {

			XB->depth_limit = true;
			XB->depth_max = atoi(Star[0]);

		} else if (match(string,"setboard *")) {

			my_log("POLYGLOT FEN %s\n",Star[0]);

			if (!game_init(Game,Star[0])) my_fatal("xboard_step(): bad FEN \"%s\"\n",Star[0]);

			State->computer[White] = false;
			State->computer[Black] = false;

			XB->new_hack = true; // HACK?
			XB->result = false;

			board_update();
			mess();

		} else if (match(string,"st *")) {

			XB->time_limit = true;
			XB->time_max = double(atoi(Star[0]));

		} else if (match(string,"time *")) {

			XB->my_time = double(atoi(Star[0])) / 100.0;
			if (XB->my_time < 0.0) XB->my_time = 0.0;

		} else if (match(string,"undo")) {

			if (game_pos(Game) >= 1) {

				game_goto(Game,game_pos(Game)-1);

				ASSERT(!XB->new_hack);
				XB->new_hack = false; // HACK?
				XB->result = false;

				board_update();
				mess();
			}

		} else if (match(string,"usermove *")) {
			game_get_board(Game,board);
			move = move_from_san(Star[0],board);

			if (move != MoveNone && move_is_legal(move,board)) {

				XB->new_hack = false;
				ASSERT(!XB->result);
				XB->result = false;

				move_step(move);
				board_update();
				no_mess(move);

			} else {

				xboard_send(XBoard,"Illegal move: %s",Star[0]);
			}
		} else if (match(string,"variant *")) {

			if (my_string_equal(Star[0],"fischerandom")) {
				option_set("Chess960","true");
			} else {
				option_set("Chess960","false");
			}

		} else if (match(string,"white")) {

			if (colour_is_white(game_turn(Game))) {

				State->computer[White] = false;
				State->computer[Black] = true;

				XB->new_hack = true;
				XB->result = false;

				mess();
			}

		} else if (match(string,"xboard")) {

			// ignore

		} else if (match(string,".")) { // analyse info

			if (State->state == ANALYSE) {
				int depth=Uci->best_depth;//HACK: don't clear engine-output window...

				ASSERT(Uci->searching);
				ASSERT(Uci->pending_nb>=1);

				if (Uci->root_move != MoveNone && move_is_legal(Uci->root_move,Uci->board)) {
					move_to_san(Uci->root_move,Uci->board,move_string,sizeof(move_string));
					xboard_send(XBoard,"stat01: %.0f " S64_FORMAT " %d %d %d %s",Uci->time*100.0,Uci->node_nb,/*Uci->*/depth,Uci->root_move_nb-(Uci->root_move_pos+1),Uci->root_move_nb,move_string);
				} else {
					xboard_send(XBoard,"stat01: %.0f " S64_FORMAT " %d %d %d",Uci->time*100.0,Uci->node_nb,/*Uci->*/depth,0,0); // HACK
				}
			}

		} else if (match(string,"?")) { // move now

			if (State->state == THINK) {

				ASSERT(Uci->searching);
				ASSERT(Uci->pending_nb>=1);

				// HACK: just send "stop" to the engine

				if (Uci->searching) {
					my_log("POLYGLOT STOP SEARCH\n");
					engine_send(Engine,"stop");
				}
			}

		} else { // unknown command, maybe a move?

			game_get_board(Game,board);
			move = move_from_san(string,board);

			if (move != MoveNone && move_is_legal(move,board)) {

				XB->new_hack = false;
				ASSERT(!XB->result);
				XB->result = false;

				move_step(move);
				board_update();
				no_mess(move);

			} else if (move != MoveNone) {

				xboard_send(XBoard,"Illegal move: %s",string);

			} else {

				xboard_send(XBoard,"Error (unknown command): %s",string);
			}
		}
#ifdef _WIN32
		}
#endif
}

// engine_step()

static void engine_step() {

	char string[StringSize];
	int event;

	// parse UCI line

	    engine_get(Engine,string,StringSize); //blocking read...
		event = uci_parse(Uci,string);
		// react to events

		if ((event & EVENT_READY) != 0) {
#ifdef _WIN32
			SetEvent(Engine_ready_ok);
#endif
			if (!Uci->ready) {
				Uci->ready = true;
				if (XB->proto_ver >= 2) xboard_send(XBoard,"feature done=1");
			}

			if (!DelayPong && XB->ping >= 0) {
				xboard_send(XBoard,"pong %d",XB->ping);
				XB->ping = -1;
			}
		}
		if ((event & EVENT_MOVE) != 0 && State->state == THINK) {
			// the engine is playing a move

			// MEGA HACK: estimate remaining time because XBoard won't send it!

			my_timer_stop(State->timer);

			XB->my_time -= my_timer_elapsed_real(State->timer);
			XB->my_time += XB->inc;
			if (XB->mps != 0 && (game_move_nb(Game) + 1) % XB->mps == 0) XB->my_time += XB->base;

			if (XB->my_time < 0.0) XB->my_time = 0.0;

			// play the engine move

			comp_move(Uci->best_move);
		}

		if ((event & EVENT_PV) != 0) {

			// the engine has sent a new PV

			send_pv();
		}
		if ((event & EVENT_INFO) != 0){
			if(!option_get_bool("InfoStrings"))
			xboard_send(XBoard,"#%d %+d %.0f " S64_FORMAT " %s ",Uci->best_depth,Uci->best_score,Uci->time*100.0,Uci->node_nb,Uci->info_string);
		else
			xboard_send(XBoard,"%d %+d %.0f " S64_FORMAT " %s ",Uci->best_depth,Uci->best_score,Uci->time*100.0,Uci->node_nb,Uci->info_string);
		}
		if((event & (EVENT_DRAW|EVENT_RESIGN))!=0){
			my_log("POYGLOT draw offer/resign from engine\n");
			if(uci_option_exist(Uci,"UCI_DrawOffers")){
				if(event & EVENT_DRAW)
					xboard_send(XBoard,"offer draw");
				else
					xboard_send(XBoard,"resign");
			}
		}
	return ;
}

// comp_move()

static void comp_move(int move) {

	board_t board[1];
	char string[256];

	ASSERT(move_is_ok(move));

	ASSERT(State->state==THINK);
	ASSERT(!XB->analyse);

	if(option_get_bool("RepeatPV")==true)
		send_pv(); // to update time and nodes

	// send the move

	game_get_board(Game,board);

	if (move_is_castle(move,board) && option_get_bool("Chess960")) {
		if (!move_to_san(move,board,string,sizeof(string))) my_fatal("comp_move(): move_to_san() failed\n"); // O-O/O-O-O
	} else {
		if (!move_to_can(move,board,string,sizeof(string))) my_fatal("comp_move(): move_to_can() failed\n");
	}

	move_step(move);
	//game ended?
	if(game_status(Game)!= PLAYING){
		//handle ics drawing stuff
		if(XB->ics){
			switch (game_status(Game)){
			case DRAW_MATERIAL:
			case DRAW_FIFTY:
			case DRAW_REPETITION:
				xboard_send(XBoard,"offer draw");
				break;
			default:
				break;
			}
		}
		xboard_send(XBoard,"move %s",string);
		board_update();
		no_mess(move);
		return;
	}

	// engine sended a move while in ponder mode? 
	if(State->state==PONDER){
		if(board->turn==White)
			xboard_send(XBoard,"0-1 {polyglot : engine moves while pondering}\n");
		else
			xboard_send(XBoard,"1-0 {polyglot : engine moves while pondering}\n");
	}
	// resign?
	if (option_get_bool("Resign") && Uci->root_move_nb > 1) {
		int best = Uci->best_score;
		if (option_get_bool("ScoreWhite") && colour_is_black(Uci->board->turn))
			best = -best;

		if (best <= -abs(option_get_int("ResignScore"))) {

			State->resign_nb++;
			my_log("POLYGLOT %d move%s with resign score\n",State->resign_nb,(State->resign_nb>1)?"s":"");

			if (State->resign_nb >= option_get_int("ResignMoves")) {
				my_log("POLYGLOT *** RESIGN ***\n");
				//send move and resign
				//xboard_send(XBoard,"move %s \nresign",string);
				//just resign
				xboard_send(XBoard,"resign",string);
				no_mess(move);
				return;
			}
		} else {
			if (State->resign_nb > 0) my_log("POLYGLOT resign reset (State->resign_nb=%d)\n",State->resign_nb);
			State->resign_nb = 0;
		}
	}
	no_mess(move);
	xboard_send(XBoard,"move %s",string);
}

// move_step()
void engine_move_fail(char *move_string){
  board_t board[1];
   game_get_board(Game,board);
   if(XB->ics) return; //
	  if(board->turn==White)
		  xboard_send(XBoard,"0-1 {polyglot: %s engine move format-error white}\n",move_string);
	  else
		  xboard_send(XBoard,"1-0 {polyglot: %s engine move format-error black}\n",move_string);
	   my_fatal("parse_bestmove(): not a move \"%s\"\n",move_string);
}

static void move_step(int move) {

   board_t board[1];
   char move_string[256];

   ASSERT(move_is_ok(move));

   // log

   game_get_board(Game,board);

   if (XB->ics || (move != MoveNone && move_is_legal(move,board))) {

      move_to_san(move,board,move_string,sizeof(move_string));
      my_log("POLYGLOT MOVE %s\n",move_string);

   } else {
      move_to_can(move,board,move_string,sizeof(move_string));
      my_log("POLYGLOT ILLEGAL MOVE \"%s\"\n",move_string);
      board_disp(board);
	  //since we have threads my_fatal is not enough,1 thread will wait for xboard to end the game
	  //stuff illegal move in the comment as well,not everybody logs all the time.
	  if(board->turn==White)
		  xboard_send(XBoard,"0-1 {polyglot: %s illegal engine move white}\n",move_string);
	  else
		  xboard_send(XBoard,"1-0 {polyglot: %s illegal engine move black}\n",move_string);
      my_fatal("move_step(): illegal move \"%s\"\n",move_string);
   }

   // play the move

   game_add_move(Game,move);
   //board_update();
}

// board_update()

static void board_update() {

   // handle game end

   ASSERT(!XB->result);

   switch (game_status(Game)) {
   case PLAYING:
      break;
   case WHITE_MATES:
      xboard_send(XBoard,"1-0 {White mates}");
      break;
   case BLACK_MATES:
      xboard_send(XBoard,"0-1 {Black mates}");
      break;
   case STALEMATE:
      xboard_send(XBoard,"1/2-1/2 {Stalemate}");
      break;
   case DRAW_MATERIAL:
      xboard_send(XBoard,"1/2-1/2 {Draw by insufficient material}");
      break;
   case DRAW_FIFTY:
      xboard_send(XBoard,"1/2-1/2 {Draw by fifty-move rule}");
      break;
   case DRAW_REPETITION:
      xboard_send(XBoard,"1/2-1/2 {Draw by repetition}");
      break;
   default:
      ASSERT(false);
      break;
   }
}

// mess()

static void mess() {

   // clear state variables

   State->resign_nb = 0;
   State->exp_move = MoveNone;
   my_timer_reset(State->timer);

   // abort a possible search

   stop_search();

   // calculate the new state

   if (false) {
   } else if (!active()) {
      State->state = WAIT;
      my_log("POLYGLOT WAIT\n");
   } else if (XB->analyse) {
      State->state = ANALYSE;
      my_log("POLYGLOT ANALYSE\n");
   } else if (State->computer[game_turn(Game)]) {
      State->state = THINK;
      my_log("POLYGLOT THINK\n");
   } else {
      State->state = WAIT;
      my_log("POLYGLOT WAIT\n");
   }
   search_update();
}

// no_mess()

static void no_mess(int move) {

   ASSERT(move_is_ok(move));

   // just received a move, calculate the new state

   if (false) {

   } else if (!active()) {

      stop_search(); // abort a possible search

      State->state = WAIT;
      State->exp_move = MoveNone;

      my_log("POLYGLOT WAIT\n");

   } else if (State->state == WAIT) {

      ASSERT(State->computer[game_turn(Game)]);
      ASSERT(!State->computer[colour_opp(game_turn(Game))]);
      ASSERT(!XB->analyse);

      my_log("POLYGLOT WAIT -> THINK\n");

      State->state = THINK;
      State->exp_move = MoveNone;

   } else if (State->state == THINK) {

      ASSERT(!State->computer[game_turn(Game)]);
      ASSERT(State->computer[colour_opp(game_turn(Game))]);
      ASSERT(!XB->analyse);

      if (ponder() && ponder_ok(Uci->ponder_move)) {

         my_log("POLYGLOT THINK -> PONDER\n");

         State->state = PONDER;
         State->exp_move = Uci->ponder_move;

      } else {

         my_log("POLYGLOT THINK -> WAIT\n");

         State->state = WAIT;
         State->exp_move = MoveNone;
      }

   } else if (State->state == PONDER) {

      ASSERT(State->computer[game_turn(Game)]);
      ASSERT(!State->computer[colour_opp(game_turn(Game))]);
      ASSERT(!XB->analyse);

      if (move == State->exp_move && Uci->searching) {

         ASSERT(Uci->searching);
         ASSERT(Uci->pending_nb>=1);

         my_timer_start(State->timer);//also resets

         my_log("POLYGLOT PONDER -> THINK (*** HIT ***)\n");
         engine_send(Engine,"ponderhit");

         State->state = THINK;
         State->exp_move = MoveNone;

         send_pv(); // update display

         return; // do not launch a new search

      } else {

         my_log("POLYGLOT PONDER -> THINK (miss)\n");

         stop_search();

         State->state = THINK;
         State->exp_move = MoveNone;
      }

   } else if (State->state == ANALYSE) {

      ASSERT(XB->analyse);

      my_log("POLYGLOT ANALYSE -> ANALYSE\n");

      stop_search();

   } else {

      ASSERT(false);
   }

   search_update();
}

// search_update()

static void search_update() {

   int move;
   int move_nb;
   board_t board[1];

   ASSERT(!Uci->searching);

   // launch a new search if needed

   if (State->state == THINK || State->state == PONDER || State->state == ANALYSE) {


      // opening book

      if (State->state == THINK && option_get_bool("Book")) {

         game_get_board(Game,Uci->board);

         move = book_move(Uci->board,option_get_bool("BookRandom"));

         if (move != MoveNone && move_is_legal(move,Uci->board)) {

            my_log("POLYGLOT *BOOK MOVE*\n");

            search_clear(); // clears Uci->ponder_move
            Uci->best_move = move;

            board_copy(board,Uci->board);
            move_do(board,move);
            Uci->ponder_move = book_move(board,false); // expected move = best book move

            Uci->best_pv[0] = Uci->best_move;
            Uci->best_pv[1] = Uci->ponder_move; // can be MoveNone
            Uci->best_pv[2] = MoveNone;

            comp_move(Uci->best_move);

            return;
         }
      }

      // engine search

      my_log("POLYGLOT START SEARCH\n");

	   //moved search_clear() and UCI->searching=true and uci->pending_nb++ up,
	   //because an answer to "go" might get in  before we knew what happened.
      search_clear();
      Uci->searching = true;
      Uci->pending_nb++;
      // options

      uci_send_option(Uci,"UCI_Chess960","%s",option_get_bool("Chess960")?"true":"false");

      if (option_get_int("UCIVersion") >= 2) {
         uci_send_option(Uci,"UCI_Opponent","none none %s %s",(XB->computer)?"computer":"human",XB->name);
         uci_send_option(Uci,"UCI_AnalyseMode","%s",(XB->analyse)?"true":"false");
      }

      uci_send_option(Uci,"Ponder","%s",ponder()?"true":"false");

      // position

      move = (State->state == PONDER) ? State->exp_move : MoveNone;
      send_board(move); // updates Uci->board global variable
      // search

	  if (State->state == THINK || State->state == PONDER) {

         engine_send_queue(Engine,"go");

         if (XB->time_limit) {

            // fixed time per move

            engine_send_queue(Engine," movetime %.0f",XB->time_max*1000.0);

         } else {

            // time controls

            if (colour_is_white(Uci->board->turn)) {
               engine_send_queue(Engine," wtime %.0f btime %.0f",XB->my_time*1000.0,XB->opp_time*1000.0);
            } else {
               engine_send_queue(Engine," wtime %.0f btime %.0f",XB->opp_time*1000.0,XB->my_time*1000.0);
            }

            if (XB->inc != 0.0)
				engine_send_queue(Engine," winc %.0f binc %.0f",XB->inc*1000.0,XB->inc*1000.0);

            if (XB->mps != 0) {

               move_nb = XB->mps - (Uci->board->move_nb % XB->mps);
               ASSERT(move_nb>=1&&move_nb<=XB->mps);
               engine_send_queue(Engine," movestogo %d",move_nb);
            }
         }

         if (XB->depth_limit) engine_send_queue(Engine," depth %d",XB->depth_max);

         if (State->state == PONDER) engine_send_queue(Engine," ponder");

         engine_send(Engine,""); // newline

      } else if (State->state == ANALYSE) {

         engine_send(Engine,"go infinite");

      } else {

         ASSERT(false);
      }

      // init search info

      ASSERT(!Uci->searching);

 //     search_clear();
 //     Uci->searching = true;
 //     Uci->pending_nb++;
   }
}

// search_clear()

static void search_clear() {

   uci_clear(Uci);

   // TODO: MOVE ME

   my_timer_start(State->timer);//also resets
}

// active()

static bool active() {

   // position state

   if (game_status(Game) != PLAYING) return false; // game ended

   // xboard state

   if (XB->analyse) return true; // analysing
   if (!State->computer[White] && !State->computer[Black]) return false; // force mode
   if (XB->new_hack || XB->result) return false; // unstarted or ended game

   return true; // playing
}

// ponder()

static bool ponder() {

   return XB->ponder && (option_get_bool("CanPonder") || uci_option_exist(Uci,"Ponder"));
}
// ponder_ok()

static bool ponder_ok(int move) {
   int status;
   board_t board[1];

   ASSERT(move==MoveNone||move_is_ok(move));

   // legal ponder move?

   if (move == MoveNone) return false;

   game_get_board(Game,board);
   if (!move_is_legal(move,board)) return false;

   // UCI-legal resulting position?

   game_add_move(Game,move);

   game_get_board(Game,board);
   status = game_status(Game);

   game_rem_move(Game);

   if (status != PLAYING) return false; // game ended

   if (option_get_bool("Book") && is_in_book(board)) {
      return false;
   }

   return true;
}

// stop_search()

static void stop_search() {

   if (Uci->searching) {

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

// send_board()

static void send_board(int extra_move) {

   char fen[256];
   int start, end;
   board_t board[1];
   int pos;
   int move;
   char string[256];

   ASSERT(extra_move==MoveNone||move_is_ok(extra_move));

   ASSERT(!Uci->searching);

   // init

   game_get_board(Game,Uci->board);
   if (extra_move != MoveNone) move_do(Uci->board,extra_move);

   board_to_fen(Uci->board,fen,sizeof(fen));
   my_log("POLYGLOT FEN %s\n",fen);

   ASSERT(board_can_play(Uci->board));

   // more init

   start = 0;
   end = game_pos(Game);
   ASSERT(end>=start);

   // position

   game_get_board(Game,board,start);
   board_to_fen(board,string,sizeof(string));

   engine_send_queue(Engine,"position");

   if (my_string_equal(string,StartFen)) {
      engine_send_queue(Engine," startpos");
   } else {
      engine_send_queue(Engine," fen %s",string);
   }

   // move list

   if (end > start || extra_move != MoveNone) engine_send_queue(Engine," moves");

   for (pos = start; pos < end; pos++) { // game moves

      move = game_move(Game,pos);

      move_to_can(move,board,string,sizeof(string));
      engine_send_queue(Engine," %s",string);

      move_do(board,move);
   }

   if (extra_move != MoveNone) { // move to ponder on
      move_to_can(extra_move,board,string,sizeof(string));
      engine_send_queue(Engine," %s",string);
   }

   // end

   engine_send(Engine,""); // newline
}

// send_pv()

static void send_pv() {

   char pv_string[StringSize];
   board_t board[1];
   int move;
   char move_string[256];
   char tb_string[256];
   ASSERT(State->state!=WAIT);


   if (Uci->best_depth == 0) return;
   //
   move_string[0]='\0';
   // xboard search information
   if (XB->post && Uci->time >= option_get_double("PostDelay")) {
      if (State->state == THINK || State->state == ANALYSE) {
         line_to_san(Uci->best_pv,Uci->board,pv_string,sizeof(pv_string));
		 tb_to_string(tb_string,sizeof(tb_string));
		 if(Uci->depth==-1) //hack to clear the engine output window
         xboard_send(XBoard,"%d %+d %.0f " S64_FORMAT " ",0,Uci->best_score,Uci->time*100.0,Uci->node_nb);
		xboard_send(XBoard,"%d %+d %.0f " S64_FORMAT " %s %s",Uci->best_depth,Uci->best_score,Uci->time*100.0,Uci->node_nb,pv_string,tb_string);
      } else if (State->state == PONDER && option_get_bool("ShowPonder")) {

         game_get_board(Game,board);
         move = State->exp_move;

         if (move != MoveNone && move_is_legal(move,board)) {
            move_to_san(move,board,move_string,sizeof(move_string));
            line_to_san(Uci->best_pv,Uci->board,pv_string,sizeof(pv_string));
			tb_to_string(tb_string,sizeof(tb_string));
            xboard_send(XBoard,"%d %+d %.0f " S64_FORMAT " (%s) %s %s",Uci->best_depth,Uci->best_score,Uci->time*100.0,Uci->node_nb,move_string,pv_string,tb_string);
         }
      }
   }

   // kibitz

   if ((Uci->searching && option_get_bool("KibitzPV") && Uci->time >= option_get_double("KibitzDelay"))
    || (!Uci->searching && option_get_bool("KibitzMove"))) {
      if (State->state == THINK || State->state == ANALYSE) {
         line_to_san(Uci->best_pv,Uci->board,pv_string,sizeof(pv_string));
         xboard_send(XBoard,"%s depth=%d time=%.2f node=" S64_FORMAT " speed=%.0f score=%+.2f pv=\"%s\"",option_get_string("KibitzCommand"),Uci->best_depth,Uci->time,Uci->node_nb,Uci->speed,double(Uci->best_score)/100.0,pv_string);
      } else if (State->state == PONDER) {

         game_get_board(Game,board);
         move = State->exp_move;

         if (move != MoveNone && move_is_legal(move,board)) {
            move_to_san(move,board,move_string,sizeof(move_string));
            line_to_san(Uci->best_pv,Uci->board,pv_string,sizeof(pv_string));
            xboard_send(XBoard,"%s depth=%d time=%.2f node=" S64_FORMAT " speed=%.0f score=%+.2f pv=\"(%s) %s\"",option_get_string("KibitzCommand"),Uci->best_depth,Uci->time,Uci->node_nb,Uci->speed,double(Uci->best_score)/100.0,move_string,pv_string);
         }
      }
   }
}

static void tb_to_string(char *tbstring,int size)
{
   int n=0;
   tbstring[0]='\0';
   if(Uci->hash>0.0)
   n=sprintf(tbstring,"%3.0f%% ",Uci->hash*100);
   if(Uci->tbhits) sprintf(tbstring+n,"tb: "S64_FORMAT,Uci->tbhits);
}

#ifndef _WIN32
// xboard_get()

static void xboard_get(xboard_t * xboard, char string[], int size) {

   ASSERT(xboard!=NULL);
   ASSERT(string!=NULL);
   ASSERT(size>=256);

   if (!io_get_line(xboard->io,string,size)) { // EOF
      my_log("POLYGLOT *** EOF from XBoard ***\n");
      quit();
   }
}
#endif
// xboard_send()

static void xboard_send(xboard_t * xboard, const char format[], ...) {

   va_list arg_list;
   char string[StringSize];

   ASSERT(xboard!=NULL);
   ASSERT(format!=NULL);

   // format

   va_start(arg_list,format);
   vsprintf(string,format,arg_list);
   va_end(arg_list);
#ifndef _WIN32
   // send

   io_send(xboard->io,"%s",string);
#else
   puts(string);
   fflush(stdout);
   my_log("Adapter->Xboard: %s\n",string);
#endif
}

// learn()

static void learn(int result) {

   int pos;
   board_t board[1];
   int move;

   ASSERT(result>=-1&&result<=+1);

   ASSERT(XB->result);
   ASSERT(State->computer[White]||State->computer[Black]);

   // init

   pos = 0;

   if (false) {
   } else if (State->computer[White]) {
      pos = 0;
   } else if (State->computer[Black]) {
      pos = 1;
      result = -result;
   } else {
      my_fatal("learn(): unknown side\n");
   }

   if (false) {
   } else if (result > 0) {
      my_log("POLYGLOT *LEARN WIN*\n");
   } else if (result < 0) {
      my_log("POLYGLOT *LEARN LOSS*\n");
   } else {
      my_log("POLYGLOT *LEARN DRAW*\n");
   }

   // loop

   for (; pos < Game->size; pos += 2) {

      game_get_board(Game,board,pos);
      move = game_move(Game,pos);

      book_learn_move(board,move,result);
   }

   book_flush();
}

// end of adapter.cpp
