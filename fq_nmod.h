/*=============================================================================

    This file is part of FLINT.

    FLINT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLINT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLINT; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

=============================================================================*/
/******************************************************************************

    Copyright (C) 2011 Sebastian Pancratz
    Copyright (C) 2012 Andres Goens
    Copyright (C) 2013 Mike Hansen

******************************************************************************/

#ifndef FQ_NMOD_H
#define FQ_NMOD_H

#undef ulong                /* interferes with system includes */
#include <stdlib.h>
#include <stdio.h>
#define ulong unsigned long

#include "nmod_poly.h"
#include "ulong_extras.h"

/* Data types and context ****************************************************/

typedef nmod_poly_t fq_nmod_t;
typedef nmod_poly_struct fq_nmod_struct;

typedef struct
{
    fmpz p;
    nmod_t mod;

    int sparse_modulus;

    mp_limb_t *a;
    long *j;
    long len;

    nmod_poly_t modulus;
    nmod_poly_t inv;

    char *var;
}
fq_nmod_ctx_struct;

typedef fq_nmod_ctx_struct fq_nmod_ctx_t[1];

void fq_nmod_ctx_init(fq_nmod_ctx_t ctx,
                      const fmpz_t p, long d, const char *var);

int _fq_nmod_ctx_init_conway(fq_nmod_ctx_t ctx,
                             const fmpz_t p, long d, const char *var);

void fq_nmod_ctx_init_conway(fq_nmod_ctx_t ctx,
                             const fmpz_t p, long d, const char *var);

void fq_nmod_ctx_init_modulus(fq_nmod_ctx_t ctx,
                              const fmpz_t p, long d, const nmod_poly_t modulus,
                              const char *var);

void fq_nmod_ctx_randtest(fq_nmod_ctx_t ctx, flint_rand_t state);

void fq_nmod_ctx_clear(fq_nmod_ctx_t ctx);

static __inline__ long fq_nmod_ctx_degree(const fq_nmod_ctx_t ctx)
{
    return ctx->modulus->length - 1;
}

#define fq_nmod_ctx_prime(ctx)  (&((ctx)->p))

static __inline__ void fq_nmod_ctx_order(fmpz_t f, const fq_nmod_ctx_t ctx)
{
    fmpz_set(f, fq_nmod_ctx_prime(ctx));
    fmpz_pow_ui(f, f, fq_nmod_ctx_degree(ctx));
}

/* TODO */
static __inline__ int fq_nmod_ctx_fprint(FILE * file, const fq_nmod_ctx_t ctx)
{
    int r;
    long i, k;

    r = fprintf(file, "p = ");
    if (r <= 0)
        return r;

    r = fmpz_fprint(file, fq_nmod_ctx_prime(ctx));
    if (r <= 0)
        return r;

    r = fprintf(file, "\nd = %ld\nf(X) = ", ctx->j[ctx->len - 1]);
    if (r <= 0)
        return r;

    r = fprintf(file, "%lu", ctx->a[0]);
    if (r <= 0)
        return r;

    for (k = 1; k < ctx->len; k++)
    {
        i = ctx->j[k];
        r = fprintf(file, " + ");
        if (r <= 0)
            return r;

        if (ctx->a[k] == 1UL)
        {
            if (i == 1)
                r = fprintf(file, "X");
            else
                r = fprintf(file, "X^%ld", i);
            if (r <= 0)
                return r;
        }
        else
        {
            r = fprintf(file, "%lu", ctx->a[k]);
            if (r <= 0)
                return r;

            if (i == 1)
                r = fprintf(file, "*X");
            else
                r = fprintf(file, "*X^%ld", i);
            if (r <= 0)
                return r;
        }
    }
    r = fprintf(file, "\n");
    return r;
}

static __inline__ void fq_nmod_ctx_print(const fq_nmod_ctx_t ctx)
{
    fq_nmod_ctx_fprint(stdout, ctx);
}

/* Memory managment  *********************************************************/

static __inline__ void fq_nmod_init(fq_nmod_t rop, const fq_nmod_ctx_t ctx)
{
    nmod_poly_init2_preinv(rop, ctx->mod.n, ctx->mod.ninv, fq_nmod_ctx_degree(ctx));
}

static __inline__ void fq_nmod_init2(fq_nmod_t rop, const fq_nmod_ctx_t ctx)
{
    nmod_poly_init2_preinv(rop, ctx->mod.n, ctx->mod.ninv, fq_nmod_ctx_degree(ctx));
}

static __inline__ void fq_nmod_clear(fq_nmod_t rop, const fq_nmod_ctx_t ctx)
{
    nmod_poly_clear(rop);
}

