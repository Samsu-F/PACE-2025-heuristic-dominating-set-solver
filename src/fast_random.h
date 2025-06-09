#ifndef _FAST_RANDOM_H
#define _FAST_RANDOM_H


#include <stdint.h>



#define FAST_RANDOM_MAX UINT64_MAX


typedef struct {
    uint64_t state;
} fast_random_t;


static inline void fast_random_init(fast_random_t* rng, uint64_t seed)
{
    rng->state = seed;
}


// Linear congruential generator (a.k.a. mixed congruential generator).
// Mediocre quality randomness but very fast.
static inline uint64_t fast_random(fast_random_t* rng)
{
    /*
    The factor below was taken from the paper
    Steele GL, Vigna S. Computationally easy, spectrally good multipliers for
    congruential pseudorandom number generators. Softw Pract Exper. 2021;1-16.
    doi: 10.1002/spe.3030
    */
    rng->state = 0xd1342543de82ef95ULL * rng->state + 1ULL;
    return rng->state;
}



#endif
