
#include "daydreamer.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static position_t pgn_position;

/*
 * Read the given pgn file. This is just a stub for future development.
 */
void read_pgn(char* filename)
{
    position_t* pos = &pgn_position;
    set_position(pos, FEN_STARTPOS);
    char line[4096];
    char* ch = line;
    FILE* pgn_file = fopen(filename, "r");
    if (!pgn_file) {
        printf("Couldn't open pgn file %s: %s\n", filename, strerror(errno));
        return;
    }
    (void)ch;
    /*
    while (fgets(line, 4096, pgn_file)) {
        char *bm=NULL, *id=NULL, *token=NULL;
        set_position(&root_data.root_pos, test);
        while ((token = strsep(&test, "; \t"))) {
            if (!*token) continue;
            if (strcasestr(token, "bm")) {
                while (!bm || !*bm) bm = strsep(&test, "; \t");
                best_move = san_str_to_move(&root_data.root_pos, bm);
            }
            if (strcasestr(token, "id")) {
                strsep(&test, "\"");
                id = strsep(&test, "\"");
            }
        }
        ch = line;
    }
    */
}
