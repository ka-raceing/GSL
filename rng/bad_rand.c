#include <config.h>
#include <stdlib.h>
#include <gsl_rng.h>

/* This is the old BSD rand() generator. The sequence is

   x_{n+1} = (a x_n + c) mod m 

   with a = 1103515245, c = 12345 and m = 2^31 = 2147483648. The seed
   specifies the initial value, x_1.

   The theoretical value of x_{10001} is 1910041713.

   The period of this generator is 2^31.

   The rand() generator is not very good -- the low bits of successive
   numbers are correlated. */

unsigned long int bad_rand_get (void *vstate);
void bad_rand_set (void *state, unsigned long int s);

static const unsigned long int m = 2147483648UL;
static const long int a = 1103515245;
static const long int c = 12345;

typedef struct
  {
    unsigned long int x;
  }
bad_rand_state_t;

unsigned long int
bad_rand_get (void *vstate)
{
  bad_rand_state_t *state = (bad_rand_state_t *) vstate;

  /* The following line relies on unsigned 32-bit arithmetic */

  state->x = (a * state->x + c) & 0x7fffffffUL;

  return state->x;
}


void
bad_rand_set (void *vstate, unsigned long int s)
{
  bad_rand_state_t *state = (bad_rand_state_t *) vstate;

  state->x = s;

  return;
}

static const gsl_rng_type bad_rand_type =
{"bad-rand",			/* name */
 0x7fffffffUL,			/* RAND_MAX */
 0,         			/* RAND_MIN */
 sizeof (bad_rand_state_t),
 &bad_rand_set,
 &bad_rand_get};

const gsl_rng_type *gsl_rng_bad_rand = &bad_rand_type;
