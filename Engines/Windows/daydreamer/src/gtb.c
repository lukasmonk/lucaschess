
#include "daydreamer.h"
#include "gtb/gtb-probe.h"
#include <string.h>

#define piece_to_gtb(p)     piece_to_gtb_table[p]
#define square_to_gtb(s)    square_to_index(s)
#define ep_to_gtb(s)        (s ? square_to_index(s) : tb_NOSQUARE)
#define stm_to_gtb(s)       (s)
#define castle_to_gtb(c)    castle_to_gtb_table[c]
#define DEFAULT_GTB_CACHE_SIZE  (32*1024*1024)
#define WDL_CACHE_FRACTION  112

static const char** tb_paths = NULL;
static const int piece_to_gtb_table[] = {
    tb_NOPIECE, tb_PAWN, tb_KNIGHT, tb_BISHOP, tb_ROOK, tb_QUEEN, tb_KING, 0,
    tb_NOPIECE, tb_PAWN, tb_KNIGHT, tb_BISHOP, tb_ROOK, tb_QUEEN, tb_KING, 0
};
static const int castle_to_gtb_table[] = {
    tb_NOCASTLE,
                                 tb_WOO,
                        tb_BOO         ,
                        tb_BOO | tb_WOO,
              tb_WOOO                  ,
              tb_WOOO |          tb_WOO,
              tb_WOOO | tb_BOO         ,
              tb_WOOO | tb_BOO | tb_WOO,
    tb_BOOO                            ,
    tb_BOOO |                    tb_WOO,
    tb_BOOO |           tb_BOO         ,
    tb_BOOO |           tb_BOO | tb_WOO,
    tb_BOOO | tb_WOOO                  ,
    tb_BOOO | tb_WOOO |          tb_WOO,
    tb_BOOO | tb_WOOO | tb_BOO,
    tb_BOOO | tb_WOOO | tb_BOO | tb_WOO,
};

typedef struct {
    int stm, ep, castle;
    unsigned int ws[17], bs[17];
    unsigned char wp[17], bp[17];
} gtb_args_t;

static gtb_args_t worker_args;
#ifdef WINDOWS_THREADS
HANDLE worker_mutex;
HANDLE worker_thread;
static DWORD WINAPI gtb_probe_firm_worker(LPVOID payload);
#else
#include <pthread.h>
pthread_mutex_t worker_mutex;
pthread_t worker_thread;
static void* gtb_probe_firm_worker(void* payload);
#endif
bool worker_task_ready;
bool worker_quit;

/*
 * Given a string identifying the location of Gaviota tb's, load those
 * tb's for use during search.
 */
bool load_gtb(char* gtb_pathlist, int cache_size_bytes)
{
    if (tb_is_initialized()) unload_gtb();
    assert(tb_paths == NULL);
    assert(cache_size_bytes >= 0);
    assert(tb_WHITE_TO_MOVE == WHITE && tb_BLACK_TO_MOVE == BLACK);
    char* path = NULL; 
    tb_paths = tbpaths_init();
    while ((path = strsep(&gtb_pathlist, ";"))) {
        if (!*path) continue;
        tb_paths = tbpaths_add(tb_paths, path);
    }
    int verbosity = 0;
    tb_init(verbosity, options.gtb_scheme, tb_paths);
    if (!cache_size_bytes) cache_size_bytes = DEFAULT_GTB_CACHE_SIZE;
    tbcache_init(cache_size_bytes, WDL_CACHE_FRACTION);
    tbstats_reset();
    bool success = tb_is_initialized() && tbcache_is_on();
    if (success) {
        if (options.verbosity) {
            printf("info string loaded Gaviota TBs\n");
        }
        worker_task_ready = false;
        worker_quit = false;
#ifdef WINDOWS_THREADS
        worker_mutex = CreateMutex(NULL, FALSE, NULL);
        worker_thread = CreateThread(
                NULL, 0, gtb_probe_firm_worker, NULL, 0, NULL);
        if (!worker_thread) {
            printf("Thread creation failed\n");
        }
#else
        pthread_mutex_init(&worker_mutex, NULL);
	if (pthread_create(&worker_thread,
                    NULL,
                    gtb_probe_firm_worker,
                    NULL)) perror("Worker thread creation failed.\n");
#endif
    } else if (options.verbosity) {
        printf("info string failed to load Gaviota TBs\n");
    }
    return success;
}

/*
 * Unload all tablebases and destroy the thread used for probing
 * during search.
 */
void unload_gtb(void)
{
    if (!tb_is_initialized()) return;
    tbcache_done();
    tb_done();
    tb_paths = tbpaths_done(tb_paths);
    worker_quit = true;
    worker_task_ready = true;
#ifdef WINDOWS_THREADS
    while (worker_quit) Sleep(1);
    CloseHandle(worker_mutex);
    CloseHandle(worker_thread);
#else
    while (worker_quit) usleep(100);
    pthread_mutex_destroy(&worker_mutex);
#endif
}

/*
 * Fill arrays with the information needed by the gtb probing code.
 */
static void fill_gtb_arrays(const position_t* pos,
        unsigned int* ws,
        unsigned int* bs,
        unsigned char* wp,
        unsigned char* bp)
{
    square_t sq;
    int count = 0, i;
    for (i=0; i<pos->num_pieces[WHITE]; ++i) {
        sq = pos->pieces[WHITE][i];
        ws[count] = square_to_gtb(sq);
        wp[count++] = piece_to_gtb(pos->board[sq]);
    }
    for (i=0; i<pos->num_pawns[WHITE]; ++i) {
        sq = pos->pawns[WHITE][i];
        ws[count] = square_to_gtb(sq);
        wp[count++] = piece_to_gtb(pos->board[sq]);
    }
    ws[count] = tb_NOSQUARE;
    wp[count] = tb_NOPIECE;

    count = 0;
    for (i=0; i<pos->num_pieces[BLACK]; ++i) {
        sq = pos->pieces[BLACK][i];
        bs[count] = square_to_gtb(sq);
        bp[count++] = piece_to_gtb(pos->board[sq]);
    }
    for (i=0; i<pos->num_pawns[BLACK]; ++i) {
        sq = pos->pawns[BLACK][i];
        bs[count] = square_to_gtb(sq);
        bp[count++] = piece_to_gtb(pos->board[sq]);
    }
    bs[count] = tb_NOSQUARE;
    bp[count] = tb_NOPIECE;
}

/*
 * Probe the tb cache only, without considering what's available on disk.
 * Get DTM information instead of just WDL.
 */
bool probe_gtb_soft_dtm(const position_t* pos, int* score)
{
    int stm = stm_to_gtb(pos->side_to_move);
    int ep = ep_to_gtb(pos->ep_square);
    int castle = castle_to_gtb(pos->castle_rights);

    unsigned int ws[17], bs[17];
    unsigned char wp[17], bp[17];
    fill_gtb_arrays(pos, ws, bs, wp, bp);

    unsigned res, val;
    int success = tb_probe_soft(stm, ep, castle, ws, bs, wp, bp, &res, &val);
    if (success) {
        if (res == tb_DRAW) *score = DRAW_VALUE;
        else if (res == tb_BMATE) *score = mated_in(val);
        else if (res == tb_WMATE) *score = mate_in(val);
        else assert(false);
        if (pos->side_to_move == BLACK) *score *= -1;
    }
    return success;
}

/*
 * Probe the tb cache only, without considering what's available on disk.
 */
bool probe_gtb_soft(const position_t* pos, int* score)
{
    int stm = stm_to_gtb(pos->side_to_move);
    int ep = ep_to_gtb(pos->ep_square);
    int castle = castle_to_gtb(pos->castle_rights);

    unsigned int ws[17], bs[17];
    unsigned char wp[17], bp[17];
    fill_gtb_arrays(pos, ws, bs, wp, bp);

    unsigned res;
    int success = tb_probe_WDL_soft(stm, ep, castle, ws, bs, wp, bp, &res);
    if (success) {
        if (res == tb_DRAW) *score = DRAW_VALUE;
        else if (res == tb_BMATE) *score = -MATE_VALUE + 1024;
        else if (res == tb_WMATE) *score = MATE_VALUE - 1024;
        else assert(false);
        if (pos->side_to_move == BLACK) *score *= -1;
    }
    return success;
}

/*
 * Probe all tbs, using the cache if available, but blocking and waiting for
 * disk if we miss in cache.
 * Get DTM information instead of just WDL.
 */
bool probe_gtb_hard_dtm(const position_t* pos, int* score)
{
    int stm = stm_to_gtb(pos->side_to_move);
    int ep = ep_to_gtb(pos->ep_square);
    int castle = castle_to_gtb(pos->castle_rights);

    unsigned int ws[17], bs[17];
    unsigned char wp[17], bp[17];
    fill_gtb_arrays(pos, ws, bs, wp, bp);

    unsigned res, val;
    int success = tb_probe_hard(stm, ep, castle, ws, bs, wp, bp, &res, &val);
    if (success) {
        if (res == tb_DRAW) *score = DRAW_VALUE;
        else if (res == tb_BMATE) *score = mated_in(val);
        else if (res == tb_WMATE) *score = mate_in(val);
        else assert(false);
        if (pos->side_to_move == BLACK) *score *= -1;
    }
    return success;
}

/*
 * Probe all tbs, using the cache if available, but blocking and waiting for
 * disk if we miss in cache.
 */