static __inline__ 
void _fq_nmod_sparse_reduce(mp_limb_t *R, long lenR, const fq_nmod_ctx_t ctx)
{
    const long d = ctx->j[ctx->len - 1];

    NMOD_VEC_NORM(R, lenR);

    if (lenR > d)
    {
        long i, k;

        for (i = lenR - 1; i >= d; i--)
        {
            for (k = ctx->len - 2; k >= 0; k--)
            {
                
                R[ctx->j[k] + i - d] = n_submod(R[ctx->j[k] + i - d],
                                                n_mulmod2_preinv(R[i], ctx->a[k], ctx->mod.n, ctx->mod.ninv),
                                                ctx->mod.n);
            }
            R[i] = 0UL;
        }
    }
}

static __inline__ void _fq_nmod_dense_reduce(mp_limb_t* R, long lenR, const fq_nmod_ctx_t ctx)
{
    mp_limb_t  *q, *r;

    if (lenR < ctx->modulus->length)
    {
        _nmod_vec_reduce(R, R, lenR, ctx->mod);
        return;
    }
    
    q = _nmod_vec_init(lenR - ctx->modulus->length + 1);
    r = _nmod_vec_init(ctx->modulus->length - 1);

    _nmod_poly_divrem_newton21_preinv(q, r, R, lenR, 
                                      ctx->modulus->coeffs, ctx->modulus->length,
                                      ctx->inv->coeffs, ctx->inv->length,
                                      ctx->mod);

    _nmod_vec_set(R, r, ctx->modulus->length - 1);
    _nmod_vec_clear(q);
    _nmod_vec_clear(r);

}

static __inline__ void _fq_nmod_reduce(mp_limb_t* R, long lenR, const fq_nmod_ctx_t ctx)
{
    if (ctx->sparse_modulus)
        _fq_nmod_sparse_reduce(R, lenR, ctx);
    else
        _fq_nmod_dense_reduce(R, lenR, ctx);    
}

static __inline__ void fq_nmod_reduce(fq_nmod_t rop, const fq_nmod_ctx_t ctx)
{
    _fq_nmod_reduce(rop->coeffs, rop->length, ctx);
    rop->length = FLINT_MIN(rop->length, ctx->modulus->length - 1);
    _nmod_poly_normalise(rop);
}

/* Basic arithmetic **********************************************************/

void fq_nmod_add(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_t op2, const fq_nmod_ctx_t ctx);

void fq_nmod_sub(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_t op2, const fq_nmod_ctx_t ctx);

void fq_nmod_sub_one(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_ctx_t ctx);

void fq_nmod_neg(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_ctx_t ctx);

void fq_nmod_mul(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_t op2, const fq_nmod_ctx_t ctx);

void fq_nmod_mul_fmpz(fq_nmod_t rop, const fq_nmod_t op, const fmpz_t x, const fq_nmod_ctx_t ctx);

void fq_nmod_mul_si(fq_nmod_t rop, const fq_nmod_t op, long x, const fq_nmod_ctx_t ctx);

void fq_nmod_mul_ui(fq_nmod_t rop, const fq_nmod_t op, ulong x, const fq_nmod_ctx_t ctx);

void fq_nmod_sqr(fq_nmod_t rop, const fq_nmod_t op, const fq_nmod_ctx_t ctx);

void fq_nmod_inv(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_ctx_t ctx);

void _fq_nmod_pow(mp_limb_t *rop, const mp_limb_t *op, long len, const fmpz_t e, const fq_nmod_ctx_t ctx);

void fq_nmod_pow(fq_nmod_t rop, const fq_nmod_t op1, const fmpz_t e, const fq_nmod_ctx_t ctx);

void fq_nmod_pow_ui(fq_nmod_t rop, const fq_nmod_t op1, const ulong e, const fq_nmod_ctx_t ctx);

void
fq_nmod_pth_root(fq_nmod_t rop, const fq_nmod_t op1, const fq_nmod_ctx_t ctx);

/* Randomisation *************************************************************/

void fq_nmod_randtest(fq_nmod_t rop, flint_rand_t state, const fq_nmod_ctx_t ctx);

void fq_nmod_randtest_dense(fq_nmod_t rop, flint_rand_t state, const fq_nmod_ctx_t ctx);

void fq_nmod_randtest_not_zero(fq_nmod_t rop, flint_rand_t state, const fq_nmod_ctx_t ctx);

/* Comparison ****************************************************************/

static __inline__ int fq_nmod_equal(const fq_nmod_t op1, const fq_nmod_t op2,
                                    const fq_nmod_ctx_t ctx)
{
    return nmod_poly_equal(op1, op2);
}

