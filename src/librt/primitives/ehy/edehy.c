/*                         E D E H Y . C
 * BRL-CAD
 *
 * Copyright (c) 1996-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file primitives/edehy.c
 *
 */

#include "common.h"

#include <math.h>
#include <string.h>

#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"
#include "wdb.h"

#include "../edit_private.h"

#define ECMD_EHY_H		20053
#define ECMD_EHY_R1		20054
#define ECMD_EHY_R2		20055
#define ECMD_EHY_C		20056

void
rt_edit_ehy_set_edit_mode(struct rt_edit *s, int mode)
{
    rt_edit_set_edflag(s, mode);

    switch (mode) {
	case ECMD_EHY_H:
	case ECMD_EHY_R1:
	case ECMD_EHY_R2:
	case ECMD_EHY_C:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    break;
	default:
	    break;
    };

    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}

static void
ehy_ed(struct rt_edit *s, int arg, int UNUSED(a), int UNUSED(b), void *UNUSED(data))
{
    rt_edit_ehy_set_edit_mode(s, arg);
}

struct rt_edit_menu_item ehy_menu[] = {
    { "EHY MENU", NULL, 0 },
    { "Set H", ehy_ed, ECMD_EHY_H },
    { "Set A", ehy_ed, ECMD_EHY_R1 },
    { "Set B", ehy_ed, ECMD_EHY_R2 },
    { "Set c", ehy_ed, ECMD_EHY_C },
    { "", NULL, 0 }
};

struct rt_edit_menu_item *
rt_edit_ehy_menu_item(const struct bn_tol *UNUSED(tol))
{
    return ehy_menu;
}

#define V3BASE2LOCAL(_pt) (_pt)[X]*base2local, (_pt)[Y]*base2local, (_pt)[Z]*base2local

void
rt_edit_ehy_write_params(
	struct bu_vls *p,
       	const struct rt_db_internal *ip,
       	const struct bn_tol *UNUSED(tol),
	fastf_t base2local)
{
    struct rt_ehy_internal *ehy = (struct rt_ehy_internal *)ip->idb_ptr;
    RT_EHY_CK_MAGIC(ehy);

    bu_vls_printf(p, "Vertex: %.9f %.9f %.9f\n", V3BASE2LOCAL(ehy->ehy_V));
    bu_vls_printf(p, "Height: %.9f %.9f %.9f\n", V3BASE2LOCAL(ehy->ehy_H));
    bu_vls_printf(p, "Semi-major axis: %.9f %.9f %.9f\n", V3ARGS(ehy->ehy_Au));
    bu_vls_printf(p, "Semi-major length: %.9f\n", ehy->ehy_r1 * base2local);
    bu_vls_printf(p, "Semi-minor length: %.9f\n", ehy->ehy_r2 * base2local);
    bu_vls_printf(p, "Dist to asymptotes: %.9f\n", ehy->ehy_c * base2local);
}

#define read_params_line_incr \
    lc = (ln) ? (ln + lcj) : NULL; \
    if (!lc) { \
	bu_free(wc, "wc"); \
	return BRLCAD_ERROR; \
    } \
    ln = strchr(lc, tc); \
    if (ln) *ln = '\0'; \
    while (lc && strchr(lc, ':')) lc++

int
rt_edit_ehy_read_params(
	struct rt_db_internal *ip,
	const char *fc,
	const struct bn_tol *UNUSED(tol),
	fastf_t local2base
	)
{
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    struct rt_ehy_internal *ehy = (struct rt_ehy_internal *)ip->idb_ptr;
    RT_EHY_CK_MAGIC(ehy);

    if (!fc)
	return BRLCAD_ERROR;

    // We're getting the file contents as a string, so we need to split it up
    // to process lines. See https://stackoverflow.com/a/17983619

    // Figure out if we need to deal with Windows line endings
    const char *crpos = strchr(fc, '\r');
    int crlf = (crpos && crpos[1] == '\n') ? 1 : 0;
    char tc = (crlf) ? '\r' : '\n';
    // If we're CRLF jump ahead another character.
    int lcj = (crlf) ? 2 : 1;

    char *ln = NULL;
    char *wc = bu_strdup(fc);
    char *lc = wc;

    // Set up initial line
    ln = strchr(lc, tc);
    if (ln) *ln = '\0';

    // Trim off prefixes, if user left them in
    while (lc && strchr(lc, ':')) lc++;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(ehy->ehy_V, a, b, c);
    VSCALE(ehy->ehy_V, ehy->ehy_V, local2base);

    // Set up Height line
    read_params_line_incr;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(ehy->ehy_H, a, b, c);
    VSCALE(ehy->ehy_H, ehy->ehy_H, local2base);

    // Set up Semi-major axis line
    read_params_line_incr;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(ehy->ehy_Au, a, b, c);
    VUNITIZE(ehy->ehy_Au);

    // Set up Semi-major length line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    ehy->ehy_r1 = a * local2base;

    // Set up Semi-minor length line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    ehy->ehy_r2 = a * local2base;

    // Set up distance to asymptotes line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    ehy->ehy_c = a * local2base;

    // Cleanup
    bu_free(wc, "wc");
    return BRLCAD_OK;
}

