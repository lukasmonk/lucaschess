/*--------------------------------------------------------------------------
    Pawny 1.2, chess engine (source code).
    Copyright (C) 2009 - 2016 by Mincho Georgiev.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#include "data.h"
#include "inline.h"
#include <math.h>

uint64 *b_moves[64];
uint64 *r_moves[64];

const unsigned int b_bits[64] =
{ 6, 5, 5, 5, 5, 5, 5, 6, 
  5, 5, 5, 5, 5, 5, 5, 5, 
  5, 5, 7, 7, 7, 7, 5, 5, 
  5, 5, 7, 9, 9, 7, 5, 5, 
  5, 5, 7, 9, 9, 7, 5, 5, 
  5, 5, 7, 7, 7, 7, 5, 5, 
  5, 5, 5, 5, 5, 5, 5, 5, 
  6, 5, 5, 5, 5, 5, 5, 6
};

const unsigned int r_bits[64] =
{ 12, 11, 11, 11, 11, 11, 11, 12, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  11, 10, 10, 10, 10, 10, 10, 11, 
  12, 11, 11, 11, 11, 11, 11, 12
};

const unsigned int b_shift[64] =
{ (64-6), (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), (64-6), 
  (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), 
  (64-5), (64-5), (64-7), (64-7), (64-7), (64-7), (64-5), (64-5), 
  (64-5), (64-5), (64-7), (64-9), (64-9), (64-7), (64-5), (64-5), 
  (64-5), (64-5), (64-7), (64-9), (64-9), (64-7), (64-5), (64-5), 
  (64-5), (64-5), (64-7), (64-7), (64-7), (64-7), (64-5), (64-5), 
  (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), 
  (64-6), (64-5), (64-5), (64-5), (64-5), (64-5), (64-5), (64-6)
};

const unsigned int r_shift[64] = 
{ (64-12), (64-11), (64-11), (64-11), (64-11), (64-11), (64-11), (64-12), 
  (64-11), (64-10), (64-10), (64-10), (64-10), (64-10), (64-10), (64-11), 
  (64-11), (64-10), (64-10), (64-10), (64-10), (64-10), (64-10), (64-11), 
  (64-11), (64-10), (64-10), (64-10), (64-10), (64-10), (64-10), (64-11), 
  (64-11), (64-10), (64-10), (64-10), (64-10), (64-10), (64-10), (64-11), 
  (64-11), (64-10), (64-10), (64-10), (64-10), (64-10), (64-10), (64-11), 
  (64-11), (64-10), (64-10), (64-10), (64-10), (64-10), (64-10), (64-11), 
  (64-12), (64-11), (64-11), (64-11), (64-11), (64-11), (64-11), (64-12)
};


bitboard_t get_rook_atk(const int sq, const bitboard_t occ)
{
  bitboard_t x, result = 0ULL;
  const bitboard_t loc = 1ULL << sq;
  const bitboard_t rank = 0xFFULL << (8 * (sq / 8));
	
  //up:
  for(x = loc; x && !(x & occ);result |= x) x <<= 8;
  //down:
  for(x = loc; x && !(x & occ);result |= x) x >>= 8;
  //left:
  for(x = loc; x && !(x & occ);result |= x)
  { x >>= 1;
    if(!(x & rank)) break;
  }
  //right:
  for(x = loc; x && !(x & occ);result |= x)
  { x <<= 1;
    if(!(x & rank)) break;
  }
  return result;
}

bitboard_t get_bishop_atk(const int sq, const bitboard_t occ)
{
  bitboard_t diag;
  bitboard_t trace;
  bitboard_t result = 0ULL;
  const bitboard_t loc = 1ULL << sq;
  const bitboard_t rank = 0xFFULL << (8 * (sq / 8));
	
  //up-left:
  for(diag = trace = loc; !(diag & occ);)
  { diag <<= 7; trace >>= 1;
    if(diag && (trace & rank)) result |= diag;
    else break;
  }
  //up-right:
  for(diag = trace = loc; !(diag & occ);)
  { diag <<= 9; trace <<= 1;
    if(diag && (trace & rank)) result |= diag;
    else break;
  }
  //down-left:
  for(diag = trace = loc; !(diag & occ);)
  { diag >>= 9; trace >>= 1;
    if(diag && (trace & rank)) result |= diag;
    else break;
  }
  //down-right:
  for(diag = trace = loc; !(diag & occ);)
  { diag >>= 7; trace <<= 1;
    if(diag && (trace & rank)) result |= diag;
    else break;
  }
  return result;
}

bitboard_t get_blockers(const bitboard_t x, bitboard_t mask)
{
  int i;
  bitboard_t temp,result = 0ULL;
	
  for(i = 0; mask != 0ULL; i++)
  { temp = mask &- mask;
    if(x & (1ULL << i)) result |= (1ULL << bitscanf(temp));
    mask ^= temp;
  }
  return result;
}

void init_sliders()
{	
  int i;
  bitboard_t x,occ;

  for(i = 0; i < 64; i++)
  {//initializing the square hashtable pointers:
    b_moves[i] = (bitboard_t *)aligned_malloc((size_t)(1 << b_bits[i]) * sizeof(bitboard_t),64);
    r_moves[i] = (bitboard_t *)aligned_malloc((size_t)(1 << r_bits[i]) * sizeof(bitboard_t),64);
    //for each square: 
    //  - get each possible occupancy state according the mask of relevant squares
    //  - get the attack config. against each occupancy state
    //  - hashing by the multiplication method to retrieve index: 
    //    the currently generated occupancy state * magic number >> relevant mask bits count:
    //    (Pradyumna Kannan's reduced array access approach.)
    //  - store the attack configuration at position [index] in the hashtable.
    for(x = 0ULL; x < (1ULL << b_bits[i]); x++)
    { occ = get_blockers(x, b_mask[i]);
      b_moves[i][(occ * b_magics[i]) >> b_shift[i]]= get_bishop_atk(i, occ);
    }
    for(x = 0ULL; x < (1ULL << r_bits[i]); x++)
    { occ = get_blockers(x, r_mask[i]);
      r_moves[i][(occ * r_magics[i]) >> r_shift[i]] = get_rook_atk(i, occ);
    }
  }
}

void init_distance()
{
  int i,j,f[2],r[2];
	
  for(i = 0; i < 64; i++)
  { f[0] = File(i);
    r[0] = Rank(i);
    for(j = 0; j < 64; j++)
    { f[1] = File(j);
      r[1] = Rank(j);
      distance[i][j] = max(abs(r[0]-r[1]), abs(f[0]-f[1]));
    }
  }
}

void init_movegen()
{ init_distance();
  init_sliders();
}

void init_direction()
{
  int i, j, next;
  bitboard_t m1, m2;
  bitboard_t diag;
  bitboard_t trace;
  bitboard_t rank;
  bitboard_t result;
  
  memset(direction, 0, sizeof(direction));
  for(i = 0; i < 64; i++)
  { rank = 0xFFULL << (8 * (i / 8));
    for(j = 0; j < 64; j++)
    { if(i == j) continue;
      ///orthogonals:
      if(File(i) == File(j) || Rank(i) == Rank(j))
      { m1 = rmoves(i, 1ULL << j);
        m2 = rmoves(j, 1ULL << i);
        direction[i][j] = m1 & m2;
        atk_flags[i][j] = R_ATK;
      }
      else ///diagonals:
      { next = 0;
        //up-right:
        result = 0ULL;
        for(diag = trace = 1ULL << i; diag; diag <<= 9, trace <<= 1)
        { if(diag && (trace & rank))
          { result |= diag;
            if(result & (1ULL << j))
            { direction[i][j] = result & ~((1ULL << i) | (1ULL << j));
              atk_flags[i][j] = B_ATK;
              next = 1;
              break;
            }
          }
          else break;
        }
        //up-left
        if(next) continue;
        result = 0ULL;
        for(diag = trace = 1ULL << i; diag; diag <<= 7, trace >>= 1)
        { if(diag && (trace & rank))
          { result |= diag;
            if(result & (1ULL << j))
            { direction[i][j] = result & ~((1ULL << i) | (1ULL << j));
              atk_flags[i][j] = B_ATK;
              next = 1;
              break;
            }
          }
          else break;
        }
        //down-right:
        if(next) continue;
        result = 0ULL;
        for(diag = trace = 1ULL << i; diag; diag >>= 7, trace <<= 1)
        { if(diag && (trace & rank))
          { result |= diag;
            if(result & (1ULL << j))
            { direction[i][j] = result & ~((1ULL << i) | (1ULL << j));
              atk_flags[i][j] = B_ATK;
              next = 1;
              break;
            }
          }
          else break;
        }
        //down-left:
        if(next) continue;
        result = 0ULL;
        for(diag = trace = 1ULL << i; diag; diag >>= 9, trace >>= 1)
        { if(diag && (trace & rank))
          { result |= diag;
            if(result & (1ULL << j))
            { direction[i][j] = result & ~((1ULL << i) | (1ULL << j));
              atk_flags[i][j] = B_ATK;
              break;
            }
          }
          else break;
        }
      }
    }
  }
}


//magic multipliers generation
#define MAX_TRIALS 10000

void print_magics(uint64 *m)
{
  int i,j;
  FILE *f;
	
  f = fopen("magics.txt", "a");
  j = 1;
  for(i = 0; i < 64; i++)
  { fprintf(f, "\t%#.16llx%s, ", m[i],"ULL");
    if(++j > 4)
    { j = 1;
      fprintf(f, "%s", "\n");
    }
  }	
  fprintf(f, "%s", "\n\n");
  fclose(f);
}

uint64 rand64_1bits(int bitcount)
{
  int i;
  uint64 p, rnum = 0ULL;

  for(i = 0; i < bitcount; i++)
  { for(;;)
    { p = 1ULL << (rand() % 63);
      if(!(p & rnum))
      { rnum |= p;
        break;
      }
    }
  }
  return (rnum);
}

uint64 gen_multiplier(int sq, bitboard_t mask, int bits, int *mbits, int *ibits, int rook)
{
  int fail,b;
  int trials;
  int bit_trials;
  uint64 magic;
  uint64 index;
  bitboard_t x, blockers[4096], solution[4096], used[4096];
	
  for(x = 0ULL; x < (1ULL << bits); x++)
  { blockers[x] = get_blockers(x, mask);
    solution[x] = (rook) ? (get_rook_atk(sq,blockers[x])) : 
      (get_bishop_atk(sq, blockers[x]));
  }
	
  trials = 0; bit_trials = 1;
  for(;;)
  { ibits[sq] = 0;
    magic = rand64_1bits(bit_trials);
    for(x = 0ULL; x < (1ULL << bits); x++) used[x] = 0;
    fail = 0;
    for(x = 0ULL; x < (1ULL << bits); x++)
    { index = (blockers[x] * magic) >> (64 - bits);
      if(used[index] == 0) 
      { used[index] = solution[x];
        b = popcnt(blockers[x] * magic);
        if(ibits[sq] < b) ibits[sq] = b;
      }
      else if(used[index] != solution[x])
      { fail = 1;
        break;
      }
    }
    if(!fail)
    { mbits[sq] = bit_trials;
      return magic;
    }
    trials++;
    if((trials > MAX_TRIALS) && (bit_trials < bits))
    { bit_trials++;
      trials = 0;
    }
  }
}

void generate_magics()
{
  int i,j,h,m,s;
  uint64 magics[64];
  int magic_bits[64];
  int index_bits[64];
  time_t t1,t2;

  srand((uint32)time(0));
  time(&t1);
	
  printf("generating magic numbers\n\nprogress:\n");
  for(i = 0; i < 64; i++)
  { magics[i] = gen_multiplier(i, b_mask[i], b_bits[i], magic_bits, index_bits, 0);
    printf(".");
  }
  print_magics(magics);
  printf("\n\nused bits in bishop magic numbers by %d trials:\n\n",MAX_TRIALS);
  for(i = 0,j = 1; i < 64; i++,j++)
  { printf("%d, ",magic_bits[i]);
    if(j >= 8) 
    { printf("\n");
      j = 0;
    }
  }
  printf("\n\nmax. multiplication bits for bishop by %d trials:\n\n",MAX_TRIALS);
  for(i = 0,j = 1; i < 64; i++,j++)
  { printf("%d, ",index_bits[i]);
    if(j >= 8) 
    { printf("\n");
      j = 0;
    }
  }	
  printf("\n\nprogress:\n");
  for(i = 0; i < 64; i++)
  { magics[i] = gen_multiplier(i, r_mask[i], r_bits[i], magic_bits, index_bits, 1);
    printf(".");
  }
  print_magics(magics);
  printf("\n\nused bits in rook magic numbers by %d trials:\n\n",MAX_TRIALS);
  for(i = 0,j = 1; i < 64; i++,j++)
  { printf("%d, ",magic_bits[i]);
    if(j >= 8) 
    { printf("\n");
      j = 0;
    }
  }
  printf("\n\nmax. multiplication bits for rook by %d trials:\n\n",MAX_TRIALS);
  for(i = 0,j = 1; i < 64; i++,j++)
  { printf("%d, ",index_bits[i]);
    if(j >= 8) 
    { printf("\n");
      j = 0;
    }
  }
  printf("\n\n");
	
  time(&t2);
  s = (int)(t2-t1); 
  h = (s / 3600); m = (s / 60) % 60; s %= 60;
  printf("time: %dh:%dm:%ds\n\n", h, m, s);
}
