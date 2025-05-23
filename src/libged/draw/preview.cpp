/*                       P R E V I E W . C P P
 * BRL-CAD
 *
 * Copyright (c) 2008-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file libged/preview.cpp
 *
 * The preview command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "bsocket.h"

#include "bu/cmd.h"
#include "bu/getopt.h"

#include "../ged_private.h"
extern "C" {
#include "./ged_draw.h"
}

static struct bv_vlblock *preview_vbp;
static double preview_delay;
static int preview_mode;
static int preview_desiredframe;
static int preview_finalframe;
static int preview_currentframe;
static int preview_tree_walk_needed;
static int draw_eye_path;
static char *image_name = NULL;

/* FIXME: this shouldn't exist as a static array and doesn't even seem
 * to be necessary.  gd_rt_cmd points into it as an argv, but the
 * elements can probably be dup'd strings and released by the caller.
 */
#define MAXARGS 9000
static char rt_cmd_storage[MAXARGS*9];


int
ged_cm_anim(struct ged *gedp, vect_t *UNUSED(v), mat_t *UNUSED(m), const int argc, const char **argv)
{

    if (gedp->dbip == DBI_NULL)
	return 0;

    if (db_parse_anim(gedp->dbip, argc, (const char **)argv) < 0) {
	bu_vls_printf(gedp->ged_result_str, "cm_anim:  %s %s failed\n", argv[1], argv[2]);
	return -1;		/* BAD */
    }

    preview_tree_walk_needed = 1;

    return 0;
}


int
ged_cm_clean(struct ged *gedp, vect_t *UNUSED(v), mat_t *UNUSED(m), const int UNUSED(argc), const char **UNUSED(argv))
{
    if (gedp->dbip == DBI_NULL)
	return 0;

    /*f_zap(NULL, interp, 0, (char **)0);*/

    /* Free animation structures */
    db_free_anim(gedp->dbip);

    preview_tree_walk_needed = 0;
    return 0;
}


int
ged_cm_end(struct ged *gedp, vect_t *v, mat_t *m, const int UNUSED(argc), const char **UNUSED(argv))
{
    vect_t xlate;
    vect_t new_cent;
    vect_t xv, yv;			/* view x, y */
    vect_t xm, ym;			/* model x, y */
    struct bu_list *vhead = &preview_vbp->head[0];
    struct bu_list *vlfree = &rt_vlfree;

    /* Only display the frames the user is interested in */
    if (preview_currentframe < preview_desiredframe) return 0;
    if (preview_finalframe && preview_currentframe > preview_finalframe) return 0;

    /* Record eye path as a polyline.  Move, then draws */
    if (BU_LIST_IS_EMPTY(vhead)) {
	BV_ADD_VLIST(vlfree, vhead, (*v), BV_VLIST_LINE_MOVE);
    } else {
	BV_ADD_VLIST(vlfree, vhead, (*v), BV_VLIST_LINE_DRAW);
    }

    /* First step:  put eye at view center (view 0, 0, 0) */
    MAT_COPY(gedp->ged_gvp->gv_rotation, (*m));
    MAT_DELTAS_VEC_NEG(gedp->ged_gvp->gv_center, (*v));
    bv_update(gedp->ged_gvp);

    /*
     * Compute camera orientation notch to right (+X) and up (+Y)
     * Done here, with eye in center of view.
     */
    VSET(xv, 0.05, 0.0, 0.0);
    VSET(yv, 0.0, 0.05, 0.0);
    MAT4X3PNT(xm, gedp->ged_gvp->gv_view2model, xv);
    MAT4X3PNT(ym, gedp->ged_gvp->gv_view2model, yv);
    BV_ADD_VLIST(vlfree, vhead, xm, BV_VLIST_LINE_DRAW);
    BV_ADD_VLIST(vlfree, vhead, (*v), BV_VLIST_LINE_MOVE);
    BV_ADD_VLIST(vlfree, vhead, ym, BV_VLIST_LINE_DRAW);
    BV_ADD_VLIST(vlfree, vhead, (*v), BV_VLIST_LINE_MOVE);

    /* Second step:  put eye at view 0, 0, 1.
     * For eye to be at 0, 0, 1, the old 0, 0, -1 needs to become 0, 0, 0.
     */
    VSET(xlate, 0.0, 0.0, -1.0);	/* correction factor */
    MAT4X3PNT(new_cent, gedp->ged_gvp->gv_view2model, xlate);
    MAT_DELTAS_VEC_NEG(gedp->ged_gvp->gv_center, new_cent);
    bv_update(gedp->ged_gvp);

    /* If new treewalk is needed, get new objects into view. */
    if (preview_tree_walk_needed) {
	const char *av[1] = {"zap"};
	(void)ged_exec_zap(gedp, 1, av);

	int *gd_rt_cmd_len = (int *)BU_PTBL_GET(&gedp->ged_uptrs, 0);
	char ***gd_rt_cmd = (char ***)BU_PTBL_GET(&gedp->ged_uptrs, 1);
	_ged_drawtrees(gedp, *gd_rt_cmd_len, (const char **)&((*gd_rt_cmd)[1]), preview_mode, (struct _ged_client_data *)0);
    }

    ged_refresh_cb(gedp);

    if (preview_delay > 0) {
	struct timeval tv;
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fileno(stdin), &readfds);
	tv.tv_sec = (long)preview_delay;
	tv.tv_usec = (long)((preview_delay - tv.tv_sec) * 1000000);
	select(fileno(stdin)+1, &readfds, (fd_set *)0, (fd_set *)0, &tv);
    }

    return 0;
}


