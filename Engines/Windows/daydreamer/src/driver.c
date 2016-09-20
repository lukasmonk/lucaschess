
#include "version.h"
#include "daydreamer.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>

#define COMPILE_COMMAND "gcc (TDM-2 mingw32) 4.4.1"


int main(int argc, char* argv[])
{
    // Set unbuffered i/o.
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);

    // Print some identifying information, then do initialization.
    printf("%s %s, by %s\n", ENGINE_NAME, ENGINE_VERSION, ENGINE_AUTHOR);
    printf("Compiled %s %s", __DATE__, __TIME__);
#ifdef COMPILE_COMMAND
    printf(", using:\n%s", COMPILE_COMMAND);
#endif
    printf("\n");
    init_daydreamer();

    // Read from uci script, if possible.
    char* script_name = argc > 1 ? argv[1] : "daydreamer.rc";
    FILE* script;
    if ((script = fopen(script_name, "r"))) {
        printf("Reading input script...\n");
        uci_read_stream(script);
        fclose(script);
        printf("done.\n");
    }

    // Main loop input processing.
    uci_read_stream(stdin);
}

