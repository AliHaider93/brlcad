/*                         Q V R O T . C
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
/** @file libged/qvrot.c
 *
 * The qvrot command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../ged_private.h"


/*
 * Apply the "joystick" delta rotation to the viewing direction,
 * where the delta is specified in terms of the *viewing* axes.
 * Rotation is performed about the view center, for now.
 * Angles are in radians.
 */
static void
usejoy(struct ged *gedp, double xangle, double yangle, double zangle)
{
    mat_t newrot;		/* NEW rot matrix, from joystick */

    /* NORMAL CASE.
     * Apply delta viewing rotation for non-edited parts.
     * The view rotates around the VIEW CENTER.
     */
    MAT_IDN(newrot);
    bn_mat_angles_rad(newrot, xangle, yangle, zangle);
    bn_mat_mul2(newrot, gedp->ged_gvp->gv_rotation);
}


int
ged_qvrot_core(struct ged *gedp, int argc, const char *argv[])
{
    double dx, dy, dz;
    double az;
    double el;
    double theta;
    int iflag = 0;
    static const char *usage = "[-i] x y z [angle]";

    GED_CHECK_VIEW(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    /* check for -i */
    if (argv[1][0] == '-' && argv[1][1] == 'i' && argv[1][2] == '\0') {
	iflag = 1;
	argv++; argc--;
    }

    if (argc != 4 && argc != 5) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[1], "%lf", &dx) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad X value - %s\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &dy) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad Y value - %s\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[3], "%lf", &dz) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad Z value - %s\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if (argc == 5 && sscanf(argv[4], "%lf", &theta) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad angle - %s\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    } else if (argc == 4) {
	theta = 0;
    }

    if (iflag) {
	dx *= -1; dy *= -1; dz *= -1;
	theta *= -1;
    }

    if (NEAR_ZERO(dy, 0.00001) && NEAR_ZERO(dx, 0.00001)) {
	if (NEAR_ZERO(dz, 0.00001)) {
	    bu_vls_printf(gedp->ged_result_str, "%s: (dx, dy, dz) may not be the zero vector\n", argv[0]);
	    return BRLCAD_ERROR;
	}
	az = 0.0;
    } else
	az = atan2(dy, dx);

    el = atan2(dz, sqrt(dx * dx + dy * dy));

    bn_mat_angles(gedp->ged_gvp->gv_rotation, 270.0 + el * RAD2DEG, 0.0, 270.0 - az * RAD2DEG);
    usejoy(gedp, 0.0, 0.0, theta*DEG2RAD);
    bv_update(gedp->ged_gvp);

    return BRLCAD_OK;
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
