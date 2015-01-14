
#include "daydreamer.h"
#include "gtb/gtb-probe.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    OPTION_CHECK,
    OPTION_SPIN,
    OPTION_COMBO,
    OPTION_BUTTON,
    OPTION_STRING
} uci_option_type_t;

typedef void(*option_handler)(void*, char*);

typedef struct {
    uci_option_type_t type;
    char name[128];
    char value[128];
    char vars[16][128];
    char default_value[128];
    int min;
    int max;
    void* address;
    option_handler handler;
} uci_option_t;

static int uci_option_count = 0;
static uci_option_t uci_options[128];

/*
 * Create an option and add it to the array. The set of things you have to
 * specify is a little weird to allow all option types to be handled with
 * the same function and data structure.
 */
static void add_uci_option(char* name,
        uci_option_type_t type,
        char* default_value,
        int min,
        int max,
        char** vars,
        void* address,
        option_handler handler)
{
    assert(uci_option_count < 128);
    uci_option_t* option = &uci_options[uci_option_count++];
    strcpy(option->name, name);
    strcpy(option->default_value, default_value);
    strcpy(option->value, default_value);
    option->type = type;
    option->min = min;
    option->max = max;
    option->handler = handler;
    option->vars[0][0] = '\0';
    option->address = address;
    int var_index=0;
    while (vars && vars[var_index]) {
        assert(var_index < 15);
        strcpy(option->vars[var_index], vars[var_index]);
        var_index++;
    }
    option->vars[var_index][0] = '\0';
    char option_command[256];
    sprintf(option_command, "%s value %s", option->name, option->default_value);
    set_uci_option(option_command);
}

/*
 * Convert an option into its uci description string and print it.
 */
static void print_uci_option(uci_option_t* option)
{
    char* option_types[] = { "check", "spin", "combo", "button", "string" };
    printf("option name %s type %s", option->name, option_types[option->type]);
    if (option->type == OPTION_BUTTON) {
        printf("\n");
        return;
    }
    printf(" default %s", option->default_value);
    if (option->type == OPTION_COMBO) {
        int var_index=0;
        while (option->vars[var_index][0]) {
            printf(" var %s", option->vars[var_index++]);
        }
    } else if (option->type == OPTION_SPIN) {
        printf(" min %d max %d", option->min, option->max);
    }
    printf("\n");
}

/*
 * Dump all uci options to the console. Used during startup.
 */
void print_uci_options(void)
{
    for (int i=0; i<uci_option_count; ++i) {
        print_uci_option(&uci_options[i]);
    }
}

/*
 * Find the uci option structure with the given name.
 */
static uci_option_t* get_uci_option(const char* name)
{
    for (int i=0; i<uci_option_count; ++i) {
        int name_length = strlen(uci_options[i].name);
        if (!strncasecmp(name, uci_options[i].name, name_length)) {
            return &uci_options[i];
        }
    }
    return NULL;
}

/*
 * Handle a console command that sets a uci option. We receive the part of the
 * command that looks like:
 * <name> value <value>
 * Each option has its own handler callback which is invoked.
 */
void set_uci_option(char* command)
{
    while (isspace(*command)) ++command;
    uci_option_t* option = get_uci_option(command);
    if (!option) {
        printf("info string Could not recognize option string %s\n", command);
        return;
    }
    command = strcasestr(command, "value ");
    if (command) {
        command += 6;
        while(isspace(*command)) ++command;
    } else if (option->type != OPTION_BUTTON) {
        printf("info string Invalid option string\n");
        return;
    }
    option->handler(option, command);
}

/*
 * Get the current value string associated with an option.
 */
char* get_option_string(const char* name)
{
    uci_option_t* option = get_uci_option(name);
    assert(option);
    return option->value;
}

/*
 * The default option handler. It copies the input into the value buffer, and
 * if an address is defined for the option, the string is interpreted as
 * either an int or a bool and is read into that address.
 */
static void default_handler(void* opt, char* value)
{
    uci_option_t* option = opt;
    if (value) strncpy(option->value, value, 128);
    if (option->address) {
        if (option->type == OPTION_CHECK) {
            bool val = strcasestr(option->value, "true") ? true : false;
            memcpy(option->address, &val, sizeof(bool));
        } else if (option->type == OPTION_SPIN) {
            int val;
            sscanf(option->value, "%d", &val);
            memcpy(option->address, &val, sizeof(int));
        } else assert(false);
    }
}