static __inline__ int fq_nmod_is_zero(const fq_nmod_t op, const fq_nmod_ctx_t ctx)
{
    return nmod_poly_is_zero(op);
}

static __inline__ int fq_nmod_is_one(const fq_nmod_t op, const fq_nmod_ctx_t ctx)
{
    return nmod_poly_is_one(op);
}

/* Assignments and conversions ***********************************************/

static __inline__ void fq_nmod_set(fq_nmod_t rop, const fq_nmod_t op,
                                   const fq_nmod_ctx_t ctx)
{
    nmod_poly_set(rop, op);
}

static __inline__ void fq_nmod_set_fmpz(fq_nmod_t rop, const fmpz_t x, const fq_nmod_ctx_t ctx)
{
    fmpz_t rx;
    fmpz_init(rx);
    fmpz_mod(rx, x, fq_nmod_ctx_prime(ctx));
    nmod_poly_zero(rop);
    nmod_poly_set_coeff_ui(rop, 0, fmpz_get_ui(rx));
    fmpz_clear(rx);
}

static __inline__ void fq_nmod_set_ui(fq_nmod_t rop, const ulong x, const fq_nmod_ctx_t ctx)
{
    nmod_poly_zero(rop);
    nmod_poly_set_coeff_ui(rop, 0, n_mod2_preinv(x, ctx->mod.n, ctx->mod.ninv));
}

static __inline__ void fq_nmod_swap(fq_nmod_t op1, fq_nmod_t op2,
                                    const fq_nmod_ctx_t ctx)
{
    nmod_poly_swap(op1, op2);
}

static __inline__ void fq_nmod_zero(fq_nmod_t rop,  const fq_nmod_ctx_t ctx)
{
    nmod_poly_zero(rop);
}

static __inline__ void fq_nmod_one(fq_nmod_t rop,  const fq_nmod_ctx_t ctx)
{
    nmod_poly_one(rop);
}

static __inline__ void fq_nmod_gen(fq_nmod_t rop, const fq_nmod_ctx_t ctx)
{
    nmod_poly_zero(rop);
    nmod_poly_set_coeff_ui(rop, 0, 0);
    nmod_poly_set_coeff_ui(rop, 1, 1);
}

/* Output ********************************************************************/

static __inline__ 
int fq_nmod_fprint(FILE * file, const fq_nmod_t op, const fq_nmod_ctx_t ctx)
{
    return nmod_poly_fprint(file, op);
}

static __inline__ 
void fq_nmod_print(const fq_nmod_t op, const fq_nmod_ctx_t ctx)
{
    nmod_poly_print(op);
}

/* TODO: Make nmod_poly_fprint_pretty */
static __inline__ 
int fq_nmod_fprint_pretty(FILE * file, const fq_nmod_t op, const fq_nmod_ctx_t ctx)
{
    return nmod_poly_fprint(file, op);
}

/* TODO: Make nmod_poly_print_pretty */
static __inline__ 
void fq_nmod_print_pretty(const fq_nmod_t op, const fq_nmod_ctx_t ctx)
{
    nmod_poly_print(op);
}

char *
fq_nmod_get_str(const fq_nmod_t op, const fq_nmod_ctx_t ctx);

char *
fq_nmod_get_str_pretty(const fq_nmod_t op, const fq_nmod_ctx_t ctx);

/* Special functions *********************************************************/

void _fq_nmod_trace(fmpz_t rop, const mp_limb_t *op, long len, 
                    const fq_nmod_ctx_t ctx);

void fq_nmod_trace(fmpz_t rop, const fq_nmod_t op, const fq_nmod_ctx_t ctx);

void _fq_nmod_frobenius(mp_limb_t *rop, const mp_limb_t *op, long len, long e, 
                        const fq_nmod_ctx_t ctx);

void fq_nmod_frobenius(fq_nmod_t rop, const fq_nmod_t op, long e, const fq_nmod_ctx_t ctx);

void _fq_nmod_norm(fmpz_t rop, const mp_limb_t *op, long len, 
                   const fq_nmod_ctx_t ctx);

void fq_nmod_norm(fmpz_t rop, const fq_nmod_t op, const fq_nmod_ctx_t ctx);

/* Bit packing ******************************************************/

void
fq_nmod_bit_pack(fmpz_t f, const fq_nmod_t op, mp_bitcnt_t bit_size,
                 const fq_nmod_ctx_t ctx);

void
fq_nmod_bit_unpack(fq_nmod_t rop, const fmpz_t f, mp_bitcnt_t bit_size,
                   const fq_nmod_ctx_t ctx);

#endif

