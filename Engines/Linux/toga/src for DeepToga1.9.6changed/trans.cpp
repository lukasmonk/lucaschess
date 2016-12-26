
// includes

#include "hash.h"
#include "move.h"
#include "option.h"
#include "protocol.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// macros

#define MIN(a,b) ((a)<=(b)?(a):(b))
#define MAX(a,b) ((a)>=(b)?(a):(b))

#define ENTRY_DATE(entry)  ((entry)->date_flags>>4)
#define ENTRY_FLAGS(entry) ((entry)->date_flags&TransFlags)


// constants

static const bool UseModulo = false;

static const int DateSize = 16; // 4 bits

static const bool AlwaysWriteTrans = true; // true in beta5c, TogaII121 & fruit21 were hardwired always write.  

static const bool SmartMove    = true;
static const bool SmartReplace = true;
static const bool UseTransHits = true; // WHM stopgap for now.  


//static const int DepthNone = -128;

// types

/*struct entry_t {
   uint32 lock;
   uint16 move;
   sint8 depth;
   uint8 date;
   sint8 move_depth;
   uint8 flags;
   sint8 min_depth;
   sint8 max_depth;
   sint16 min_value;
   sint16 max_value;
};*/

/*struct entry_t {
   uint64 key;
   uint16 move;
   uint8 depth;
   uint8 date_flags;
   sint16 value;
   uint16 nproc; 
};*/

struct trans { // HACK: typedef'ed in trans.h
   entry_t * table;
   uint32 size;
   uint32 mask;
   int date;
   int age[DateSize];
   uint32 used;
   sint64 read_nb;
   sint64 read_hit;
   sint64 write_nb;
   sint64 write_hit;
   sint64 write_collision;
};

// variables

volatile trans_t Trans[1];
static int ClusterSize = 4; // WHM; at 4096Mb we use all 32 bits of address, no overflow of table...

// prototypes

static void      trans_set_date (volatile trans_t * trans, int date);
static int       trans_age      (const volatile trans_t * trans, int date);

static entry_t * trans_entry    (volatile trans_t * trans, uint64 key);

static bool      entry_is_ok    (const entry_t * entry);

// functions

// trans_is_ok()

bool trans_is_ok(const volatile trans_t * trans) {

   int date;

   if (trans == NULL) return false;

   if (trans->table == NULL) return false;
   if (trans->size == 0) return false;
   if (trans->mask == 0 || trans->mask >= trans->size) return false;
   if (trans->date >= DateSize) return false;

   for (date = 0; date < DateSize; date++) {
      if (trans->age[date] != trans_age(trans,date)) return false;
   }

   return true;
}

// trans_init()

void trans_init(volatile trans_t * trans) {

   ASSERT(trans!=NULL);

   ASSERT(sizeof(entry_t)==16);

   trans->size = 0;
   trans->mask = 0;
   trans->table = NULL;

   trans_set_date(trans,0);

   trans_clear(trans);

   // ASSERT(trans_is_ok(trans));
}

// trans_alloc()

void trans_alloc(volatile trans_t * trans) {

   uint64 size, target;

   ASSERT(trans!=NULL);

   // calculate size

   target = option_get_int("Hash");
   ASSERT(target >= 16); // option limit.  
   if (target < 16) target = 16;
   if      (4096 == target) ClusterSize = 1;
   else if (2048 == target) ClusterSize = 2;
   else                     ClusterSize = 4;
   ASSERT(target <= 4096); // 32-bit limit.  

   target *= 1024 * 1024;

   for (size = 1; size != 0 && size <= target; size *= 2)
      ;

   size /= 2;
   ASSERT(size>0&&size<=target);

   // allocate table

   size /= sizeof(entry_t);
   ASSERT(size!=0&&(size&(size-1))==0); // power of 2

   ASSERT(268435456  >= size + (ClusterSize - 1));
   trans->size = uint32(size + (ClusterSize - 1)); // HACK to avoid testing for end of table
   trans->mask = uint32(size - 1);

   trans->table = (entry_t *) my_malloc(trans->size*sizeof(entry_t));

   trans_clear(trans);

   ASSERT(trans_is_ok(trans));
}

