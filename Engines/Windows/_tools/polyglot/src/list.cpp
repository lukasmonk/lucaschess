
// list.cpp

// includes

#include "board.h"
#include "list.h"
#include "move.h"
#include "util.h"

// functions

// list_is_ok()

bool list_is_ok(const list_t * list) {

   if (list == NULL) return false;

   if (list->size >= ListSize) return false;

   return true;
}

// list_clear()

void list_clear(list_t * list) {

   ASSERT(list!=NULL);

   list->size = 0;
}

// list_add()

void list_add(list_t * list, int move, int value) {

   ASSERT(list_is_ok(list));
   ASSERT(move_is_ok(move));
   ASSERT(value>=-32767&&value<=+32767);

   ASSERT(list->size<ListSize-1);

   list->move[list->size] = move;
   list->value[list->size] = value;
   list->size++;
}

// list_remove()

void list_remove(list_t * list, int index) {

   int i;

   ASSERT(list_is_ok(list));
   ASSERT(index>=0&&index<list->size);

   for (i = index; i < list->size-1; i++) {
      list->move[i] = list->move[i+1];
      list->value[i] = list->value[i+1];
   }

   list->size--;
}

// list_is_empty()

bool list_is_empty(const list_t * list) {

   ASSERT(list_is_ok(list));

   return list->size == 0;
}

// list_size()

int list_size(const list_t * list) {

   ASSERT(list_is_ok(list));

   return list->size;
}

// list_move()

int list_move(const list_t * list, int index) {

   ASSERT(list_is_ok(list));
   ASSERT(index>=0&&index<list->size);

   return list->move[index];
}

// list_value()

int list_value(const list_t * list, int index) {

   ASSERT(list_is_ok(list));
   ASSERT(index>=0&&index<list->size);

   return list->value[index];
}

// list_copy()

void list_copy(list_t * dst, const list_t * src) {

   int i;

   ASSERT(dst!=NULL);
   ASSERT(list_is_ok(src));

   dst->size = src->size;

   for (i = 0; i < src->size; i++) {
      dst->move[i] = src->move[i];
      dst->value[i] = src->value[i];
   }
}

// list_move_to_front()

void list_move_to_front(list_t * list, int index) {

   int i;
   int move, value;

   ASSERT(list_is_ok(list));
   ASSERT(index>=0&&index<list->size);

   if (index != 0) {

      move = list->move[index];
      value = list->value[index];

      for (i = index; i > 0; i--) {
         list->move[i] = list->move[i-1];
         list->value[i] = list->value[i-1];
      }

      list->move[0] = move;
      list->value[0] = value;
   }
}

// list_note()

void list_note(list_t * list) {

   int i, move;

   ASSERT(list_is_ok(list));

   for (i = 0; i < list->size; i++) {
      move = list->move[i];
      ASSERT(move_is_ok(move));
      list->value[i] = -move_order(move);
   }
}

// list_sort()

void list_sort(list_t * list) {

   int i, j;
   int best_index, best_move, best_value;

   ASSERT(list_is_ok(list));

   for (i = 0; i < list->size-1; i++) {

      best_index = i;
      best_value = list->value[i];

      for (j = i+1; j < list->size; j++) {
         if (list->value[j] > best_value) {
            best_index = j;
            best_value = list->value[j];
         }
      }

      if (best_index != i) {

         best_move = list->move[best_index];
         ASSERT(best_value==list->value[best_index]);

         for (j = best_index; j > i; j--) {
            list->move[j] = list->move[j-1];
            list->value[j] = list->value[j-1];
         }

         list->move[i] = best_move;
         list->value[i] = best_value;
      }
   }

   if (DEBUG) {
      for (i = 0; i < list->size-1; i++) {
         ASSERT(list->value[i]>=list->value[i+1]);
      }
   }
}

// list_contain()

bool list_contain(const list_t * list, int move) {

   int i;

   ASSERT(list_is_ok(list));
   ASSERT(move_is_ok(move));

   for (i = 0; i < list->size; i++) {
      if (list->move[i] == move) return true;
   }

   return false;
}

// list_equal()

bool list_equal(list_t * list_1, list_t * list_2) {

   list_t copy_1[1], copy_2[1];
   int i;

   ASSERT(list_is_ok(list_1));
   ASSERT(list_is_ok(list_2));

   if (list_1->size != list_2->size) return false;

   list_copy(copy_1,list_1);
   list_note(copy_1);
   list_sort(copy_1);

   list_copy(copy_2,list_2);
   list_note(copy_2);
   list_sort(copy_2);

   for (i = 0; i < copy_1->size; i++) {
      if (copy_1->move[i] != copy_2->move[i]) return false;
   }

   return true;
}

// list_disp()

void list_disp(const list_t * list, const board_t * board) {

   int i, move, value;
   char string[256];

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));

   for (i = 0; i < list->size; i++) {

      move = list->move[i];
      value = list->value[i];

      if (!move_to_can(move,board,string,sizeof(string))) ASSERT(false);
      my_log("POLYGLOT %-5s %04X %+4d\n",string,move,value);
   }

   my_log("POLYGLOT\n");
}

// end of list.cpp

