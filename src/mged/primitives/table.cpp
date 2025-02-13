/*                       T A B L E . C P P
 * BRL-CAD
 *
 * Copyright (c) 1989-2024 United States Government as represented by
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
/** @file primitives/table.cpp
 *
 * Per-primitive methods for MGED editing.
 *
 */
/** @} */

#include "common.h"

extern "C" {

#include "vmath.h"
#include "./mged_functab.h"

#define MGED_DECLARE_INTERFACE(name) \
    extern void mged_##name##_labels(int *num_lines, point_t *lines, struct rt_point_labels *pl, int max_pl, const mat_t xform, struct rt_db_internal *ip, struct bn_tol *); \
    extern const char *mged_##name##_keypoint(point_t *pt, const char *keystr, const mat_t mat, const struct rt_db_internal *ip, const struct bn_tol *tol); \


MGED_DECLARE_INTERFACE(tor);
MGED_DECLARE_INTERFACE(tgc);
MGED_DECLARE_INTERFACE(ell);
MGED_DECLARE_INTERFACE(arb);
MGED_DECLARE_INTERFACE(ars);
MGED_DECLARE_INTERFACE(hlf);
MGED_DECLARE_INTERFACE(rec);
MGED_DECLARE_INTERFACE(pg);
MGED_DECLARE_INTERFACE(bspline);
MGED_DECLARE_INTERFACE(sph);
MGED_DECLARE_INTERFACE(ebm);
MGED_DECLARE_INTERFACE(vol);
MGED_DECLARE_INTERFACE(arbn);
MGED_DECLARE_INTERFACE(pipe);
MGED_DECLARE_INTERFACE(part);
MGED_DECLARE_INTERFACE(nmg);
MGED_DECLARE_INTERFACE(rpc);
MGED_DECLARE_INTERFACE(rhc);
MGED_DECLARE_INTERFACE(epa);
MGED_DECLARE_INTERFACE(ehy);
MGED_DECLARE_INTERFACE(eto);
MGED_DECLARE_INTERFACE(grp);
MGED_DECLARE_INTERFACE(hf);
MGED_DECLARE_INTERFACE(dsp);
MGED_DECLARE_INTERFACE(sketch);
MGED_DECLARE_INTERFACE(annot);
MGED_DECLARE_INTERFACE(extrude);
MGED_DECLARE_INTERFACE(submodel);
MGED_DECLARE_INTERFACE(cline);
MGED_DECLARE_INTERFACE(bot);
MGED_DECLARE_INTERFACE(superell);
MGED_DECLARE_INTERFACE(metaball);
MGED_DECLARE_INTERFACE(hyp);
MGED_DECLARE_INTERFACE(revolve);
MGED_DECLARE_INTERFACE(constraint);
MGED_DECLARE_INTERFACE(material);
/* MGED_DECLARE_INTERFACE(binunif); */
MGED_DECLARE_INTERFACE(pnts);
MGED_DECLARE_INTERFACE(hrt);
MGED_DECLARE_INTERFACE(datum);
MGED_DECLARE_INTERFACE(brep);
MGED_DECLARE_INTERFACE(joint);
MGED_DECLARE_INTERFACE(script);

const struct mged_functab MGED_OBJ[] = {
    {
	/* 0: unused, for sanity checking. */
	RT_FUNCTAB_MAGIC, "ID_NULL", "NULL",
	NULL,
	NULL
    },

    {
	/* 1 */
	RT_FUNCTAB_MAGIC, "ID_TOR", "tor",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 2 */
	RT_FUNCTAB_MAGIC, "ID_TGC", "tgc",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 3 */
	RT_FUNCTAB_MAGIC, "ID_ELL", "ell",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 4 */
	RT_FUNCTAB_MAGIC, "ID_ARB8", "arb8",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_arb_keypoint) /* keypoint */
    },

    {
	/* 5 */
	RT_FUNCTAB_MAGIC, "ID_ARS", "ars",
	MGEDFUNCTAB_FUNC_LABELS_CAST(mged_ars_labels),    /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_ars_keypoint) /* keypoint */
    },

    {
	/* 6 */
	RT_FUNCTAB_MAGIC, "ID_HALF", "half",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 7 */
	RT_FUNCTAB_MAGIC, "ID_REC", "rec",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 8 */
	RT_FUNCTAB_MAGIC, "ID_POLY", "poly",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 9 */
	RT_FUNCTAB_MAGIC, "ID_BSPLINE", "bspline",
	MGEDFUNCTAB_FUNC_LABELS_CAST(mged_bspline_labels), /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_bspline_keypoint) /* keypoint */
    },

    {
	/* 10 */
	RT_FUNCTAB_MAGIC, "ID_SPH", "sph",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 11 */
	RT_FUNCTAB_MAGIC, "ID_NMG", "nmg",
	MGEDFUNCTAB_FUNC_LABELS_CAST(mged_nmg_labels),    /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_nmg_keypoint) /* keypoint */
    },

    {
	/* 12 */
	RT_FUNCTAB_MAGIC, "ID_EBM", "ebm",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 13 */
	RT_FUNCTAB_MAGIC, "ID_VOL", "vol",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 14 */
	RT_FUNCTAB_MAGIC, "ID_ARBN", "arbn",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 15 */
	RT_FUNCTAB_MAGIC, "ID_PIPE", "pipe",
	MGEDFUNCTAB_FUNC_LABELS_CAST(mged_pipe_labels), /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_pipe_keypoint) /* keypoint */
    },

    {
	/* 16 */
	RT_FUNCTAB_MAGIC, "ID_PARTICLE", "part",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 17 */
	RT_FUNCTAB_MAGIC, "ID_RPC", "rpc",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 18 */
	RT_FUNCTAB_MAGIC, "ID_RHC", "rhc",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 19 */
	RT_FUNCTAB_MAGIC, "ID_EPA", "epa",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 20 */
	RT_FUNCTAB_MAGIC, "ID_EHY", "ehy",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 21 */
	RT_FUNCTAB_MAGIC, "ID_ETO", "eto",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 22 */
	RT_FUNCTAB_MAGIC, "ID_GRIP", "grip",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_grp_keypoint) /* keypoint */
    },

    {
	/* 23 -- XXX unimplemented */
	RT_FUNCTAB_MAGIC, "ID_JOINT", "joint",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 24 */
	RT_FUNCTAB_MAGIC, "ID_HF", "hf",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 25 Displacement Map (alt. height field) */
	RT_FUNCTAB_MAGIC, "ID_DSP", "dsp",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 26 2D sketch */
	RT_FUNCTAB_MAGIC, "ID_SKETCH", "sketch",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 27 Solid of extrusion */
	RT_FUNCTAB_MAGIC, "ID_EXTRUDE", "extrude",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_extrude_keypoint) /* keypoint */
    },

    {
	/* 28 Instanced submodel */
	RT_FUNCTAB_MAGIC, "ID_SUBMODEL", "submodel",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 29 Fastgen cline solid */
	RT_FUNCTAB_MAGIC, "ID_CLINE", "cline",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 30 Bag o' Triangles */
	RT_FUNCTAB_MAGIC, "ID_BOT", "bot",
	MGEDFUNCTAB_FUNC_LABELS_CAST(mged_bot_labels),    /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_bot_keypoint) /* keypoint */
    },

    {
	/* 31 combination objects (should not be in this table) */
	RT_FUNCTAB_MAGIC, "ID_COMBINATION", "comb",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 32 available placeholder to not offset latter table indices
	 * (was ID_BINEXPM)
	 */
	RT_FUNCTAB_MAGIC, "ID_UNUSED1", "UNUSED1",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 33 */
	RT_FUNCTAB_MAGIC, "ID_BINUNIF", "binunif",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 34 available placeholder to not offset latter table indices
	 * (was ID_BINMIME)
	 */
	RT_FUNCTAB_MAGIC, "ID_UNUSED2", "UNUSED2",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 35 (but "should" be 31) Superquadratic Ellipsoid */
	RT_FUNCTAB_MAGIC, "ID_SUPERELL", "superell",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 36 (but "should" be 32) Metaball */
	RT_FUNCTAB_MAGIC, "ID_METABALL", "metaball",
	MGEDFUNCTAB_FUNC_LABELS_CAST(mged_metaball_labels),    /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_metaball_keypoint) /* keypoint */
    },

    {
	/* 37 */
	RT_FUNCTAB_MAGIC, "ID_BREP", "brep",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 38 (but "should" be 34) Hyperboloid */
	RT_FUNCTAB_MAGIC, "ID_HYP", "hyp",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 39 */
	RT_FUNCTAB_MAGIC, "ID_CONSTRAINT", "constrnt",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 40 */
	RT_FUNCTAB_MAGIC, "ID_REVOLVE", "revolve",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 41 */
	RT_FUNCTAB_MAGIC, "ID_PNTS", "pnts",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 42 */
	RT_FUNCTAB_MAGIC, "ID_ANNOT", "annot",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },

    {
	/* 43 */
	RT_FUNCTAB_MAGIC, "ID_HRT", "hrt",
	NULL,  /* label */
	NULL   /* keypoint */
    },


    {
	/* 44 */
	RT_FUNCTAB_MAGIC, "ID_DATUM", "datum",
	NULL,  /* label */
	MGEDFUNCTAB_FUNC_KEYPOINT_CAST(mged_generic_keypoint) /* keypoint */
    },


    {
    /* 45 */
    RT_FUNCTAB_MAGIC, "ID_SCRIPT", "script",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* 46 */
	RT_FUNCTAB_MAGIC, "ID_MATERIAL", "material",
	NULL,  /* label */
	NULL   /* keypoint */
    },

    {
	/* this entry for sanity only */
	0L, ">ID_MAXIMUM", ">id_max",
	NULL,  /* label */
	NULL   /* keypoint */
    }
};

} /* end extern "C" */

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

