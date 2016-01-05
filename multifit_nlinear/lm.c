/* multifit_nlinear/lm.c
 * 
 * Copyright (C) 2014, 2015 Patrick Alken
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>

#include "oct.h"

/*
 * This module contains an implementation of the Levenberg-Marquardt
 * algorithm for nonlinear optimization problems. This implementation
 * closely follows the following works:
 *
 * [1] H. B. Nielsen, K. Madsen, Introduction to Optimization and
 *     Data Fitting, Informatics and Mathematical Modeling,
 *     Technical University of Denmark (DTU), 2010.
 *
 * [2] J. J. More, The Levenberg-Marquardt Algorithm: Implementation
 *     and Theory, Lecture Notes in Mathematics, v630, 1978.
 */

typedef struct
{
  size_t n;                  /* number of observations */
  size_t p;                  /* number of parameters */
  gsl_vector *diag;          /* D = diag(J^T J) */
  gsl_vector *x_trial;       /* trial parameter vector */
  gsl_vector *f_trial;       /* trial function vector */
  gsl_vector *workp;         /* workspace, length p */
  gsl_vector *tau;           /* Householder scalars for QR */
  gsl_vector *fvv;           /* D_v^2 f(x), size n */
  gsl_vector *vel;           /* geodesic velocity (standard LM step), size p */
  gsl_vector *acc;           /* geodesic acceleration, size p */
  long nu;                   /* nu */
  double lambda;             /* LM parameter lambda */
  double lambda0;            /* initial scale factor for lambda */

  /* normal equations variables */
  gsl_matrix *A;             /* J^T J */
  gsl_matrix *work_JTJ;      /* copy of J^T J */
  gsl_vector *rhs_vel;       /* -J^T f */
  gsl_vector *rhs_acc;       /* -J^T fvv */
  int chol;                  /* Cholesky factorization successful */

  /* QR solver variables */
  gsl_matrix *R;             /* QR factorization of J */
  gsl_matrix *Q;             /* Householder reflectors for J */
  gsl_permutation *perm;     /* permutation matrix */
  gsl_vector *qtf;           /* Q^T f */
  gsl_vector *qtfvv;         /* Q^T fvv */
  gsl_vector *workn;         /* workspace, length n */

  /* tunable parameters */

  int accel;                 /* use geodesic acceleration */
  double accel_alpha;        /* max |a| / |v| */
  int (*init_diag) (const gsl_matrix * J, gsl_vector * diag);
  int (*update_diag) (const gsl_matrix * J, gsl_vector * diag);

  /* solver initialization independent of lambda */
  int (*solver_init) (const gsl_vector * f, const gsl_matrix * J,
                      const gsl_vector * g, void * vstate);

  /* solver initialization with lambda */
  int (*solver_init_lambda) (const double lambda, void * vstate);

  /* solve linear system for geodesic velocity (standard LM step) */
  int (*solver_vel) (gsl_vector * v, void * vstate);

  /* solve linear system for geodesic acceleration */
  int (*solver_acc) (const gsl_matrix * J, const gsl_vector * fvv,
                     gsl_vector * a, void * vstate);
} lm_state_t;

#include "lmdiag.c"
#include "lmmisc.c"
#include "lmsolve.c"

#define LM_ONE_THIRD         (0.333333333333333)

static void * lm_alloc (const gsl_multifit_nlinear_parameters * params,
                        const size_t n, const size_t p);
static void lm_free(void *vstate);
static int lm_init(void *vstate, const gsl_vector * swts,
                   gsl_multifit_nlinear_fdf *fdf, const gsl_vector *x,
                   gsl_vector *f, gsl_matrix *J, gsl_vector *g);
static int lm_iterate(void *vstate, const gsl_vector *swts,
                      gsl_multifit_nlinear_fdf *fdf,
                      gsl_vector *x, gsl_vector *f, gsl_matrix *J,
                      gsl_vector *g, gsl_vector *dx);