static void handle_verbosity(void* opt, char* value)
{
    if (!value) return;
    uci_option_t* option = opt;
    if (value) strncpy(option->value, value, 128);
    options.verbosity = 0;
    if (!strcasecmp(value, "low")) options.verbosity = 0;
    else if (!strcasecmp(value, "medium")) options.verbosity = 1;
    else if (!strcasecmp(value, "high")) options.verbosity = 2;
}

/*
 * Initialize the transposition table.
 */
static void handle_hash(void* opt, char* value)
{
    uci_option_t* option = opt;
    int mbytes = 0;
    strncpy(option->value, value, 128);
    sscanf(value, "%d", &mbytes);
    if (mbytes < option->min || mbytes > option->max) {
        warn("Option value out of range, using default\n");
        sscanf(option->default_value, "%d", &mbytes);
    }
    init_transposition_table(mbytes * (1ull<<20));
}

/*
 * Initialize the pawn cache.
 */
static void handle_pawn_cache(void* opt, char* value)
{
    uci_option_t* option = opt;
    int mbytes = 0;
    strncpy(option->value, value, 128);
    sscanf(value, "%d", &mbytes);
    if (mbytes < option->min || mbytes > option->max) {
        warn("Option value out of range, using default\n");
        sscanf(option->default_value, "%d", &mbytes);
    }
    init_pawn_table(mbytes * (1ull<<20));
}

/*
 * Initialize the pv cache.
 */
static void handle_pv_cache(void* opt, char* value)
{
    uci_option_t* option = opt;
    int mbytes = 0;
    strncpy(option->value, value, 128);
    sscanf(value, "%d", &mbytes);
    if (mbytes < option->min || mbytes > option->max) {
        warn("Option value out of range, using default\n");
        sscanf(option->default_value, "%d", &mbytes);
    }
    init_pv_cache(mbytes * (1ull<<20));
}

/*
 * Clear the transposition table.
 */
static void handle_clear_hash(void* opt, char* value)
{
    (void) opt; (void) value;
    clear_transposition_table();
}

/*
 * Turns Gaviota tablebase use on and off.
 */
static void handle_gtb_use(void* opt, char* value)
{
    uci_option_t* option = opt;
    if (!option->value) return;
    strncpy(option->value, value, 128);
    bool val = !strcasecmp(value, "true");
    if (val) {
        load_gtb(get_option_string("gaviota tablebase path"),
                options.gtb_cache_size*1024*1024);
    } else unload_gtb();
    memcpy(option->address, &val, sizeof(bool));
}

/*
 * Sets the path used to look for Gaviota bitbases, reloading them if the
 * appropriate option is set.
 */
static void handle_gtb_path(void* opt, char* value)
{
    uci_option_t* option = opt;
    strncpy(option->value, value, 128);
    int len = strlen(option->value);
    if (strrchr(option->value, DIR_SEP[0]) - option->value + 1 != len) {
        strcat(option->value, DIR_SEP);
    }
    if (options.use_gtb) {
        load_gtb(option->value, options.gtb_cache_size*1024*1024);
    }
}

/*
 * Sets the Gaviota tablebase cache size.
 */
static void handle_gtb_cache(void* opt, char* value)
{
    uci_option_t* option = opt;
    if (!option->value) return;
    strncpy(option->value, value, 128);
    int size;
    sscanf(value, "%d", &size);
    memcpy(option->address, &size, sizeof(int));
    if (options.use_gtb) {
        load_gtb(get_option_string("gaviota tablebase path"),
                options.gtb_cache_size*1024*1024);
    }
}

/*
 * Sets the Gaviota tablebase compression scheme.
 */
static void handle_gtb_scheme(void* opt, char* value)
{
    if (!value) return;
    uci_option_t* option = opt;
    if (value) strncpy(option->value, value, 128);
    options.gtb_scheme = tb_CP4;
    if (!strcasecmp(value, "uncompressed")) {
        options.gtb_scheme = tb_UNCOMPRESSED;
    } else if (!strcasecmp(value, "cp1")) options.gtb_scheme = tb_CP1;
    else if (!strcasecmp(value, "cp2")) options.gtb_scheme = tb_CP2;
    else if (!strcasecmp(value, "cp3")) options.gtb_scheme = tb_CP3;
    else if (!strcasecmp(value, "cp4")) options.gtb_scheme = tb_CP4;
    if (options.use_gtb) {
        load_gtb(get_option_string("gaviota tablebase path"),
                options.gtb_cache_size*1024*1024);
    }
}

/*
 * Turns Scorpio bitbase use on and off.
 */
static void handle_scorpio_bb_use(void* opt, char* value)
{
    uci_option_t* option = opt;
    if (!option->value) return;
    strncpy(option->value, value, 128);
    bool val = !strcasecmp(value, "true");
    if (val) {
        load_scorpio_bb(get_option_string("Scorpio bitbase path"), 0);
    } else unload_scorpio_bb();
    memcpy(option->address, &val, sizeof(bool));
}

