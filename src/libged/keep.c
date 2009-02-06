/*                         K E E P . C
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
/** @file keep.c
 *
 * The keep command.
 *
 */

#include "common.h"

#include <string.h>

#include "bio.h"
#include "cmd.h"
#include "rtgeom.h"
#include "ged_private.h"

struct keep_node_data {
    struct rt_wdb *wdbp;
    struct ged    *gedp;
};

static void
ged_node_write(struct db_i		*dbip,
	       register struct directory *dp,
	       genptr_t			ptr);

int
ged_keep(struct ged *gedp, int argc, const char *argv[])
{
    struct keep_node_data knd;
    struct rt_wdb		*keepfp;
    register struct directory *dp;
    struct bu_vls		title;
    register int		i;
    struct db_i		*new_dbip;
    static const char *usage = "file object(s)";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc < 3 || MAXARGS < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    /* First, clear any existing counts */
    for (i = 0; i < RT_DBNHASH; i++) {
	for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
	    dp->d_nref = 0;
    }

    /* Alert user if named file already exists */

    new_dbip = db_open(argv[1], "w");

    if (new_dbip != DBI_NULL) {
	if (new_dbip->dbi_version != gedp->ged_wdbp->dbip->dbi_version) {
	    bu_vls_printf(&gedp->ged_result_str, "keep: File format mismatch between '%s' and '%s'\n",
			  argv[1], gedp->ged_wdbp->dbip->dbi_filename);
	    return BRLCAD_ERROR;
	}

	if ((keepfp = wdb_dbopen(new_dbip, RT_WDB_TYPE_DB_DISK)) == NULL) {
	    bu_vls_printf(&gedp->ged_result_str, "keep: Error opening '%s'\n", argv[1]);
	    return BRLCAD_ERROR;
	} else {
	    bu_vls_printf(&gedp->ged_result_str, "keep:  appending to '%s'\n");

	    /* --- Scan geometry database and build in-memory directory --- */
	    db_dirbuild(new_dbip);
	}
    } else {
	/* Create a new database */
	keepfp = wdb_fopen_v(argv[1], gedp->ged_wdbp->dbip->dbi_version);

	if (keepfp == NULL) {
	    perror(argv[1]);
	    return BRLCAD_ERROR;
	}
    }

    knd.wdbp = keepfp;
    knd.gedp = gedp;

    /* ident record */
    bu_vls_init(&title);
    if (strncmp(gedp->ged_wdbp->dbip->dbi_title, "Parts of: ", 10) != 0) {
	bu_vls_strcat(&title, "Parts of: ");
    }
    bu_vls_strcat(&title, gedp->ged_wdbp->dbip->dbi_title);

    if (db_update_ident(keepfp->dbip, bu_vls_addr(&title), gedp->ged_wdbp->dbip->dbi_local2base) < 0) {
	perror("fwrite");
	bu_vls_printf(&gedp->ged_result_str, "db_update_ident() failed\n");
	wdb_close(keepfp);
	bu_vls_free(&title);
	return BRLCAD_ERROR;
    }
    bu_vls_free(&title);

    for (i = 2; i < argc; i++) {
	if ((dp = db_lookup(gedp->ged_wdbp->dbip, argv[i], LOOKUP_NOISY)) == DIR_NULL)
	    continue;
	db_functree(gedp->ged_wdbp->dbip, dp, ged_node_write, ged_node_write, &rt_uniresource, (genptr_t)&knd);
    }

    wdb_close(keepfp);
    return BRLCAD_OK;
}

/**
 *			G E D _ N O D E _ W R I T E
 *@brief
 *  Support for the 'keep' method.
 *  Write each node encountered exactly once.
 */
void
ged_node_write(struct db_i		*dbip,
	       register struct directory *dp,
	       genptr_t			ptr)
{
    struct keep_node_data *kndp = (struct keep_node_data *)ptr;
    struct rt_db_internal intern;

    RT_CK_WDB(kndp->wdbp);

    if (dp->d_nref++ > 0)
	return;		/* already written */

    if (rt_db_get_internal(&intern, dp, dbip, NULL, &rt_uniresource) < 0) {
	bu_vls_printf(&kndp->gedp->ged_result_str, "Database read error, aborting\n");
	return;
    }

    /* if this is an extrusion, keep the referenced sketch */
    if ( dp->d_major_type == DB5_MAJORTYPE_BRLCAD && dp->d_minor_type == DB5_MINORTYPE_BRLCAD_EXTRUDE ) {
	struct rt_extrude_internal *extr;
	struct directory *dp2;

	extr = (struct rt_extrude_internal *)intern.idb_ptr;
	RT_EXTRUDE_CK_MAGIC( extr );

	if ( (dp2 = db_lookup( dbip, extr->sketch_name, LOOKUP_QUIET )) != DIR_NULL ) {
	    ged_node_write( dbip, dp2, ptr );
	}
    } else if ( dp->d_major_type == DB5_MAJORTYPE_BRLCAD && dp->d_minor_type == DB5_MINORTYPE_BRLCAD_DSP ) {
	struct rt_dsp_internal *dsp;
	struct directory *dp2;

	/* this is a DSP, if it uses a binary object, keep it also */
	dsp = (struct rt_dsp_internal *)intern.idb_ptr;
	RT_DSP_CK_MAGIC( dsp );

	if ( dsp->dsp_datasrc == RT_DSP_SRC_OBJ ) {
	    /* need to keep this object */
	    if ( (dp2 = db_lookup( dbip, bu_vls_addr(&dsp->dsp_name),  LOOKUP_QUIET )) != DIR_NULL ) {
		ged_node_write( dbip, dp2, ptr );
	    }
	}
    }

    if (wdb_put_internal(kndp->wdbp, dp->d_namep, &intern, 1.0) < 0) {
	bu_vls_printf(&kndp->gedp->ged_result_str, "Database write error, aborting\n");
	return;
    }
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