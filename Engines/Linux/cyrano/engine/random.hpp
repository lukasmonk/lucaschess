
// random.h

#ifndef RANDOM_H
#define RANDOM_H

// constants

const int RandomNb = 781;

// macros

#define RANDOM_64(n) (Random64[(n)])

// "constants"

extern const U64 Random64[RandomNb];

// functions

extern void   random_init ();
extern U64 random_64   (int n);

#endif // !defined RANDOM_H

// end of random.h