/*
 * Sets the path used to look for Scorpio bitbases, reloading them if the
 * appropriate option is set.
 */
static void handle_scorpio_bb_path(void* opt, char* value)
{
    uci_option_t* option = opt;
    strncpy(option->value, value, 128);
    int len = strlen(option->value);
    if (strrchr(option->value, DIR_SEP[0]) - option->value + 1 != len) {
        strcat(option->value, DIR_SEP);
    }
    if (options.use_scorpio_bb) {
        load_scorpio_bb(option->value, 0);
    }
}

/*
 * Sets the path to the opening book, in Polyglot or ctg format, and
 * set the function for book probing accordingly.
 */
static void handle_book_file(void* opt, char* value)
{
    uci_option_t* option = opt;
    strncpy(option->value, value, 128);
    int name_len = strlen(value);
    if (value[name_len-3] == 'c' &&
            value[name_len-2] == 't' &&
            value[name_len-1] == 'g') {
        options.book_loaded = init_ctg_book(option->value);
        options.probe_book = &get_ctg_book_move;
    } else {
        options.book_loaded = init_poly_book(option->value);
        options.probe_book = &get_poly_book_move;
    }
}

/*
 * Create all uci options and set them to their default values. Also set
 * default values for any options that aren't exposed to the uci interface.
 */
void init_uci_options()
{
    add_uci_option("Hash", OPTION_SPIN, "64",
            1, 4096, NULL, NULL, &handle_hash);
    add_uci_option("Clear Hash", OPTION_BUTTON, "",
            0, 0, NULL, NULL, &handle_clear_hash);
    add_uci_option("Ponder", OPTION_CHECK, "false",
            0, 0, NULL, &options.ponder, &default_handler);
    add_uci_option("MultiPV", OPTION_SPIN, "1",
            1, 256, NULL, &options.multi_pv, &default_handler);
    add_uci_option("OwnBook", OPTION_CHECK, "false",
            0, 0, NULL, &options.use_book, &default_handler);
    add_uci_option("Book file", OPTION_STRING, "book.bin",
            0, 0, NULL, NULL, &handle_book_file);
    add_uci_option("UCI_Chess960", OPTION_CHECK, "false",
            0, 0, NULL, &options.chess960, &default_handler);
    add_uci_option("Arena-style 960 castling", OPTION_CHECK, "false",
            0, 0, NULL, &options.arena_castle, &default_handler);
    add_uci_option("Use Gaviota tablebases", OPTION_CHECK, "false",
            0, 0, NULL, &options.use_gtb, &handle_gtb_use);
    add_uci_option("Gaviota tablebase path", OPTION_STRING, ".",
            0, 0, NULL, NULL, &handle_gtb_path);
    char* schemes[6] = { "uncompressed", "cp1", "cp2", "cp3", "cp4", NULL };
    add_uci_option("Gaviota compression scheme", OPTION_COMBO, "cp4",
            0, 0, schemes, &options.gtb_scheme, &handle_gtb_scheme);
    add_uci_option("Gaviota tablebase cache size", OPTION_SPIN, "32",
            0, 4096, NULL, &options.gtb_cache_size, &handle_gtb_cache);
    add_uci_option("Load tablebases in a separate thread", OPTION_CHECK, "true",
            0, 0, NULL, &options.nonblocking_gtb, &default_handler);
    add_uci_option("Tablebase pieces", OPTION_SPIN, "5",
            3, 6, NULL, &options.max_egtb_pieces, &default_handler);
    add_uci_option("Use Scorpio bitbases", OPTION_CHECK, "false",
            0, 0, NULL, &options.use_scorpio_bb, &handle_scorpio_bb_use);
    add_uci_option("Scorpio bitbase path", OPTION_STRING, ".",
            0, 0, NULL, NULL, &handle_scorpio_bb_path);
    add_uci_option("Pawn cache size", OPTION_SPIN, "1",
            1, 128, NULL, NULL, &handle_pawn_cache);
    add_uci_option("PV cache size", OPTION_SPIN, "32",
            1, 1024, NULL, NULL, &handle_pv_cache);
    add_uci_option("Output Delay", OPTION_SPIN, "2000",
            0, 1000000, NULL, &options.output_delay, &default_handler);
    char* verbosities[4] = { "low", "medium", "high", NULL };
    add_uci_option("Verbosity", OPTION_COMBO, "low",
            0, 0, verbosities, &options.verbosity, &handle_verbosity);
    options.book_loaded = false;
}