static void *
lm_alloc (const gsl_multifit_nlinear_parameters * params,
          const size_t n, const size_t p)
{
  lm_state_t *state;
  
  state = calloc(1, sizeof(lm_state_t));
  if (state == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate lm state", GSL_ENOMEM);
    }

  state->diag = gsl_vector_alloc(p);
  if (state->diag == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for diag", GSL_ENOMEM);
    }

  state->workp = gsl_vector_alloc(p);
  if (state->workp == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for workp", GSL_ENOMEM);
    }

  state->tau = gsl_vector_alloc(p);
  if (state->tau == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for tau", GSL_ENOMEM);
    }

  state->fvv = gsl_vector_alloc(n);
  if (state->fvv == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for fvv", GSL_ENOMEM);
    }

  state->vel = gsl_vector_alloc(p);
  if (state->vel == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for vel", GSL_ENOMEM);
    }

  state->acc = gsl_vector_alloc(p);
  if (state->acc == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for acc", GSL_ENOMEM);
    }

  state->x_trial = gsl_vector_alloc(p);
  if (state->x_trial == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for x_trial", GSL_ENOMEM);
    }

  state->f_trial = gsl_vector_alloc(n);
  if (state->f_trial == NULL)
    {
      GSL_ERROR_NULL ("failed to allocate space for f_trial", GSL_ENOMEM);
    }

  if (params->scale == GSL_MULTIFIT_NLINEAR_SCALE_LEVENBERG)
    {
      state->init_diag = init_diag_levenberg;
      state->update_diag = update_diag_levenberg;
    }
  else if (params->scale == GSL_MULTIFIT_NLINEAR_SCALE_MARQUARDT)
    {
      state->init_diag = init_diag_marquardt;
      state->update_diag = update_diag_marquardt;
    }
  else if (params->scale == GSL_MULTIFIT_NLINEAR_SCALE_MORE)
    {
      state->init_diag = init_diag_more;
      state->update_diag = update_diag_more;
    }
  else
    {
      GSL_ERROR_NULL ("invalid scale parameter", GSL_EINVAL);
    }

  if (params->solver == GSL_MULTIFIT_NLINEAR_SOLVER_NORMAL)
    {
      /* allocate variables specific to normal equations solver */

      state->A = gsl_matrix_alloc(p, p);
      if (state->A == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for A", GSL_ENOMEM);
        }

      state->work_JTJ = gsl_matrix_alloc(p, p);
      if (state->work_JTJ == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for JTJ workspace",
                          GSL_ENOMEM);
        }

      state->rhs_vel = gsl_vector_alloc(p);
      if (state->rhs_vel == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for rhs_vel", GSL_ENOMEM);
        }

      state->rhs_acc = gsl_vector_alloc(p);
      if (state->rhs_acc == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for rhs_acc", GSL_ENOMEM);
        }

      state->solver_init = normal_init;
      state->solver_init_lambda = normal_init_lambda;
      state->solver_vel = normal_solve_vel;
      state->solver_acc = normal_solve_acc;
    }
  else if (params->solver == GSL_MULTIFIT_NLINEAR_SOLVER_QR)
    {
      /* allocate variables specific to QR solver */

      state->R = gsl_matrix_alloc(n, p);
      if (state->R == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for R", GSL_ENOMEM);
        }

      state->Q = gsl_matrix_alloc(n, p);
      if (state->Q == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for Q", GSL_ENOMEM);
        }

      state->qtf = gsl_vector_alloc(n);
      if (state->qtf == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for qtf",
                          GSL_ENOMEM);
        }

      state->qtfvv = gsl_vector_alloc(n);
      if (state->qtfvv == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for qtfvv",
                          GSL_ENOMEM);
        }

      state->perm = gsl_permutation_calloc(p);
      if (state->perm == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for perm",
                          GSL_ENOMEM);
        }

      state->workn = gsl_vector_alloc(n);
      if (state->workn == NULL)
        {
          GSL_ERROR_NULL ("failed to allocate space for workn",
                          GSL_ENOMEM);
        }

      state->solver_init = qr_init;
      state->solver_init_lambda = qr_init_lambda;
      state->solver_vel = qr_solve_vel;
      state->solver_acc = qr_solve_acc;
    }
  else
    {
      GSL_ERROR_NULL ("invalid solver parameter", GSL_EINVAL);
    }

  state->n = n;
  state->p = p;
  state->lambda0 = 1.0e-3;
  state->accel = params->accel;
  state->accel_alpha = params->accel_alpha;

  return state;
}