// trans_free()

void trans_free(volatile trans_t * trans) {

   ASSERT(trans_is_ok(trans));

   my_free(trans->table);

   trans->table = NULL;
   trans->size = 0;
   trans->mask = 0;
}

// trans_clear()

void trans_clear(volatile trans_t * trans) {

   entry_t clear_entry[1];
   entry_t * entry;
   uint32 index;

   ASSERT(trans!=NULL);

   trans_set_date(trans,0);

   clear_entry->key = 0;
   clear_entry->move = MoveNone;
   clear_entry->depth = DepthNone;
   clear_entry->date_flags = (trans->date << 4);
   clear_entry->nhits = 0;
      
// ASSERT(entry_is_ok(clear_entry));

   entry = trans->table;

   for (index = 0; index < trans->size; index++) {
      *entry++ = *clear_entry;
   }
}

// trans_inc_date()

void trans_inc_date(volatile trans_t * trans) {

   ASSERT(trans!=NULL);

   trans_set_date(trans,(trans->date+1)%DateSize);
}

// trans_set_date()

static void trans_set_date(volatile trans_t * trans, int date) {

   ASSERT(trans!=NULL);
   ASSERT(date>=0&&date<DateSize);

   trans->date = date;

   for (date = 0; date < DateSize; date++) {
      trans->age[date] = trans_age(trans,date);
   }

   trans->used = 0;
   trans->read_nb = 0;
   trans->read_hit = 0;
   trans->write_nb = 0;
   trans->write_hit = 0;
   trans->write_collision = 0;
}

// trans_age()

static int trans_age(const volatile trans_t * trans, int date) {

   int age;

   ASSERT(trans!=NULL);
   ASSERT(date>=0&&date<DateSize);

   age = trans->date - date;
   if (age < 0) age += DateSize;

   ASSERT(age>=0&&age<DateSize);

   return age;
}

// trans_store()

