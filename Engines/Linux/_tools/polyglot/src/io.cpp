#ifndef _WIN32

// io.cpp

// includes

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <unistd.h>

#include "io.h"
#include "util.h"

// constants

static const bool UseDebug = false;
static const bool UseCR = false;

static const int StringSize = 4096;

static const char LF = '\n';
static const char CR = '\r';

// prototypes

static int  my_read  (int fd, char string[], int size);
static void my_write (int fd, const char string[], int size);

// functions

// io_is_ok()

bool io_is_ok(const io_t * io) {

   if (io == NULL) return false;

   if (io->name == NULL) return false;

   if (io->in_eof != true && io->in_eof != false) return false;

   if (io->in_size < 0 || io->in_size > BufferSize) return false;
   if (io->out_size < 0 || io->out_size > BufferSize) return false;

   return true;
}

// io_init()

void io_init(io_t * io) {

   ASSERT(io!=NULL);

   io->in_eof = false;

   io->in_size = 0;
   io->out_size = 0;

   ASSERT(io_is_ok(io));
}

// io_close()

void io_close(io_t * io) {

   ASSERT(io_is_ok(io));

   ASSERT(io->out_fd>=0);

   my_log("> %s EOF\n",io->name);

   if (close(io->out_fd) == -1) {
      my_fatal("io_close(): close(): %s\n",strerror(errno));
   }

   io->out_fd = -1;
}

// io_get_update()

void io_get_update(io_t * io) {

   int pos, size;
   int n;

   ASSERT(io_is_ok(io));

   ASSERT(io->in_fd>=0);
   ASSERT(!io->in_eof);

   // init

   pos = io->in_size;

   size = BufferSize - pos;
   if (size <= 0) my_fatal("io_get_update(): buffer overflow\n");

   // read as many data as possible

   n = my_read(io->in_fd,&io->in_buffer[pos],size);
   if (UseDebug) my_log("POLYGLOT read %d byte%s from %s\n",n,(n>1)?"s":"",io->name);

   if (n > 0) { // at least one character was read

      // update buffer size

      ASSERT(n>=1&&n<=size);

      io->in_size += n;
      ASSERT(io->in_size>=0&&io->in_size<=BufferSize);

   } else { // EOF

      ASSERT(n==0);

      io->in_eof = true;
   }
}

// io_line_ready()

bool io_line_ready(const io_t * io) {

   ASSERT(io_is_ok(io));

   if (io->in_eof) return true;

   if (memchr(io->in_buffer,LF,io->in_size) != NULL) return true; // buffer contains LF

   return false;
}

// io_get_line()

bool io_get_line(io_t * io, char string[], int size) {

   int src, dst;
   int c;

   ASSERT(io_is_ok(io));
   ASSERT(string!=NULL);
   ASSERT(size>=256);

   src = 0;
   dst = 0;

   while (true) {

      // test for end of buffer

      if (src >= io->in_size) {
         if (io->in_eof) {
            my_log("%s->Adapter: EOF\n",io->name);
            return false;
         } else {
            my_fatal("io_get_line(): no EOL in buffer\n");
         }
      }

      // test for end of string

      if (dst >= size) my_fatal("io_get_line(): buffer overflow\n");

      // copy the next character

      c = io->in_buffer[src++];

      if (c == LF) { // LF => line complete
         string[dst] = '\0';
         break;
      } else if (c != CR) { // skip CRs
         string[dst++] = c;
      }
   }

   // shift the buffer

   ASSERT(src>0);

   io->in_size -= src;
   ASSERT(io->in_size>=0);

   if (io->in_size > 0) memmove(&io->in_buffer[0],&io->in_buffer[src],io->in_size);

   // return

   my_log("%s->Adapter: %s\n",io->name,string);

   return true;
}

// io_send()

void io_send(io_t * io, const char format[], ...) {

   va_list arg_list;
   char string[StringSize];
   int len;

   ASSERT(io_is_ok(io));
   ASSERT(format!=NULL);

   ASSERT(io->out_fd>=0);

   // format

   va_start(arg_list,format);
   vsprintf(string,format,arg_list);
   va_end(arg_list);

   // append string to buffer

   len = strlen(string);
   if (io->out_size + len > BufferSize-2) my_fatal("io_send(): buffer overflow\n");

   memcpy(&io->out_buffer[io->out_size],string,len);
   io->out_size += len;

   ASSERT(io->out_size>=0&&io->out_size<=BufferSize-2);

   // log

   io->out_buffer[io->out_size] = '\0';
   my_log("Adapter->%s: %s\n",io->name,io->out_buffer);
//	my_log("> %f %s %s\n",now_real(),io->name,io->out_buffer);
   // append EOL to buffer

   if (UseCR) io->out_buffer[io->out_size++] = CR;
   io->out_buffer[io->out_size++] = LF;

   ASSERT(io->out_size>=0&&io->out_size<=BufferSize);

   // flush buffer

   if (UseDebug) my_log("POLYGLOT writing %d byte%s to %s\n",io->out_size,(io->out_size>1)?"s":"",io->name);
   my_write(io->out_fd,io->out_buffer,io->out_size);

   io->out_size = 0;
}

// io_send_queue()

void io_send_queue(io_t * io, const char format[], ...) {

   va_list arg_list;
   char string[StringSize];
   int len;

   ASSERT(io_is_ok(io));
   ASSERT(format!=NULL);

   ASSERT(io->out_fd>=0);

   // format

   va_start(arg_list,format);
   vsprintf(string,format,arg_list);
   va_end(arg_list);

   // append string to buffer

   len = strlen(string);
   if (io->out_size + len > BufferSize-2) my_fatal("io_send_queue(): buffer overflow\n");

   memcpy(&io->out_buffer[io->out_size],string,len);
   io->out_size += len;

   ASSERT(io->out_size>=0&&io->out_size<=BufferSize-2);
}

// my_read()

static int my_read(int fd, char string[], int size) {

   int n;

   ASSERT(fd>=0);
   ASSERT(string!=NULL);
   ASSERT(size>0);

   do {
      n = read(fd,string,size);
   } while (n == -1 && errno == EINTR);

   if (n == -1) my_fatal("my_read(): read(): %s\n",strerror(errno));

   ASSERT(n>=0);

   return n;
}

// my_write()

static void my_write(int fd, const char string[], int size) {

   int n;

   ASSERT(fd>=0);
   ASSERT(string!=NULL);
   ASSERT(size>0);

   do {

      n = write(fd,string,size);

      // if (n == -1 && errno != EINTR && errno != EPIPE) my_fatal("my_write(): write(): %s\n",strerror(errno));

      if (n == -1) {
         if (false) {
         } else if (errno == EINTR) {
            n = 0; // nothing has been written
         } else if (errno == EPIPE) {
            n = size; // pretend everything has been written
         } else {
            my_fatal("my_write(): write(): %s\n",strerror(errno));
         }
      }

      ASSERT(n>=0);

      string += n;
      size -= n;

   } while (size > 0);

   ASSERT(size==0);
}

// end of io.cpp

#endif
