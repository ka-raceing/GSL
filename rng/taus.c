#include <config.h>
#include <stdlib.h>
#include <gsl_rng.h>
#include <stdio.h>

/* This is a maximally equidistributed combined Tausworthe
   generator. The sequence is,

   x_n = (s1_n ^ s2_n ^ s3_n) 

   s1_{n+1} = (((s1_n & 4294967294) <<12) ^ (((s1_n <<13) ^ s1_n) >>19))
   s2_{n+1} = (((s2_n & 4294967288) << 4) ^ (((s2_n << 2) ^ s2_n) >>25))
   s3_{n+1} = (((s3_n & 4294967280) <<17) ^ (((s3_n << 3) ^ s3_n) >>11))

   computed modulo 2^32. In the three formulas above '^' means
   exclusive-or (C-notation), not exponentiation. Note that the
   algorithm relies on the properties of 32-bit unsigned integers (it
   is formally defined on bit-vectors of length 32). I have added a
   bitmask to make it work on 64 bit machines.

   We initialize the generator with s1_1 .. s3_1 = s_n MOD m, where
   s_n = (69069 * s_{n-1}) mod 2^32, and s_0 = s is the user-supplied
   seed.

   The theoretical value of x_{10007} is 2733957125. The subscript
   10007 means (1) seed the generator with s=1 (2) do six warm-up
   iterations, (3) then do 10000 actual iterations.

   The period of this generator is about 2^88.

   From: P. L'Ecuyer, "Maximally Equidistributed Combined Tausworthe
   Generators", Mathematics of Computation, 65, 213 (1996), 203--213.

   This is available on the net from L'Ecuyer's home page,

   http://www.iro.umontreal.ca/~lecuyer/myftp/papers/tausme.ps
   ftp://ftp.iro.umontreal.ca/pub/simulation/lecuyer/papers/tausme.ps */

unsigned long int taus_get (void *vstate);
void taus_set (void *state, unsigned long int s);

typedef struct
  {
    unsigned long int s1, s2, s3;
  }
taus_state_t;

unsigned long
taus_get (void *vstate)
{
  taus_state_t *state = (taus_state_t *) vstate;

#define MASK 0xffffffffUL
#define TAUSWORTHE(s,a,b,c,d) (((s &c) <<d) &MASK) ^ ((((s <<a) &MASK)^s) >>b)

  state->s1 = TAUSWORTHE (state->s1, 13, 19, 4294967294UL, 12);
  state->s2 = TAUSWORTHE (state->s2, 2, 25, 4294967288UL, 4);
  state->s3 = TAUSWORTHE (state->s3, 3, 11, 4294967280UL, 17);

  return (state->s1 ^ state->s2 ^ state->s3);
}

void
taus_set (void *vstate, unsigned long int s)
{
  taus_state_t *state = (taus_state_t *) vstate;

  if (s == 0)
    s = 1;	/* default seed is 1 */

#define LCG(n) ((69069 * n) & 0xffffffffUL)
  state->s1 = LCG (s);
  state->s2 = LCG (state->s1);
  state->s3 = LCG (state->s2);

  /* "warm it up" */
  taus_get (state);
  taus_get (state);
  taus_get (state);
  taus_get (state);
  taus_get (state);
  taus_get (state);
  return;
}

static const gsl_rng_type taus_type =
{"taus",			/* name */
 0xffffffffUL,			/* RAND_MAX */
 0,			        /* RAND_MIN */
 sizeof (taus_state_t),
 &taus_set,
 &taus_get};

const gsl_rng_type *gsl_rng_taus = &taus_type;