int
ged_cm_multiview(struct ged *UNUSED(gedp), vect_t *UNUSED(v), mat_t *UNUSED(m), const int UNUSED(argc), const char **UNUSED(argv))
{
    return -1;
}


int
ged_cm_start(struct ged *UNUSED(gedp), vect_t *UNUSED(v), mat_t *UNUSED(m), const int argc, const char **argv)
{
    if (argc < 2)
	return -1;
    preview_currentframe = atoi(argv[1]);
    preview_tree_walk_needed = 0;

    return 0;
}


int
ged_cm_tree(struct ged *gedp, vect_t *UNUSED(v), mat_t *UNUSED(m), const int argc, const char **argv)
{
    int i = 1;
    char *cp = rt_cmd_storage;
    int *gd_rt_cmd_len = (int *)BU_PTBL_GET(&gedp->ged_uptrs, 0);
    char ***gd_rt_cmd = (char ***)BU_PTBL_GET(&gedp->ged_uptrs, 1);

    for (i = 1;  i < argc && i < MAXARGS; i++) {
	bu_strlcpy(cp, argv[i], MAXARGS*9);
	(*gd_rt_cmd)[i] = cp;
	cp += strlen(cp) + 1;
    }

    (*gd_rt_cmd)[i] = (char *)0;
    (*gd_rt_cmd_len) = i-1;

    preview_tree_walk_needed = 1;

    return 0;
}


struct ged_command_tab ged_preview_cmdtab[] = {
    {"start", "frame number", "start a new frame",
	ged_cm_start,	2, 2},
    {"viewsize", "size in mm", "set view size",
	_ged_cm_vsize,	2, 2},
    {"eye_pt", "xyz of eye", "set eye point",
	_ged_cm_eyept,	4, 4},
    {"lookat_pt", "x y z [yflip]", "set eye look direction, in X-Y plane",
	_ged_cm_lookat_pt,	4, 5},
    {"orientation", "quaternion", "set view direction from quaternion",
	_ged_cm_orientation,	5, 5},
    {"viewrot", "4x4 matrix", "set view direction from matrix",
	_ged_cm_vrot,	17, 17},
    {"end", 	"", "end of frame setup, begin raytrace",
	ged_cm_end,		1, 1},
    {"multiview", "", "produce stock set of views",
	ged_cm_multiview,	1, 1},
    {"anim", 	"path type args", "specify articulation animation",
	ged_cm_anim,	4, 999},
    {"tree", 	"treetop(s)", "specify alternate list of tree tops",
	ged_cm_tree,	1, 999},
    {"clean", "", "clean articulation from previous frame",
	ged_cm_clean,	1, 1},
    {"set", 	"", "show or set parameters",
	_ged_cm_set,		1, 999},
    {"ae", "azim elev", "specify view as azim and elev, in degrees",
	_ged_cm_null,		3, 3},
    {"opt", "-flags", "set flags, like on command line",
	_ged_cm_null,		2, 999},
    {(char *)0, (char *)0, (char *)0,
	0,		0, 0}	/* END */
};


