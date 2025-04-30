/*                         E D E P A . C
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
/** @file primitives/edepa.c
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

#define ECMD_EPA_H		19050
#define ECMD_EPA_R1		19051
#define ECMD_EPA_R2		19052

void
rt_solid_edit_epa_set_edit_mode(struct rt_solid_edit *s, int mode)
{
    rt_solid_edit_set_edflag(s, mode);

    switch (mode) {
	case ECMD_EPA_H:
	case ECMD_EPA_R1:
	case ECMD_EPA_R2:
	    s->solid_edit_mode = RT_SOLID_EDIT_SCALE;
	    break;
	default:
	    break;
    };


    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_solid_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}

static void
epa_ed(struct rt_solid_edit *s, int arg, int UNUSED(a), int UNUSED(b), void *UNUSED(data))
{
    rt_solid_edit_epa_set_edit_mode(s, arg);
}

struct rt_solid_edit_menu_item epa_menu[] = {
    { "EPA MENU", NULL, 0 },
    { "Set H", epa_ed, ECMD_EPA_H },
    { "Set A", epa_ed, ECMD_EPA_R1 },
    { "Set B", epa_ed, ECMD_EPA_R2 },
    { "", NULL, 0 }
};

struct rt_solid_edit_menu_item *
rt_solid_edit_epa_menu_item(const struct bn_tol *UNUSED(tol))
{
    return epa_menu;
}

#define V3BASE2LOCAL(_pt) (_pt)[X]*base2local, (_pt)[Y]*base2local, (_pt)[Z]*base2local

void
rt_solid_edit_epa_write_params(
	struct bu_vls *p,
       	const struct rt_db_internal *ip,
       	const struct bn_tol *UNUSED(tol),
	fastf_t base2local)
{
    struct rt_epa_internal *epa = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(epa);

    bu_vls_printf(p, "Vertex: %.9f %.9f %.9f\n", V3BASE2LOCAL(epa->epa_V));
    bu_vls_printf(p, "Height: %.9f %.9f %.9f\n", V3BASE2LOCAL(epa->epa_H));
    bu_vls_printf(p, "Semi-major axis: %.9f %.9f %.9f\n", V3ARGS(epa->epa_Au));
    bu_vls_printf(p, "Semi-major length: %.9f\n", epa->epa_r1 * base2local);
    bu_vls_printf(p, "Semi-minor length: %.9f\n", epa->epa_r2 * base2local);
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
rt_solid_edit_epa_read_params(
	struct rt_db_internal *ip,
	const char *fc,
	const struct bn_tol *UNUSED(tol),
	fastf_t local2base
	)
{
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    struct rt_epa_internal *epa = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(epa);

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

    // Set up initial line (Vertex)
    ln = strchr(lc, tc);
    if (ln) *ln = '\0';

    // Trim off prefixes, if user left them in
    while (lc && strchr(lc, ':')) lc++;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(epa->epa_V, a, b, c);
    VSCALE(epa->epa_V, epa->epa_V, local2base);

    // Set up Height line
    read_params_line_incr;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(epa->epa_H, a, b, c);
    VSCALE(epa->epa_H, epa->epa_H, local2base);

    // Set up Semi-major axis line
    read_params_line_incr;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(epa->epa_Au, a, b, c);
    VUNITIZE(epa->epa_Au);

    // Set up Semi-major length line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    epa->epa_r1 = a * local2base;

    // Set up Semi-minor length line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    epa->epa_r2 = a * local2base;

    // Cleanup
    bu_free(wc, "wc");
    return BRLCAD_OK;
}

/* scale height vector H */
void
ecmd_epa_h(struct rt_solid_edit *s)
{
    struct rt_epa_internal *epa =
	(struct rt_epa_internal *)s->es_int.idb_ptr;

    RT_EPA_CK_MAGIC(epa);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / MAGNITUDE(epa->epa_H);
    }
    VSCALE(epa->epa_H, epa->epa_H, s->es_scale);
}

/* scale semimajor axis of EPA */
void
ecmd_epa_r1(struct rt_solid_edit *s)
{
    struct rt_epa_internal *epa =
	(struct rt_epa_internal *)s->es_int.idb_ptr;

    RT_EPA_CK_MAGIC(epa);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / epa->epa_r1;
    }
    if (epa->epa_r1 * s->es_scale >= epa->epa_r2)
	epa->epa_r1 *= s->es_scale;
    else
	bu_log("pscale:  semi-minor axis cannot be longer than semi-major axis!");
}

/* scale semiminor axis of EPA */
void
ecmd_epa_r2(struct rt_solid_edit *s)
{
    struct rt_epa_internal *epa =
	(struct rt_epa_internal *)s->es_int.idb_ptr;

    RT_EPA_CK_MAGIC(epa);
    if (s->e_inpara) {
	/* take s->e_mat[15] (path scaling) into account */
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / epa->epa_r2;
    }
    if (epa->epa_r2 * s->es_scale <= epa->epa_r1)
	epa->epa_r2 *= s->es_scale;
    else
	bu_log("pscale:  semi-minor axis cannot be longer than semi-major axis!");
}

static int
rt_solid_edit_epa_pscale(struct rt_solid_edit *s)
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
	case ECMD_EPA_H:
	    ecmd_epa_h(s);
	    break;
	case ECMD_EPA_R1:
	    ecmd_epa_r1(s);
	    break;
	case ECMD_EPA_R2:
	    ecmd_epa_r2(s);
	    break;
    };

    return 0;
}

int
rt_solid_edit_epa_edit(struct rt_solid_edit *s)
{
    switch (s->edit_flag) {
	case RT_SOLID_EDIT_SCALE:
	    /* scale the solid uniformly about its vertex point */
	    return rt_solid_edit_generic_sscale(s, &s->es_int);
	case RT_SOLID_EDIT_TRANS:
	    /* translate solid */
	    rt_solid_edit_generic_strans(s, &s->es_int);
	    break;
	case RT_SOLID_EDIT_ROT:
	    /* rot solid about vertex */
	    rt_solid_edit_generic_srot(s, &s->es_int);
	    break;
	default:
	    return rt_solid_edit_epa_pscale(s);
    }

    return 0;
}

int
rt_solid_edit_epa_edit_xy(
        struct rt_solid_edit *s,
        const vect_t mousevec
        )
{
    vect_t pos_view = VINIT_ZERO;       /* Unrotated view space pos */
    struct rt_db_internal *ip = &s->es_int;
    bu_clbk_t f = NULL;
    void *d = NULL;

    switch (s->edit_flag) {
        case RT_SOLID_EDIT_SCALE:
	case ECMD_EPA_H:
	case ECMD_EPA_R1:
	case ECMD_EPA_R2:
            rt_solid_edit_generic_sscale_xy(s, mousevec);
            return 0;
        case RT_SOLID_EDIT_TRANS:
            rt_solid_edit_generic_strans_xy(&pos_view, s, mousevec);
            rt_update_edit_absolute_tran(s, pos_view);
            return 0;
        case RT_SOLID_EDIT_ROT:
            bu_vls_printf(s->log_str, "RT_SOLID_EDIT_ROT XY editing setup unimplemented in %s_edit_xy callback\n", EDOBJ[ip->idb_type].ft_label);
            rt_solid_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
            if (f)
                (*f)(0, NULL, d, NULL);
            return BRLCAD_ERROR;
        default:
            bu_vls_printf(s->log_str, "%s: XY edit undefined in solid edit mode %d\n", EDOBJ[ip->idb_type].ft_label, s->edit_flag);
            rt_solid_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
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