static void
lm_free(void *vstate)
{
  lm_state_t *state = (lm_state_t *) vstate;

  if (state->A)
    gsl_matrix_free(state->A);

  if (state->diag)
    gsl_vector_free(state->diag);

  if (state->workp)
    gsl_vector_free(state->workp);

  if (state->tau)
    gsl_vector_free(state->tau);

  if (state->fvv)
    gsl_vector_free(state->fvv);

  if (state->vel)
    gsl_vector_free(state->vel);

  if (state->acc)
    gsl_vector_free(state->acc);

  if (state->work_JTJ)
    gsl_matrix_free(state->work_JTJ);

  if (state->x_trial)
    gsl_vector_free(state->x_trial);

  if (state->f_trial)
    gsl_vector_free(state->f_trial);

  if (state->R)
    gsl_matrix_free(state->R);

  if (state->Q)
    gsl_matrix_free(state->Q);

  if (state->qtf)
    gsl_vector_free(state->qtf);

  if (state->qtfvv)
    gsl_vector_free(state->qtfvv);

  if (state->perm)
    gsl_permutation_free(state->perm);

  if (state->workn)
    gsl_vector_free(state->workn);

  if (state->rhs_vel)
    gsl_vector_free(state->rhs_vel);

  if (state->rhs_acc)
    gsl_vector_free(state->rhs_acc);

  free(state);
}

/*
lm_init()
  Initialize LM solver

Inputs: vstate - workspace
        swts   - sqrt(W) vector
        fdf    - user callback functions
        x      - initial parameter values
        f      - (output) f(x) vector
        J      - (output) J(x) matrix
        g      - (output) J(x)' f(x) vector

Return: success/error
*/

static int
lm_init(void *vstate, const gsl_vector *swts,
        gsl_multifit_nlinear_fdf *fdf, const gsl_vector *x,
        gsl_vector *f, gsl_matrix *J, gsl_vector *g)
{
  int status;
  lm_state_t *state = (lm_state_t *) vstate;

  /* evaluate function and Jacobian at x and apply weight transform */
  status = gsl_multifit_nlinear_eval_f(fdf, x, swts, f);
  if (status)
   return status;

  status = gsl_multifit_nlinear_eval_df(fdf, x, f, swts, J);
  if (status)
    return status;

  /* compute g = J^T f */
  gsl_blas_dgemv(CblasTrans, 1.0, J, f, 0.0, g);

  /* initialize diagonal scaling matrix D */
  (state->init_diag)(J, state->diag);

  /* initialize LM parameter lambda */
  lm_init_lambda(J, state);

  gsl_vector_set_zero(state->vel);
  gsl_vector_set_zero(state->acc);

  /* set default parameters */
  state->nu = 2;

  return GSL_SUCCESS;
}

/*
lm_iterate()
  This function performs 1 iteration of the LM algorithm 6.18
from [1]. The algorithm is slightly modified to loop until we
find an acceptable step dx, in order to guarantee that each
function call contains a new input vector x.

Args: vstate - lm workspace
      swts   - data weights (NULL if unweighted)
      fdf    - function and Jacobian pointers
      x      - on input, current parameter vector
               on output, new parameter vector x + dx
      f      - on input, f(x)
               on output, f(x + dx)
      J      - on input, J(x)
               on output, J(x + dx)
      g      - on input, g(x) = J(x)' f(x)
               on output, g(x + dx) = J(x + dx)' f(x + dx)
      dx     - (output only) parameter step vector
               dx = v + 1/2 a

Return:
1) GSL_SUCCESS if we found a step which reduces the cost
function

2) GSL_ENOPROG if 15 successive attempts were to made to
find a good step without success

Notes:
1) On input, the following must be initialized in state:
nu, lambda

2) On output, the following are updated with the current iterates:
nu, lambda
*/

