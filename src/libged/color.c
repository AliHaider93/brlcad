/*                         C O L O R . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file color.c
 *
 * The color command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "bio.h"

#include "ged.h"
#include "db.h"
#include "mater.h"
#include "ged_private.h"


/*
 * used by the 'color' command when provided the -e option
 */
static int
_ged_edcolor(struct ged *gedp, int argc, const char *argv[])
{
    register struct mater *mp;
    register struct mater *zot;
    register FILE *fp;
    char line[128];
    static char hdr[] = "LOW\tHIGH\tRed\tGreen\tBlue\n";
    char tmpfil[MAXPATHLEN];
    static const char *usage = "";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    fp = bu_temp_file(tmpfil, MAXPATHLEN);
    if (fp == NULL) {
	bu_vls_printf(&gedp->ged_result_str, "%s: could not create tmp file", argv[0]);
	return BRLCAD_ERROR;
    }

    fprintf( fp, "%s", hdr );
    for (mp = rt_material_head(); mp != MATER_NULL; mp = mp->mt_forw) {
	(void)fprintf( fp, "%d\t%d\t%3d\t%3d\t%3d",
		       mp->mt_low, mp->mt_high,
		       mp->mt_r, mp->mt_g, mp->mt_b);
	(void)fprintf(fp, "\n");
    }
    (void)fclose(fp);

    if (!ged_editit( tmpfil)) {
	bu_vls_printf(&gedp->ged_result_str, "%s: editor returned bad status. Aborted\n", argv[0]);
	return BRLCAD_ERROR;
    }

    /* Read file and process it */
    if ((fp = fopen( tmpfil, "r")) == NULL) {
	perror(tmpfil);
	return BRLCAD_ERROR;
    }

    if (bu_fgets(line, sizeof (line), fp) == NULL ||
	line[0] != hdr[0]) {
	bu_vls_printf(&gedp->ged_result_str, "%s: Header line damaged, aborting\n", argv[0]);
	return BRLCAD_ERROR;
    }

    if (gedp->ged_wdbp->dbip->dbi_version < 5) {
	/* Zap all the current records, both in core and on disk */
	while (rt_material_head() != MATER_NULL) {
	    zot = rt_material_head();
	    rt_new_material_head(zot->mt_forw);
	    ged_color_zaprec(gedp, zot);
	    bu_free((genptr_t)zot, "mater rec");
	}

	while (bu_fgets(line, sizeof (line), fp) != NULL) {
	    int cnt;
	    int low, hi, r, g, b;

	    cnt = sscanf(line, "%d %d %d %d %d",
			 &low, &hi, &r, &g, &b);
	    if (cnt != 5) {
		bu_vls_printf(&gedp->ged_result_str, "%s: Discarding %s\n", argv[0], line);
		continue;
	    }
	    BU_GETSTRUCT(mp, mater);
	    mp->mt_low = low;
	    mp->mt_high = hi;
	    mp->mt_r = r;
	    mp->mt_g = g;
	    mp->mt_b = b;
	    mp->mt_daddr = MATER_NO_ADDR;
	    rt_insert_color(mp);
	    ged_color_putrec(gedp, mp);
	}
    } else {
	struct bu_vls vls;

	/* free colors in rt_material_head */
	rt_color_free();

	bu_vls_init(&vls);

	while (bu_fgets(line, sizeof (line), fp) != NULL) {
	    int cnt;
	    int low, hi, r, g, b;

	    /* check to see if line is reasonable */
	    cnt = sscanf(line, "%d %d %d %d %d",
			 &low, &hi, &r, &g, &b);
	    if (cnt != 5) {
		bu_vls_printf(&gedp->ged_result_str, "%s: Discarding %s\n", argv[0], line);
		continue;
	    }
	    bu_vls_printf(&vls, "{%d %d %d %d %d} ", low, hi, r, g, b);
	}

	db5_update_attribute("_GLOBAL", "regionid_colortable", bu_vls_addr(&vls), gedp->ged_wdbp->dbip);
	db5_import_color_table(bu_vls_addr(&vls));
	bu_vls_free(&vls);
    }

    (void)fclose(fp);
    (void)unlink(tmpfil);

    /* if there are drawables, update their colors */
    if (gedp->ged_gdp)
	ged_color_soltab((struct solid *)&gedp->ged_gdp->gd_headSolid);

    return BRLCAD_OK;
}