bool probe_gtb_hard(const position_t* pos, int* score)
{
    int stm = stm_to_gtb(pos->side_to_move);
    int ep = ep_to_gtb(pos->ep_square);
    int castle = castle_to_gtb(pos->castle_rights);

    unsigned int ws[17], bs[17];
    unsigned char wp[17], bp[17];
    fill_gtb_arrays(pos, ws, bs, wp, bp);

    unsigned res;
    int success = tb_probe_WDL_hard(stm, ep, castle, ws, bs, wp, bp, &res);
    if (success) {
        if (res == tb_DRAW) *score = DRAW_VALUE;
        else if (res == tb_BMATE) *score = -MIN_MATE_VALUE;
        else if (res == tb_WMATE) *score = MIN_MATE_VALUE;
        else assert(false);
        if (pos->side_to_move == BLACK) *score *= -1;
    }
    return success;
}

/*
 * A compromise between probe_hard and probe_soft. Check the cache, and return
 * if the position is found. If not, use a thread to load the position into
 * cache in the background while we return to the main search. The background
 * load uses a worker thread, and has minimal load implications because the
 * thread is blocked nearly 100% of the time.
 * Get DTM information instead of just WDL.
 */
bool probe_gtb_firm_dtm(const position_t* pos, int* score)
{
    gtb_args_t args_storage;
    gtb_args_t* gtb_args = &args_storage;

    gtb_args->stm = stm_to_gtb(pos->side_to_move);
    gtb_args->ep = ep_to_gtb(pos->ep_square);
    gtb_args->castle = castle_to_gtb(pos->castle_rights);

    fill_gtb_arrays(pos, gtb_args->ws, gtb_args->bs,
            gtb_args->wp, gtb_args->bp);

    unsigned res, val;
    int success = tb_probe_soft(gtb_args->stm, gtb_args->ep, gtb_args->castle,
            gtb_args->ws, gtb_args->bs, gtb_args->wp, gtb_args->bp, &res, &val);
    if (success) {
        if (res == tb_DRAW) *score = DRAW_VALUE;
        else if (res == tb_BMATE) *score = mated_in(val);
        else if (res == tb_WMATE) *score = mate_in(val);
        else assert(false);
        if (pos->side_to_move == BLACK) *score *= -1;
        return true;
    }

    if (worker_task_ready) return false;
    memcpy(&worker_args, gtb_args, sizeof(gtb_args_t));
    worker_task_ready = true;
    return false;
}

/*
 * A compromise between probe_hard and probe_soft. Check the cache, and return
 * if the position is found. If not, use a thread to load the position into
 * cache in the background while we return to the main search. The background
 * load uses a worker thread, and has minimal load implications because the
 * thread is blocked nearly 100% of the time.
 */
bool probe_gtb_firm(const position_t* pos, int* score)
{
    gtb_args_t args_storage;
    gtb_args_t* gtb_args = &args_storage;

    gtb_args->stm = stm_to_gtb(pos->side_to_move);
    gtb_args->ep = ep_to_gtb(pos->ep_square);
    gtb_args->castle = castle_to_gtb(pos->castle_rights);

    fill_gtb_arrays(pos, gtb_args->ws, gtb_args->bs,
            gtb_args->wp, gtb_args->bp);

    unsigned res;
    int success = tb_probe_WDL_soft(gtb_args->stm, gtb_args->ep,
            gtb_args->castle, gtb_args->ws, gtb_args->bs,
            gtb_args->wp, gtb_args->bp, &res);
    if (success) {
        if (res == tb_DRAW) *score = DRAW_VALUE;
        else if (res == tb_BMATE) *score = -MIN_MATE_VALUE;
        else if (res == tb_WMATE) *score = MIN_MATE_VALUE;
        else assert(false);
        if (pos->side_to_move == BLACK) *score *= -1;
        return true;
    }

    if (worker_task_ready) return false;
    memcpy(&worker_args, gtb_args, sizeof(gtb_args_t));
    worker_task_ready = true;
    return false;
}

/*
 * The worker function for background probing in probe_firm.
 */
#ifdef WINDOWS_THREADS
DWORD WINAPI gtb_probe_firm_worker(LPVOID payload)
#else
void* gtb_probe_firm_worker(void* payload)
#endif
{
    (void)payload;
    while (true) {
        // Wait for a readied task.
        int counter = 0;
        while (++counter < 5000 && !worker_task_ready) {}
        if (!worker_task_ready) {
#ifdef WINDOWS_THREADS
            while (!worker_task_ready) Sleep(1);
#else
            while (!worker_task_ready) usleep(100);
#endif
        }

        // We've been woken back up, there must be something for us to do.
        if (worker_quit) break;

        unsigned res;
        int success = tb_probe_WDL_hard(worker_args.stm,
                worker_args.ep,
                worker_args.castle,
                worker_args.ws,
                worker_args.bs,
                worker_args.wp,
                worker_args.bp,
                &res);
        (void)success;
        if (worker_quit) break;

        worker_task_ready = false;
    }

    worker_task_ready = false;
    worker_quit = false;
    return 0;
}