static int
_loadframe(struct ged *gedp, vect_t *v, mat_t *m, FILE *fp)
{
    char *cmd;

    int end = 0;
    while (!end && ((cmd = rt_read_cmd(fp)) != NULL)) {
	/* Hack to prevent running framedone scripts prematurely */
	if (cmd[0] == '!') {
	    if (preview_currentframe < preview_desiredframe ||
		    (preview_finalframe && preview_currentframe > preview_finalframe)) {
		bu_free((void *)cmd, "preview ! cmd");
		continue;
	    }
	}

	if (cmd[0] == 'e' && bu_strncmp(cmd, "end", 3) == 0) {
	    end = 1;
	}

	if (ged_do_cmd(gedp, v, m, cmd, ged_preview_cmdtab) < 0)
	    bu_vls_printf(gedp->ged_result_str, "command failed: %s\n", cmd);
	bu_free((void *)cmd, "preview cmd");
    }

    if (end) {
	return BRLCAD_OK; /* possible more frames */
    }
    return BRLCAD_ERROR; /* end of frames */
}


/**
 * Preview a new style RT animation script.
 * Note that the RT command parser code is used, rather than the
 * MGED command parser, because of the differences in format.
 * The RT parser expects command handlers of the form "ged_cm_xxx()",
 * and all communications are done via global variables.
 *
 * For the moment, the only preview mode is the normal one,
 * moving the eyepoint as directed.
 * However, as a bonus, the eye path is left behind as a vector plot.
 */