static int
lm_iterate(void *vstate, const gsl_vector *swts,
           gsl_multifit_nlinear_fdf *fdf, gsl_vector *x,
           gsl_vector *f, gsl_matrix *J, gsl_vector *g,
           gsl_vector *dx)
{
  int status;
  lm_state_t *state = (lm_state_t *) vstate;
  const size_t p = state->p;
  gsl_vector *x_trial = state->x_trial;       /* trial x + dx */
  gsl_vector *f_trial = state->f_trial;       /* trial f(x + dx) */
  gsl_vector *diag = state->diag;             /* diag(D) */
  double rho;                                 /* ratio dF/dL */
  int foundstep = 0;                          /* found step dx */
  int bad_steps = 0;                          /* consecutive rejected steps */
  size_t i;

  /* initialize linear least squares solver */
  status = (state->solver_init)(f, J, g, vstate);
  if (status)
    return status;

  /* loop until we find an acceptable step dx */
  while (!foundstep)
    {
      /* further solver initialization with current lambda */
      status = (state->solver_init_lambda)(state->lambda, vstate);
      if (status)
        return status;

      /*
       * solve: [    J     ] dx = - [ f ]
       *        [ lambda*D ]        [ 0 ]
       */
      status = (state->solver_vel)(state->vel, vstate);
      if (status)
        return status;

      if (state->accel)
        {
          /* compute geodesic acceleration */
          status = gsl_multifit_nlinear_eval_fvv(fdf, x, state->vel, swts, state->fvv);
          if (status)
            return status;

          status = (state->solver_acc)(J, state->fvv, state->acc, vstate);
          if (status)
            return status;
        }

      /* compute step dx = v + 1/2 a */
      for (i = 0; i < p; ++i)
        {
          double vi = gsl_vector_get(state->vel, i);
          double ai = gsl_vector_get(state->acc, i);
          gsl_vector_set(dx, i, vi + 0.5 * ai);
        }

      /* compute x_trial = x + dx */
      lm_trial_step(x, dx, x_trial);

      /* compute f(x + dx) */
      status = gsl_multifit_nlinear_eval_f(fdf, x_trial, swts, f_trial);
      if (status)
        return status;

      /* determine whether to accept or reject proposed step */
      status = lm_check_step(state->vel, g, f, f_trial, &rho, state);

      if (status == GSL_SUCCESS)
        {
          /* reduction in cost function, step acceptable */

          double b;

          /* update LM parameter */
          b = 2.0 * rho - 1.0;
          b = 1.0 - b*b*b;
          state->lambda *= GSL_MAX(LM_ONE_THIRD, b);
          state->nu = 2;

          /* compute J <- J(x + dx) */
          status = gsl_multifit_nlinear_eval_df(fdf, x_trial, f_trial,
                                                swts, J);
          if (status)
            return status;

          /* update x <- x + dx */
          gsl_vector_memcpy(x, x_trial);

          /* update f <- f(x + dx) */
          gsl_vector_memcpy(f, f_trial);

          /* compute new g = J^T f */
          gsl_blas_dgemv(CblasTrans, 1.0, J, f, 0.0, g);

          /* update scaling matrix D */
          (state->update_diag)(J, diag);

          foundstep = 1;
          bad_steps = 0;
        }
      else
        {
          long nu2;

          /* step did not reduce error, reject step */

          /* if more than 15 consecutive rejected steps, report no progres */
          if (++bad_steps > 15)
            return GSL_ENOPROG;

          state->lambda *= (double) state->nu;
          nu2 = state->nu << 1; /* 2*nu */
          if (nu2 <= state->nu)
            {
              /*
               * nu has wrapped around / overflown, reset lambda and nu
               * to original values and break to force another iteration
               */
              state->nu = 2;
              lm_init_lambda(J, state);

              break;
            }

          state->nu = nu2;
        }
    } /* while (!foundstep) */

  return GSL_SUCCESS;
} /* lm_iterate() */

static const gsl_multifit_nlinear_type lm_type =
{
  "lm",
  lm_alloc,
  lm_init,
  lm_iterate,
  lm_free
};

const gsl_multifit_nlinear_type *gsl_multifit_nlinear_lm = &lm_type;
