/*                         E D C L I N E . C
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
/** @file primitives/edcline.c
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

#define ECMD_CLINE_SCALE_H	29077	/* scale height vector */
#define ECMD_CLINE_MOVE_H	29078	/* move end of height vector */
#define ECMD_CLINE_SCALE_R	29079	/* scale radius */
#define ECMD_CLINE_SCALE_T	29080	/* scale thickness */

void
rt_edit_cline_set_edit_mode(struct rt_edit *s, int mode)
{
    rt_edit_set_edflag(s, mode);

    switch (mode) {
	case ECMD_CLINE_MOVE_H:
	    s->edit_mode = RT_PARAMS_EDIT_TRANS;
	    break;
	case ECMD_CLINE_SCALE_H:
	case ECMD_CLINE_SCALE_R:
	case ECMD_CLINE_SCALE_T:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    break;
	default:
	    break;
    };

    rt_edit_process(s);
}

static void
cline_ed(struct rt_edit *s, int arg, int UNUSED(a), int UNUSED(b), void *UNUSED(data))
{
    rt_edit_cline_set_edit_mode(s, arg);
}

struct rt_edit_menu_item cline_menu[] = {
    { "CLINE MENU",		NULL, 0 },
    { "Set H",		cline_ed, ECMD_CLINE_SCALE_H },
    { "Move End H",		cline_ed, ECMD_CLINE_MOVE_H },
    { "Set R",		cline_ed, ECMD_CLINE_SCALE_R },
    { "Set plate thickness", cline_ed, ECMD_CLINE_SCALE_T },
    { "", NULL, 0 }
};

struct rt_edit_menu_item *
rt_edit_cline_menu_item(const struct bn_tol *UNUSED(tol))
{
    return cline_menu;
}

void
rt_edit_cline_e_axes_pos(
	struct rt_edit *s,
	const struct rt_db_internal *ip,
       	const struct bn_tol *UNUSED(tol))
{
    if (s->edit_flag == ECMD_CLINE_MOVE_H) {
	struct rt_cline_internal *cli =(struct rt_cline_internal *)ip->idb_ptr;
	point_t cli_v;
	vect_t cli_h;

	RT_CLINE_CK_MAGIC(cli);

	MAT4X3PNT(cli_v, s->e_mat, cli->v);
	MAT4X3VEC(cli_h, s->e_mat, cli->h);
	VADD2(s->curr_e_axes_pos, cli_h, cli_v);
    } else {
	VMOVE(s->curr_e_axes_pos, s->e_keypoint);
    }
}

/*
 * Scale height vector
 */