int
ged_color(struct ged *gedp, int argc, const char *argv[])
{
    register struct mater *newp;
    register struct mater *mp;
    register struct mater *next_mater;
    static const char *usage = "[-e] [low high r g b]";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 6 && argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    /* edcolor */
    if (argc == 2) {
	if (argv[1][0] == '-' && argv[1][1] == 'e' && argv[1][2] == '\0') {
	    return _ged_edcolor(gedp, argc, argv);
	} else {
	    bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	    return BRLCAD_ERROR;
	}
    }

    if (gedp->ged_wdbp->dbip->dbi_version < 5) {
	/* Delete all color records from the database */
	mp = rt_material_head();
	while (mp != MATER_NULL) {
	    next_mater = mp->mt_forw;
	    ged_color_zaprec(gedp, mp);
	    mp = next_mater;
	}

	/* construct the new color record */
	BU_GETSTRUCT(newp, mater);
	newp->mt_low = atoi(argv[1]);
	newp->mt_high = atoi(argv[2]);
	newp->mt_r = atoi(argv[3]);
	newp->mt_g = atoi(argv[4]);
	newp->mt_b = atoi(argv[5]);
	newp->mt_daddr = MATER_NO_ADDR;		/* not in database yet */

	/* Insert new color record in the in-memory list */
	rt_insert_color(newp);

	/* Write new color records for all colors in the list */
	mp = rt_material_head();
	while (mp != MATER_NULL) {
	    next_mater = mp->mt_forw;
	    ged_color_putrec(gedp, mp);
	    mp = next_mater;
	}
    } else {
	struct bu_vls colors;

	/* construct the new color record */
	BU_GETSTRUCT(newp, mater);
	newp->mt_low = atoi(argv[1]);
	newp->mt_high = atoi(argv[2]);
	newp->mt_r = atoi(argv[3]);
	newp->mt_g = atoi(argv[4]);
	newp->mt_b = atoi(argv[5]);
	newp->mt_daddr = MATER_NO_ADDR;		/* not in database yet */

	/* Insert new color record in the in-memory list */
	rt_insert_color(newp);

	/*
	 * Gather color records from the in-memory list to build
	 * the _GLOBAL objects regionid_colortable attribute.
	 */
	bu_vls_init(&colors);
	rt_vls_color_map(&colors);

	db5_update_attribute("_GLOBAL", "regionid_colortable", bu_vls_addr(&colors), gedp->ged_wdbp->dbip);
	bu_vls_free(&colors);
    }

    return BRLCAD_OK;
}


/**
 *  			G E D _ C O L O R _ P U T R E C
 *@brief
 *  Used to create a database record and get it written out to a granule.
 *  In some cases, storage will need to be allocated.
 */
void
ged_color_putrec(struct ged		*gedp,
		 register struct mater	*mp)
		 
{
    struct directory dir;
    union record rec;

    /* we get here only if database is NOT read-only */

    rec.md.md_id = ID_MATERIAL;
    rec.md.md_low = mp->mt_low;
    rec.md.md_hi = mp->mt_high;
    rec.md.md_r = mp->mt_r;
    rec.md.md_g = mp->mt_g;
    rec.md.md_b = mp->mt_b;

    /* Fake up a directory entry for db_* routines */
    RT_DIR_SET_NAMEP( &dir, "ged_color_putrec" );
    dir.d_magic = RT_DIR_MAGIC;
    dir.d_flags = 0;

    if (mp->mt_daddr == MATER_NO_ADDR) {
	/* Need to allocate new database space */
	if (db_alloc(gedp->ged_wdbp->dbip, &dir, 1) < 0) {
	    bu_vls_printf(&gedp->ged_result_str, "Database alloc error, aborting");
	    return;
	}
	mp->mt_daddr = dir.d_addr;
    } else {
	dir.d_addr = mp->mt_daddr;
	dir.d_len = 1;
    }

    if (db_put(gedp->ged_wdbp->dbip, &dir, &rec, 0, 1) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "Database write error, aborting");
	return;
    }
}

/**
 *  			W D B _ C O L O R _ Z A P R E C
 *@brief
 *  Used to release database resources occupied by a material record.
 */
void
ged_color_zaprec(struct ged		*gedp,
		 register struct mater	*mp)
		 
{
    struct directory dir;

    /* we get here only if database is NOT read-only */
    if (mp->mt_daddr == MATER_NO_ADDR)
	return;

    dir.d_magic = RT_DIR_MAGIC;
    RT_DIR_SET_NAMEP( &dir, "ged_color_zaprec" );
    dir.d_len = 1;
    dir.d_addr = mp->mt_daddr;
    dir.d_flags = 0;

    if (db_delete(gedp->ged_wdbp->dbip, &dir) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "Database delete error, aborting");
	return;
    }
    mp->mt_daddr = MATER_NO_ADDR;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */