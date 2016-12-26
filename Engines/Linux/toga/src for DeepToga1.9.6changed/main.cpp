
// main.cpp

// includes

#include <cstdio>
#include <cstdlib>

#include "attack.h"
#include "book.h"
#include "hash.h"

#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "random.h"
#include "search.h" // WHM, for NumberThreads printf()
#include "sort.h"
#include "square.h"
#include "trans.h"
#include "util.h"
#include "value.h"
#include "vector.h"
#include "probe.h"

// functions

// main()

int main(int argc, char * argv[]) {

   // init

   util_init();
   my_random_init(); // for opening book

   option_init();

   NumberThreads = option_get_int("Number of Threads");
   get_NumberProcessors(); // WHM and ThinkingALot
   NumberThreadsInternal = MIN(NumberThreads, NumberProcessors); // WHM

   printf("DeepTogaNPS LC change to add NPS control.Michele Tumbarello specifications\n");
   printf("Original messages from DeepToga:\n");
   printf("DeepToga UCI by WHMoweryJr, Thomas Gaksch and Fabien Letouzey.\n");
   printf("Based on Toga II 1.4beta5c by Thomas Gaksch.\n");
   printf("Based on Fruit 2.1 by Fabien Letouzey.\n");
   printf("Number of cpu's                 = %d.  \n", NumberProcessors);
   printf("Number of Threads actually used = %d.  \n", NumberThreadsInternal);
   printf("Settings by Dieter Eberle and Chris Formula {Toga II 1.4.1SE}.\n");
   printf("Compilations and testing by Denis Mendoza.\n");
   printf("Compilations and testing by Jim Ablett.\n");
   printf("Toga Developers Discussion Board and testing by Shaun Brewer.\n");
   printf("Book expert: Kevin Frayer.\n");
   printf("MP help: The mysterious ThinkingALot {Vadim Dem}.\n");
   printf("Systems Engineering: WHMoweryJr. \n");
   printf("Systems Engineering: Jerry Donald. \n");
   printf("Systems Engineering: Chris Formula. \n");


   // early initialisation (the rest is done after UCI options are parsed in protocol.cpp)

   square_init();
   piece_init();
   sort_init(); // WHM; for the code stuff in sort and first history init.
   pawn_init_bit();
   value_init();
   vector_init();
   attack_init();
   move_do_init();

   random_init();
   hash_init();

   trans_init(Trans);
   book_init();

   // this is after option_init() so lets use options for conformity.
   // all probe variables here are 'extern's in module probe.
   egbb_cache_size = (option_get_int("Bitbases Cache Size") * 1024 * 1024);
   egbb_cache_size /= NumberThreadsInternal; // WHM_egbb
   egbb_path = (char *) option_get_string("Bitbases Path");
   egbb_is_loaded = false;
   if (option_get_bool("Use Bitbases")) {
      if (1 == LoadEgbbLibrary(egbb_path)) egbb_is_loaded = true;
   }
   if (egbb_is_loaded) printf("EgbbProbe IS Loaded!\n");
   else                printf("EgbbProbe NOT Loaded!\n");

   // loop

   loop();

   return EXIT_SUCCESS;
}

// end of main.cpp

