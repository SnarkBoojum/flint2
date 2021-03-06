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

    Copyright (C) 2014 Abhinav Baid

******************************************************************************/

#include "mpf_mat.h"

void
mpf_mat_gso(mpf_mat_t B, const mpf_mat_t A)
{
    slong i, j, k, flag;
    mpf_t t, s, tmp, eps;
    mp_bitcnt_t exp;

    if (B->r != A->r || B->c != A->c)
    {
        flint_printf("Exception (mpf_mat_gso). Incompatible dimensions.\n");
        abort();
    }

    if (B == A)
    {
        mpf_mat_t T;
        mpf_mat_init(T, A->r, A->c, B->prec);
        mpf_mat_gso(T, A);
        mpf_mat_swap(B, T);
        mpf_mat_clear(T);
        return;
    }

    if (A->r == 0)
    {
        return;
    }

    mpf_init2(t, B->prec);
    mpf_init2(s, B->prec);
    mpf_init2(tmp, B->prec);
    mpf_init2(eps, B->prec);
    exp = ceil((A->prec) / 64.0) * 64;
    mpf_set_ui(eps, 1);
    mpf_div_2exp(eps, eps, exp);

    for (k = 0; k < A->c; k++)
    {
        for (j = 0; j < A->r; j++)
        {
            mpf_set(mpf_mat_entry(B, j, k), mpf_mat_entry(A, j, k));
        }
        flag = 1;
        while (flag)
        {
            mpf_set_ui(t, 0);
            for (i = 0; i < k; i++)
            {
                mpf_set_ui(s, 0);
                for (j = 0; j < A->r; j++)
                {
                    mpf_mul(tmp, mpf_mat_entry(B, j, i),
                            mpf_mat_entry(B, j, k));
                    mpf_add(s, s, tmp);
                }
                mpf_mul(tmp, s, s);
                mpf_add(t, t, tmp);
                for (j = 0; j < A->r; j++)
                {
                    mpf_mul(tmp, s, mpf_mat_entry(B, j, i));
                    mpf_sub(mpf_mat_entry(B, j, k), mpf_mat_entry(B, j, k),
                            tmp);
                }
            }
            mpf_set_ui(s, 0);
            for (j = 0; j < A->r; j++)
            {
                mpf_mul(tmp, mpf_mat_entry(B, j, k), mpf_mat_entry(B, j, k));
                mpf_add(s, s, tmp);
            }
            mpf_add(t, t, s);
            flag = 0;
            if (mpf_cmp(s, t) < 0)
            {
                if (mpf_cmp(s, eps) < 0)
                    mpf_set_ui(s, 0);
                else
                    flag = 1;
            }
        }
        mpf_sqrt(s, s);
        if (mpf_cmp_ui(s, 0) != 0)
            mpf_ui_div(s, 1, s);
        for (j = 0; j < A->r; j++)
        {
            mpf_mul(mpf_mat_entry(B, j, k), mpf_mat_entry(B, j, k), s);
        }
    }
    mpf_clears(t, s, tmp, eps, '\0');
}
