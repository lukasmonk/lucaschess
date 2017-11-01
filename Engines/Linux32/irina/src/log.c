#include <stdio.h>
#include <stdarg.h>

FILE * flog = NULL;
void open_log()
{
    if( ! flog ) flog = fopen("irina.log", "a");
}

void close_log()
{
    if( flog ) fclose(flog);
}

// position fen 7R/1k3r2/5n2/2p4p/4KB2/P6P/1P3P2/8 w - - 6 43 moves e4e5 f6d7