/* scale height vector H */
void
ecmd_ehy_h(struct rt_edit *s)
{
    struct rt_ehy_internal *ehy =
	(struct rt_ehy_internal *)s->es_int.idb_ptr;

    RT_EHY_CK_MAGIC(ehy);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / MAGNITUDE(ehy->ehy_H);
    }
    VSCALE(ehy->ehy_H, ehy->ehy_H, s->es_scale);
}

/* scale semimajor axis of EHY */
void
ecmd_ehy_r1(struct rt_edit *s)
{
    struct rt_ehy_internal *ehy =
	(struct rt_ehy_internal *)s->es_int.idb_ptr;

    RT_EHY_CK_MAGIC(ehy);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / ehy->ehy_r1;
    }
    if (ehy->ehy_r1 * s->es_scale >= ehy->ehy_r2)
	ehy->ehy_r1 *= s->es_scale;
    else
	bu_log("pscale:  semi-minor axis cannot be longer than semi-major axis!");
}

/* scale semiminor axis of EHY */
void
ecmd_ehy_r2(struct rt_edit *s)
{
    struct rt_ehy_internal *ehy =
	(struct rt_ehy_internal *)s->es_int.idb_ptr;

    RT_EHY_CK_MAGIC(ehy);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / ehy->ehy_r2;
    }
    if (ehy->ehy_r2 * s->es_scale <= ehy->ehy_r1)
	ehy->ehy_r2 *= s->es_scale;
    else
	bu_log("pscale:  semi-minor axis cannot be longer than semi-major axis!");
}

/* scale distance between apex of EHY & asymptotic cone */
void
ecmd_ehy_c(struct rt_edit *s)
{
    struct rt_ehy_internal *ehy =
	(struct rt_ehy_internal *)s->es_int.idb_ptr;

    RT_EHY_CK_MAGIC(ehy);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / ehy->ehy_c;
    }
    ehy->ehy_c *= s->es_scale;
}

static int
rt_edit_ehy_pscale(struct rt_edit *s)
{
    if (s->e_inpara > 1) {
	bu_vls_printf(s->log_str, "ERROR: only one argument needed\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (s->e_inpara) {
	if (s->e_para[0] <= 0.0) {
	    bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	    s->e_inpara = 0;
	    return BRLCAD_ERROR;
	}

	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;
    }

    switch (s->edit_flag) {
	case ECMD_EHY_H:
	    ecmd_ehy_h(s);
	    break;
	case ECMD_EHY_R1:
	    ecmd_ehy_r1(s);
	    break;
	case ECMD_EHY_R2:
	    ecmd_ehy_r2(s);
	    break;
	case ECMD_EHY_C:
	    ecmd_ehy_c(s);
	    break;
    };

    return 0;
}

int
rt_edit_ehy_edit(struct rt_edit *s)
{
    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	    /* scale the solid uniformly about its vertex point */
	    return rt_edit_generic_sscale(s, &s->es_int);
	case RT_PARAMS_EDIT_TRANS:
	    /* translate solid */
	    rt_edit_generic_strans(s, &s->es_int);
	    break;
	case RT_PARAMS_EDIT_ROT:
	    /* rot solid about vertex */
	    rt_edit_generic_srot(s, &s->es_int);
	    break;
	default:
	    return rt_edit_ehy_pscale(s);
    }

    return 0;
}

int
rt_edit_ehy_edit_xy(
        struct rt_edit *s,
        const vect_t mousevec
        )
{
    vect_t pos_view = VINIT_ZERO;       /* Unrotated view space pos */
    struct rt_db_internal *ip = &s->es_int;
    bu_clbk_t f = NULL;
    void *d = NULL;

    switch (s->edit_flag) {
        case RT_PARAMS_EDIT_SCALE:
	case ECMD_EHY_H:
	case ECMD_EHY_R1:
	case ECMD_EHY_R2:
	case ECMD_EHY_C:
            rt_edit_generic_sscale_xy(s, mousevec);
            return 0;
        case RT_PARAMS_EDIT_TRANS:
            rt_edit_generic_strans_xy(&pos_view, s, mousevec);
            rt_update_edit_absolute_tran(s, pos_view);
            return 0;
        case RT_PARAMS_EDIT_ROT:
            bu_vls_printf(s->log_str, "RT_PARAMS_EDIT_ROT XY editing setup unimplemented in %s_edit_xy callback\n", EDOBJ[ip->idb_type].ft_label);
            rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
            if (f)
                (*f)(0, NULL, d, NULL);
            return BRLCAD_ERROR;
        default:
            bu_vls_printf(s->log_str, "%s: XY edit undefined in solid edit mode %d\n", EDOBJ[ip->idb_type].ft_label, s->edit_flag);
            rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
            if (f)
                (*f)(0, NULL, d, NULL);
            return BRLCAD_ERROR;
    }
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
