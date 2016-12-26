
// book_merge.cpp

// includes

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "book_merge.h"
#include "util.h"

// types

struct book_t {
   FILE * file;
   int size;
};

struct entry_t {
   uint64 key;
   uint16 move;
   uint16 count;
   uint16 n;
   uint16 sum;
};

// variables

static book_t In1[1];
static book_t In2[1];
static book_t Out[1];

// prototypes

static void   book_clear    (book_t * book);

static void   book_open     (book_t * book, const char file_name[], const char mode[]);
static void   book_close    (book_t * book);

static bool   read_entry    (book_t * book, entry_t * entry, int n);
static void   write_entry   (book_t * book, const entry_t * entry);

static uint64 read_integer  (FILE * file, int size);
static void   write_integer (FILE * file, int size, uint64 n);

// functions

// book_merge()

void book_merge(int argc, char * argv[]) {

   int i;
   const char * in_file_1;
   const char * in_file_2;
   const char * out_file;
   int i1, i2;
   bool b1, b2;
   entry_t e1[1], e2[1];
   int skip;

   in_file_1 = NULL;
   my_string_clear(&in_file_1);

   in_file_2 = NULL;
   my_string_clear(&in_file_2);

   out_file = NULL;
   my_string_set(&out_file,"out.bin");

   for (i = 1; i < argc; i++) {

      if (false) {

      } else if (my_string_equal(argv[i],"merge-book")) {

         // skip

      } else if (my_string_equal(argv[i],"-in1")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_merge(): missing argument\n");

         my_string_set(&in_file_1,argv[i]);

      } else if (my_string_equal(argv[i],"-in2")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_merge(): missing argument\n");

         my_string_set(&in_file_2,argv[i]);

      } else if (my_string_equal(argv[i],"-out")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_merge(): missing argument\n");

         my_string_set(&out_file,argv[i]);

      } else {

         my_fatal("book_merge(): unknown option \"%s\"\n",argv[i]);
      }
   }

   book_clear(In1);
   book_clear(In2);
   book_clear(Out);

   book_open(In1,in_file_1,"rb");
   book_open(In2,in_file_2,"rb");
   book_open(Out,out_file,"wb");

   skip = 0;

   i1 = 0;
   i2 = 0;

   while (true) {

      b1 = read_entry(In1,e1,i1);
      b2 = read_entry(In2,e2,i2);

      if (false) {

      } else if (!b1 && !b2) {

         break;

      } else if (b1 && !b2) {

         write_entry(Out,e1);
         i1++;

      } else if (b2 && !b1) {

         write_entry(Out,e2);
         i2++;

      } else {

         ASSERT(b1);
         ASSERT(b2);

         if (false) {
         } else if (e1->key < e2->key) {
            write_entry(Out,e1);
            i1++;
         } else if (e1->key > e2->key) {
            write_entry(Out,e2);
            i2++;
         } else {
            ASSERT(e1->key==e2->key);
            skip++;
            i2++;
         }
      }
   }

   book_close(In1);
   book_close(In2);
   book_close(Out);

   if (skip != 0) {
      printf("skipped %d entr%s.\n",skip,(skip>1)?"ies":"y");
   }

   printf("done!\n");
}

// book_clear()

static void book_clear(book_t * book) {

   ASSERT(book!=NULL);

   book->file = NULL;
   book->size = 0;
}

// book_open()

static void book_open(book_t * book, const char file_name[], const char mode[]) {

   ASSERT(book!=NULL);
   ASSERT(file_name!=NULL);
   ASSERT(mode!=NULL);

   book->file = fopen(file_name,mode);
   if (book->file == NULL) my_fatal("book_open(): can't open file \"%s\": %s\n",file_name,strerror(errno));

   if (fseek(book->file,0,SEEK_END) == -1) {
      my_fatal("book_open(): fseek(): %s\n",strerror(errno));
   }

   book->size = ftell(book->file) / 16;
}

// book_close()

static void book_close(book_t * book) {

   ASSERT(book!=NULL);

   if (fclose(book->file) == EOF) {
      my_fatal("book_close(): fclose(): %s\n",strerror(errno));
   }
}

// read_entry()

static bool read_entry(book_t * book, entry_t * entry, int n) {

   ASSERT(book!=NULL);
   ASSERT(entry!=NULL);

   if (n < 0 || n >= book->size) return false;

   ASSERT(n>=0&&n<book->size);

   if (fseek(book->file,n*16,SEEK_SET) == -1) {
      my_fatal("read_entry(): fseek(): %s\n",strerror(errno));
   }

   entry->key   = read_integer(book->file,8);
   entry->move  = (uint16)read_integer(book->file,2);
   entry->count = (uint16)read_integer(book->file,2);
   entry->n     = (uint16)read_integer(book->file,2);
   entry->sum   = (uint16)read_integer(book->file,2);

   return true;
}

// write_entry()

static void write_entry(book_t * book, const entry_t * entry) {

   ASSERT(book!=NULL);
   ASSERT(entry!=NULL);

   write_integer(book->file,8,entry->key);
   write_integer(book->file,2,entry->move);
   write_integer(book->file,2,entry->count);
   write_integer(book->file,2,entry->n);
   write_integer(book->file,2,entry->sum);
}

// read_integer()

static uint64 read_integer(FILE * file, int size) {

   uint64 n;
   int i;
   int b;

   ASSERT(file!=NULL);
   ASSERT(size>0&&size<=8);

   n = 0;

   for (i = 0; i < size; i++) {

      b = fgetc(file);

      if (b == EOF) {
         if (feof(file)) {
            my_fatal("read_integer(): fgetc(): EOF reached\n");
         } else { // error
            my_fatal("read_integer(): fgetc(): %s\n",strerror(errno));
         }
      }

      ASSERT(b>=0&&b<256);
      n = (n << 8) | b;
   }

   return n;
}

// write_integer()

static void write_integer(FILE * file, int size, uint64 n) {

   int i;
   int b;

   ASSERT(file!=NULL);
   ASSERT(size>0&&size<=8);
   ASSERT(size==8||n>>(size*8)==0);

   for (i = size-1; i >= 0; i--) {

      b = (int)((n >> (i*8)) & 0xFF);
      ASSERT(b>=0&&b<256);

      if (fputc(b,file) == EOF) {
         my_fatal("write_integer(): fputc(): %s\n",strerror(errno));
      }
   }
}

// end of book_merge.cpp