int
ecmd_cline_scale_h(struct rt_edit *s)
{
    if (s->e_inpara != 1) {
	bu_vls_printf(s->log_str, "ERROR: only one argument needed\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (s->e_para[0] <= 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    /* must convert to base units */
    s->e_para[0] *= s->local2base;
    s->e_para[1] *= s->local2base;
    s->e_para[2] *= s->local2base;

    struct rt_cline_internal *cli =
	(struct rt_cline_internal *)s->es_int.idb_ptr;

    RT_CLINE_CK_MAGIC(cli);

    if (s->e_inpara) {
	s->e_para[0] *= s->e_mat[15];
	s->es_scale = s->e_para[0] / MAGNITUDE(cli->h);
	VSCALE(cli->h, cli->h, s->es_scale);
    } else if (s->es_scale > 0.0) {
	VSCALE(cli->h, cli->h, s->es_scale);
	s->es_scale = 0.0;
    }

    return 0;
}

/*
 * Scale radius
 */
int
ecmd_cline_scale_r(struct rt_edit *s)
{
    if (s->e_inpara != 1) {
	bu_vls_printf(s->log_str, "ERROR: only one argument needed\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    /* must convert to base units */
    s->e_para[0] *= s->local2base;
    s->e_para[1] *= s->local2base;
    s->e_para[2] *= s->local2base;

    if (s->e_para[0] <= 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    struct rt_cline_internal *cli =
	(struct rt_cline_internal *)s->es_int.idb_ptr;

    RT_CLINE_CK_MAGIC(cli);

    if (s->e_inpara)
	cli->radius = s->e_para[0];
    else if (s->es_scale > 0.0) {
	cli->radius *= s->es_scale;
	s->es_scale = 0.0;
    }

    return 0;
}

/*
 * Scale plate thickness
 */
int
ecmd_cline_scale_t(struct rt_edit *s)
{
    if (s->e_inpara != 1) {
	bu_vls_printf(s->log_str, "ERROR: only one argument needed\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (s->e_para[0] <= 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    /* must convert to base units */
    s->e_para[0] *= s->local2base;
    s->e_para[1] *= s->local2base;
    s->e_para[2] *= s->local2base;

    struct rt_cline_internal *cli =
	(struct rt_cline_internal *)s->es_int.idb_ptr;

    RT_CLINE_CK_MAGIC(cli);

    if (s->e_inpara)
	cli->thickness = s->e_para[0];
    else if (s->es_scale > 0.0) {
	cli->thickness *= s->es_scale;
	s->es_scale = 0.0;
    }

    return 0;
}

/*
 * Move end of height vector
 */
int
ecmd_cline_move_h(struct rt_edit *s)
{
    vect_t work;
    struct rt_cline_internal *cli =
	(struct rt_cline_internal *)s->es_int.idb_ptr;
    bu_clbk_t f = NULL;
    void *d = NULL;
 
    RT_CLINE_CK_MAGIC(cli);

    if (s->e_inpara) {
	if (s->e_inpara != 3) {
	    bu_vls_printf(s->log_str, "ERROR: three arguments needed\n");
	    s->e_inpara = 0;
	    return BRLCAD_ERROR;
	}

	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;

	if (s->mv_context) {
	    MAT4X3PNT(work, s->e_invmat, s->e_para);
	    VSUB2(cli->h, work, cli->v);
	} else
	    VSUB2(cli->h, s->e_para, cli->v);
    }
    /* check for zero H vector */
    if (MAGNITUDE(cli->h) <= SQRT_SMALL_FASTF) {
	bu_vls_printf(s->log_str, "Zero H vector not allowed, resetting to +Z\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	VSET(cli->h, 0.0, 0.0, 1.0);
	return BRLCAD_ERROR;
    }

    return 0;
}

void
ecmd_cline_move_h_mousevec(struct rt_edit *s, const vect_t mousevec)
{
    vect_t pos_view = VINIT_ZERO;	/* Unrotated view space pos */
    vect_t tr_temp = VINIT_ZERO;	/* temp translation vector */
    vect_t temp = VINIT_ZERO;
    struct rt_cline_internal *cli =
	(struct rt_cline_internal *)s->es_int.idb_ptr;

    RT_CLINE_CK_MAGIC(cli);

    MAT4X3PNT(pos_view, s->vp->gv_model2view, s->curr_e_axes_pos);
    pos_view[X] = mousevec[X];
    pos_view[Y] = mousevec[Y];
    /* Do NOT change pos_view[Z] ! */
    MAT4X3PNT(temp, s->vp->gv_view2model, pos_view);
    MAT4X3PNT(tr_temp, s->e_invmat, temp);
    VSUB2(cli->h, tr_temp, cli->v);
}

int
rt_edit_cline_edit(struct rt_edit *s)
{
    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	    /* scale the solid uniformly about its vertex point */
	    return edit_sscale(s);
	case RT_PARAMS_EDIT_TRANS:
	    /* translate solid */
	    edit_stra(s);
	    break;
	case RT_PARAMS_EDIT_ROT:
	    /* rot solid about vertex */
	    edit_srot(s);
	    break;
	case ECMD_CLINE_SCALE_H:
	    return ecmd_cline_scale_h(s);
	case ECMD_CLINE_SCALE_R:
	    return ecmd_cline_scale_r(s);
	case ECMD_CLINE_SCALE_T:
	    return ecmd_cline_scale_t(s);
	case ECMD_CLINE_MOVE_H:
	    return ecmd_cline_move_h(s);
    }

    return 0;
}

int
rt_edit_cline_edit_xy(
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
	case ECMD_CLINE_SCALE_H:
	case ECMD_CLINE_SCALE_R:
	case ECMD_CLINE_SCALE_T:
	    edit_sscale_xy(s, mousevec);
	    return 0;
	case RT_PARAMS_EDIT_TRANS:
	    edit_stra_xy(&pos_view, s, mousevec);
	    break;
	case ECMD_CLINE_MOVE_H:
	    ecmd_cline_move_h_mousevec(s, mousevec);
	    break;
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

    edit_abs_tra(s, pos_view);

    return 0;
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