int
ged_preview_core(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "[-v] [-e] [-o image_name.ext]  [-d sec_delay] [-D start frame] [-K last frame] rt_script_file";

    FILE *fp;
    int c;
    vect_t temp;
    char **vp;
    size_t args = 0;
    struct bu_vls extension = BU_VLS_INIT_ZERO;
    struct bu_vls name = BU_VLS_INIT_ZERO;
    char *dot;
    struct bu_list *vlfree = &rt_vlfree;
    vect_t *ged_eye_model = &gedp->i->i->ged_eye_model;
    mat_t *ged_viewrot = &gedp->i->i->ged_viewrot;
    char **gd_rt_cmd = NULL;
    int gd_rt_cmd_len = 0;

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_DRAWABLE(gedp, BRLCAD_ERROR);
    GED_CHECK_VIEW(gedp, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc < 2) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    preview_delay = 0;			/* Full speed, by default */
    preview_mode = 1;			/* wireframe drawing */
    preview_desiredframe = 0;
    preview_finalframe = 0;
    draw_eye_path = 0;
    image_name = NULL;

    /* Parse options */
    bu_optind = 1;			/* re-init bu_getopt() */
    while ((c=bu_getopt(argc, (char * const *)argv, "d:evD:K:o:")) != -1) {
	switch (c) {
	    case 'd':
		preview_delay = atof(bu_optarg);
		break;
	    case 'D':
		preview_desiredframe = atof(bu_optarg);
		break;
	    case 'e':
		draw_eye_path = 1;
		break;
	    case 'K':
		preview_finalframe = atof(bu_optarg);
		break;
	    case 'o':
		image_name = bu_optarg;
		break;
	    case 'v':
		preview_mode = 3;	/* Like "ev" */
		break;
	    default: {
			 bu_vls_printf(gedp->ged_result_str, "option '%c' unknown\n", c);
			 bu_vls_printf(gedp->ged_result_str, "        -d#     inter-frame delay\n");
			 bu_vls_printf(gedp->ged_result_str, "        -e      overlay plot of eye path\n");
			 bu_vls_printf(gedp->ged_result_str, "        -v      polygon rendering (visual)\n");
			 bu_vls_printf(gedp->ged_result_str, "        -D#     desired starting frame\n");
			 bu_vls_printf(gedp->ged_result_str, "        -K#     final frame\n");
			 bu_vls_printf(gedp->ged_result_str, "        -o image_name.ext     output frame to file typed by extension(defaults to PIX)\n");
			 return BRLCAD_ERROR;
		     }

		     break;
	}
    }
    argc -= bu_optind-1;
    argv += bu_optind-1;

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
	perror(argv[1]);
	return BRLCAD_ERROR;
    }

    args = argc + 2 + ged_who_argc(gedp);
    gd_rt_cmd = (char **)bu_calloc(args, sizeof(char *), "alloc gd_rt_cmd");
    vp = &gd_rt_cmd[0];
    *vp++ = bu_strdup("tree");

    /* Build list of top-level objects in view, in gd_rt_cmd[] */
    gd_rt_cmd_len = ged_who_argv(gedp, vp, (const char **)&gd_rt_cmd[args]);
    /* Print out the command we are about to run */
    vp = &gd_rt_cmd[0];
    while ((vp != NULL) && (*vp))
	bu_vls_printf(gedp->ged_result_str, "%s ", *vp++);

    bu_vls_printf(gedp->ged_result_str, "\n");

    preview_vbp = bv_vlblock_init(vlfree, 32);

    bu_vls_printf(gedp->ged_result_str, "eyepoint at (0, 0, 1) viewspace\n");

    /* Assign the rt command and length pointers to the ged user pointer
     * container */
    struct bu_ptbl ged_tmp_uptrs = BU_PTBL_INIT_ZERO;
    bu_ptbl_cat(&ged_tmp_uptrs, &gedp->ged_uptrs);
    bu_ptbl_reset(&gedp->ged_uptrs);
    bu_ptbl_ins(&gedp->ged_uptrs, (long *)&gd_rt_cmd_len);
    bu_ptbl_ins(&gedp->ged_uptrs, (long *)&gd_rt_cmd);

    /*
     * Initialize the view to the current one provided by the ged
     * structure in case a view specification is never given.
     */
    MAT_COPY(*ged_viewrot, gedp->ged_gvp->gv_rotation);
    VSET(temp, 0.0, 0.0, 1.0);
    MAT4X3PNT(*ged_eye_model, gedp->ged_gvp->gv_view2model, temp);

    if (image_name) {
	/* parse file name and possible extension */
	if ((dot = strrchr(image_name, '.')) != (char *) NULL) {
	    bu_vls_strncpy(&name, image_name, dot - image_name);
	    bu_vls_strcpy(&extension, dot);
	} else {
	    bu_vls_strcpy(&extension, "");
	    bu_vls_strcpy(&name, image_name);
	}
    }
    while (_loadframe(gedp, ged_eye_model, ged_viewrot, fp) == BRLCAD_OK) {
	if (image_name) {
	    struct bu_vls fullname = BU_VLS_INIT_ZERO;
	    const char *screengrab_args[2] = {"screengrab", NULL};
	    bu_vls_sprintf(&fullname, "%s%05d%s", bu_vls_cstr(&name),
		    preview_currentframe, bu_vls_cstr(&extension));
	    screengrab_args[1] = bu_vls_cstr(&fullname);
	    ged_exec_screengrab(gedp, 2, screengrab_args);

	    bu_vls_free(&fullname);
	}
    }

    if (image_name) {
	bu_vls_free(&name);
	bu_vls_free(&extension);
    }

    fclose(fp);
    fp = NULL;

    if (draw_eye_path) {
	if (gedp->new_cmd_forms) {
	    struct bview *view = gedp->ged_gvp;
	    bv_vlblock_obj(preview_vbp, view, "preview::eye_path");
	} else {
	    _ged_cvt_vlblock_to_solids(gedp, preview_vbp, "EYE_PATH", 0);
	}
    }

    if (preview_vbp) {
	bv_vlblock_free(preview_vbp);
	preview_vbp = (struct bv_vlblock *)NULL;
    }
    db_free_anim(gedp->dbip);	/* Forget any anim commands */

    // Restore app ged_uptrs
    bu_ptbl_reset(&gedp->ged_uptrs);
    bu_ptbl_cat(&gedp->ged_uptrs, &ged_tmp_uptrs);

    return BRLCAD_OK;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8 cino=N-s
