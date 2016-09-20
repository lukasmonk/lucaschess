/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef THREAD_H
#define THREAD_H

#include <inttypes.h>
#include <limits.h>

#ifndef __MINGW32__
#include <pthread.h>
#else
#include <windows.h>
#endif

#include "position.h"
#include "search.h"

#define HistoryMax 16384

class QHash{
    public:
        Key key;
        int8_t flag;
        int8_t depth;
        int16_t eval;
        int16_t move;
        static const int ALPHA = 1;
        static const int BETA = 2;
        static const int EXACT = 3;
        static const int EVAL = 4;
};

class MaterialHash{
    public:
        Key key;
        int8_t recog;
        int8_t flags;
        int8_t cflags[2];
        int8_t mul[2];
        int16_t phase;
        int16_t eval[2];
};

class PawnHash{
    public:
        Key key;
        int16_t eval[2];
        Bitboard passed[2];
};

class Thread
{
    public:
        Thread(const int m_hash, const int p_hash, const int q_hash);
        ~Thread();
        void set_position(const Position& pos);
        static void start_search(const Position pos);
        static void stop_search();

		void move_good(const Move b_move, const Move h_move[256], const int h_size, const int ply, const int depth);
		int eval_history_move(const Move move) const;
        void reset();
        QHash* find(const Key key);
        void write(const Key key, const int eval, const int depth, const int flag, const Move move);

        int id;
        Position pos;
#if defined(__MINGW32__)
		HANDLE wthread;
#else
        pthread_t pthread;
#endif

        bool stopped;
        Move killer[MAX_SEARCH_PLY][2];
		int16_t history[14][64][64];
		int16_t his_tot[14][64][64];
		int16_t his_hit[14][64][64];
		MaterialHash* material_hash;
        PawnHash* pawn_hash;
        QHash* hash;
        int ph_size;
        int mh_size;
        int qh_size;

        int m_hits;
        int p_hits;
        int h_hits;

    private:
#if defined(__MINGW32__)
		friend DWORD WINAPI thread_run(LPVOID arg);
#else
       	friend void *thread_run(void *thread);
#endif
        static Thread s_thread;
        bool joinable;
};

#endif // THREAD_H