#if DEBUG
void trans_store    (volatile trans_t * trans, uint64 key, int   move, int   depth, int   flags, int   value, board_t * board) {
#else
void trans_store    (volatile trans_t * trans, uint64 key, int   move, int   depth, int   flags, int   value) {
#endif

   entry_t * entry, * best_entry;
   int score, best_score;
   int i;

   ASSERT(trans_is_ok(trans));
   ASSERT(move>=0&&move<65536);
   ASSERT(depth>=0&&depth<256 || depth == DepthNone);
   ASSERT((flags&~TransFlags)==0);
   ASSERT((flags& TransFlags)!=0);
   ASSERT(value>=-32767&&value<=+32767);
   
   // init

   trans->write_nb++;

   // probe

   best_entry = NULL;
   best_score = -64 * 64 * 2048 * 4;

   entry = trans_entry(trans,key);

   for (i = 0; i < ClusterSize; i++, entry++) {

      if (entry->key == key) {

         // hash hit => update existing entry

         trans->write_hit++;
         if (ENTRY_DATE(entry) != trans->date) {
             trans->used++;
             if (UseTransHits) {
                entry->nhits = (entry->nhits >> 2); // was = 0; // first time with new trans date/time.  
             }
         } else if (UseTransHits) {
             entry->nhits++; // inc the number of entry hits
             ASSERT(entry->nhits <= 2048);
         }

         if (entry->depth <= depth) {

             if (SmartMove && move == MoveNone && entry->move != MoveNone) {
                 ASSERT(pseudo_is_legal(entry->move,board));
                 move = entry->move;
             }

            ASSERT(entry->key==key);
            entry->move = move;
            entry->depth = depth;
            entry->date_flags = (trans->date << 4) | flags;
            entry->value = value;

         } else { // deeper entry

             if (SmartMove && entry->move == MoveNone && move != MoveNone) {
                 ASSERT(pseudo_is_legal(move,board));
                 entry->move = move;
             }
            entry->date_flags = (trans->date << 4) | ENTRY_FLAGS(entry);
         }

         return;
      }


      // evaluate replacement score

      score = trans->age[ENTRY_DATE(entry)] * (64 * 64)  -  (entry->depth * entry->depth);
      if (UseTransHits) score -= entry->nhits; // WHM
      if (SmartReplace) {
         score = score * 4;
         if (TRANS_IS_EXACT(ENTRY_FLAGS(entry))) score -= 256; // WHM; depth 8 equiv.
         else                                    score -= ENTRY_FLAGS(entry); // back to Mr. Hack's formula.  
      }

      if (score > best_score) {
         best_entry = entry;
         best_score = score;
      }
   }

   // "best" entry found

   entry = best_entry;
   ASSERT(entry!=NULL);
// ASSERT(entry->key!=key);
   if (DEBUG && entry->key==key) printf("info - entry->key == key\n"); // DEBUG WHM

   if (ENTRY_DATE(entry) == trans->date) {

      trans->write_collision++;
      
      // (     const bool    &&    entry is deeper     &&  (input is not exact     OR     entry is exact                ))
      if (!AlwaysWriteTrans  &&  entry->depth > depth  &&  (!TRANS_IS_EXACT(flags) || TRANS_IS_EXACT(ENTRY_FLAGS(entry)))) {
         return; // do not replace deeper entries and do not replace NodePV entries.  
      }

   } else {

      trans->used++;
   }

   // store

   ASSERT(entry!=NULL);

   entry->key = key;
   entry->move = move;
   entry->depth = depth;
   entry->date_flags = (trans->date << 4) | flags;
   entry->value = value;
   if (UseTransHits) {
      entry->nhits = 0; // first time.
   }
}


// trans_retrieve()

bool trans_retrieve(volatile trans_t * trans, uint64 key, int * move, int * depth, int * flags, int * value) {

   int i;
   entry_t * entry;

   ASSERT(trans_is_ok(trans));
   ASSERT(move!=NULL);
   ASSERT(depth!=NULL);
   ASSERT(flags!=NULL);
   ASSERT(value!=NULL);

   // init

   trans->read_nb++;

   // probe

   entry = trans_entry(trans,key);

   for (i = 0; i < ClusterSize; i++, entry++) {

      if (entry->key == key) {

         // found

         trans->read_hit++;
         
         if (UseTransHits) {
            if (ENTRY_DATE(entry) != trans->date) {
               entry->nhits /= 2; // was = 0; // reset the number of entry hits
            } else {
               entry->nhits++; // inc the number of entry hits
               ASSERT(entry->nhits <= 2048);
            }
         }
         
         entry->date_flags = (trans->date << 4) | ENTRY_FLAGS(entry); // WHM put this back in, entry to current time.

         *move  = entry->move;
         *depth = entry->depth;
         *flags = ENTRY_FLAGS(entry);
         *value = entry->value;

         return true;
      }
   }

   // not found

   return false;
}

// trans_stats()

void trans_stats(const volatile trans_t * trans) {

   double full;
   // double hit, collision;

   ASSERT(trans_is_ok(trans));

   full = double(trans->used) / double(trans->size);
   // hit = double(trans->read_hit) / double(trans->read_nb);
   // collision = double(trans->write_collision) / double(trans->write_nb);

   send("info hashfull %.0f",full*1000.0);
}

// trans_entry()

static entry_t * trans_entry(volatile trans_t * trans, uint64 key) {

   uint32 index;

   ASSERT(trans_is_ok(trans));

   if (UseModulo) {
      index = key % (trans->mask + 1);
   } else {
      index = key & trans->mask;
   }

   ASSERT(index<=trans->mask);

   return &trans->table[index];
}

// entry_is_ok()

static bool entry_is_ok(const entry_t * entry) {

   if (entry == NULL) return false;

   if (ENTRY_DATE(entry) >= DateSize) return false;

   if (entry->move == MoveNone) return false;
   
   return true;
}

// end of trans.cpp
