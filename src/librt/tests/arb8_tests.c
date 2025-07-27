/*                    A R B 8 _ T E S T S . C
 * BRL-CAD
 *
 * Copyright (c) 2025 United States Government as represented by
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
/** @file arb8_tests.c
 *
 * Test different configurations of valid and invalid ARB8
 *
 */

#include "common.h"

#include <stdio.h>

#include "bu/log.h"
#include "vmath.h"
#include "raytrace.h"


// ARB8 configurations
typedef struct {
    const char *label;
    point_t vertices[8];
} arb8_config_t;

arb8_config_t arb8_configs[] = {
    // Standard ARB8 configurations (all starting in 1st quadrant, some extending outside)
    {"StandardCube", {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}}, /* axis-aligned unit cube */
    {"SlantedARB8_1", {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.2, 0.2, 1}, {1.2, 0.2, 1}, {1.2, 1.2, 1}, {0.2, 1.2, 1}}}, /* uniform top face skew */
    {"SlantedARB8_2", {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.1, 0.3, 1}, {1.1, 0.3, 1}, {1.3, 1.3, 1}, {0.1, 1.3, 1}}}, /* uneven skew */
    {"SlantedARB8_3", {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {-0.2, 0.2, 1}, {0.8, 0.2, 1}, {0.8, 1.2, 1}, {-0.2, 1.2, 1}}}, /* opposite dir skew */
    {"IrregularARB8_1", {{0, 0, 0}, {1, 0, 0.1}, {1.1, 1, 0}, {0, 1.1, -0.1}, {0.1, 0, 1}, {1, 0.1, 1.1}, {1.1, 1.1, 0.9}, {0, 1, 1}}}, /* warped non-planar prism */
    {"IrregularARB8_2", {{0, 0, 0}, {1, 0, -0.1}, {1.1, 1.1, 0}, {0, 1.2, 0.1}, {0.2, 0, 0.9}, {1, 0.1, 1.2}, {1.2, 1.2, 0.8}, {0, 1, 1}}}, /* twisted top, non-planar sides */
    {"IrregularARB8_3", {{0, 0, 0}, {0.8, 0, 0.1}, {1.1, 0.9, 0.2}, {0.1, 1.1, -0.2}, {0.2, -0.2, 0.8}, {1, 0, 1.1}, {1.1, 0.9, 0.9}, {0, 1, 0.9}}}, /* complex warp, non-planar */

    // Unit cubes in each quadrant
    {"UnitCube_Q1", {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
    {"UnitCube_Q2", {{-1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {-1, 1, 0}, {-1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {-1, 1, 1}}},
    {"UnitCube_Q3", {{-1, -1, 0}, {0, -1, 0}, {0, 0, 0}, {-1, 0, 0}, {-1, -1, 1}, {0, -1, 1}, {0, 0, 1}, {-1, 0, 1}}},
    {"UnitCube_Q4", {{0, -1, 0}, {1, -1, 0}, {1, 0, 0}, {0, 0, 0}, {0, -1, 1}, {1, -1, 1}, {1, 0, 1}, {0, 0, 1}}},
    {"UnitCube_Q5", {{0, 0, -1}, {1, 0, -1}, {1, 1, -1}, {0, 1, -1}, {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}},
    {"UnitCube_Q6", {{-1, 0, -1}, {0, 0, -1}, {0, 1, -1}, {-1, 1, -1}, {-1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {-1, 1, 0}}},
    {"UnitCube_Q7", {{-1, -1, -1}, {0, -1, -1}, {0, 0, -1}, {-1, 0, -1}, {-1, -1, 0}, {0, -1, 0}, {0, 0, 0}, {-1, 0, 0}}},
    {"UnitCube_Q8", {{0, -1, -1}, {1, -1, -1}, {1, 0, -1}, {0, 0, -1}, {0, -1, 0}, {1, -1, 0}, {1, 0, 0}, {0, 0, 0}}},

    // Elongated cubes
    {"Elongated_X1", {{-1, 0, 0}, {1, 0, 0}, {1, 1, 0}, {-1, 1, 0}, {-1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {-1, 1, 1}}}, /* +-X in +Y,+Z*/
    {"Elongated_X2", {{-1, -1, 0}, {1, -1, 0}, {1, 0, 0}, {-1, 0, 0}, {-1, -1, 1}, {1, -1, 1}, {1, 0, 1}, {-1, 0, 1}}}, /* +-X in -Y,+Z */
    {"Elongated_Y1", {{0, -1, 0}, {1, -1, 0}, {1, 1, 0}, {0, 1, 0}, {0, -1, 1}, {1, -1, 1}, {1, 1, 1}, {0, 1, 1}}}, /* +-Y in +X,+Z */
    {"Elongated_Y2", {{-1, -1, 0}, {0, -1, 0}, {0, 1, 0}, {-1, 1, 0}, {-1, -1, 1}, {0, -1, 1}, {0, 1, 1}, {-1, 1, 1}}}, /* +-Y in -X,+Z */
    {"Elongated_Z1", {{0, 0, -1}, {1, 0, -1}, {1, 1, -1}, {0, 1, -1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}}, /* +-Z in +X,+Y */
    {"Elongated_Z2", {{-1, 0, -1}, {0, 0, -1}, {0, 1, -1}, {-1, 1, -1}, {-1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {-1, 1, 1}}}, /* +-Z in -X,+Y */

    // Spanning cube (all 8 quadrants)
    {"SpanningCube", {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}}}
};


// Ray inputs
typedef struct {
    const char *label;
    point_t origin;
    vect_t direction;
} ray_input_t;

ray_input_t rays[] = {
    {"Ray_PosX", {-2, 0, 0}, {1, 0, 0}},
    {"Ray_NegX", {2, 0, 0}, {-1, 0, 0}},
    {"Ray_PosY", {0, -2, 0}, {0, 1, 0}},
    {"Ray_NegY", {0, 2, 0}, {0, -1, 0}},
    {"Ray_PosZ", {0, 0, -2}, {0, 0, 1}},
    {"Ray_NegZ", {0, 0, 2}, {0, 0, -1}},
    {"Ray_Diagonal", {-2, -2, -2}, {1, 1, 1}},
    {"Ray_GrazingEdge", {0.5, -2, 0.5}, {0, 1, 0}},
    {"Ray_GrazingVertex", {1, -1, -1}, {1, 1, 1}},
    {"Ray_Inside", {0.5, 0.5, 0.5}, {1, 1, 1}}
};


// Expected results (to be filled manually or programmatically based on geometry)
typedef struct {
    const char *arb8_label;
    const char *ray_label;
    int expected_hit; // 1 = hit, 0 = miss
} expected_result_t;

expected_result_t expected_results[] = {
    // Standard ARB8 Configurations
    {"StandardCube", "Ray_PosX", 1},
    {"StandardCube", "Ray_NegX", 1},
    {"StandardCube", "Ray_PosY", 1},
    {"StandardCube", "Ray_NegY", 1},
    {"StandardCube", "Ray_PosZ", 1},
    {"StandardCube", "Ray_NegZ", 1},
    {"StandardCube", "Ray_Diagonal", 1},
    {"StandardCube", "Ray_GrazingEdge", 1},
    {"StandardCube", "Ray_GrazingVertex", 1},
    {"StandardCube", "Ray_Inside", 1},

    // Slanted ARB8 Configurations
    {"SlantedARB8_1", "Ray_PosX", 1},
    {"SlantedARB8_1", "Ray_NegX", 1},
    {"SlantedARB8_1", "Ray_PosY", 1},
    {"SlantedARB8_1", "Ray_NegY", 1},
    {"SlantedARB8_1", "Ray_PosZ", 1},
    {"SlantedARB8_1", "Ray_NegZ", 1},
    {"SlantedARB8_1", "Ray_Diagonal", 1},
    {"SlantedARB8_1", "Ray_GrazingEdge", 1},
    {"SlantedARB8_1", "Ray_GrazingVertex", 1},
    {"SlantedARB8_1", "Ray_Inside", 1},

    {"SlantedARB8_2", "Ray_PosX", 1},
    {"SlantedARB8_2", "Ray_NegX", 1},
    {"SlantedARB8_2", "Ray_PosY", 1},
    {"SlantedARB8_2", "Ray_NegY", 1},
    {"SlantedARB8_2", "Ray_PosZ", 1},
    {"SlantedARB8_2", "Ray_NegZ", 1},
    {"SlantedARB8_2", "Ray_Diagonal", 1},
    {"SlantedARB8_2", "Ray_GrazingEdge", 1},
    {"SlantedARB8_2", "Ray_GrazingVertex", 1},
    {"SlantedARB8_2", "Ray_Inside", 1},

    {"SlantedARB8_3", "Ray_PosX", 1},
    {"SlantedARB8_3", "Ray_NegX", 1},
    {"SlantedARB8_3", "Ray_PosY", 1},
    {"SlantedARB8_3", "Ray_NegY", 1},
    {"SlantedARB8_3", "Ray_PosZ", 1},
    {"SlantedARB8_3", "Ray_NegZ", 1},
    {"SlantedARB8_3", "Ray_Diagonal", 1},
    {"SlantedARB8_3", "Ray_GrazingEdge", 1},
    {"SlantedARB8_3", "Ray_GrazingVertex", 1},
    {"SlantedARB8_3", "Ray_Inside", 1},

    // Irregular ARB8 Configurations
    {"IrregularARB8_1", "Ray_PosX", 1},
    {"IrregularARB8_1", "Ray_NegX", 1},
    {"IrregularARB8_1", "Ray_PosY", 1},
    {"IrregularARB8_1", "Ray_NegY", 1},
    {"IrregularARB8_1", "Ray_PosZ", 1},
    {"IrregularARB8_1", "Ray_NegZ", 1},
    {"IrregularARB8_1", "Ray_Diagonal", 1},
    {"IrregularARB8_1", "Ray_GrazingEdge", 1},
    {"IrregularARB8_1", "Ray_GrazingVertex", 1},
    {"IrregularARB8_1", "Ray_Inside", 1},

    {"IrregularARB8_2", "Ray_PosX", 1},
    {"IrregularARB8_2", "Ray_NegX", 1},
    {"IrregularARB8_2", "Ray_PosY", 1},
    {"IrregularARB8_2", "Ray_NegY", 1},
    {"IrregularARB8_2", "Ray_PosZ", 1},
    {"IrregularARB8_2", "Ray_NegZ", 1},
    {"IrregularARB8_2", "Ray_Diagonal", 1},
    {"IrregularARB8_2", "Ray_GrazingEdge", 1},
    {"IrregularARB8_2", "Ray_GrazingVertex", 1},
    {"IrregularARB8_2", "Ray_Inside", 1},

    {"IrregularARB8_3", "Ray_PosX", 1},
    {"IrregularARB8_3", "Ray_NegX", 1},
    {"IrregularARB8_3", "Ray_PosY", 1},
    {"IrregularARB8_3", "Ray_NegY", 1},
    {"IrregularARB8_3", "Ray_PosZ", 1},
    {"IrregularARB8_3", "Ray_NegZ", 1},
    {"IrregularARB8_3", "Ray_Diagonal", 1},
    {"IrregularARB8_3", "Ray_GrazingEdge", 1},
    {"IrregularARB8_3", "Ray_GrazingVertex", 1},
    {"IrregularARB8_3", "Ray_Inside", 1},

    // Unit Cubes in Quadrants
    {"UnitCube_Q1", "Ray_PosX", 1},
    {"UnitCube_Q1", "Ray_NegX", 0},
    {"UnitCube_Q1", "Ray_PosY", 1},
    {"UnitCube_Q1", "Ray_NegY", 0},
    {"UnitCube_Q1", "Ray_PosZ", 1},
    {"UnitCube_Q1", "Ray_NegZ", 0},
    {"UnitCube_Q1", "Ray_Diagonal", 1},
    {"UnitCube_Q1", "Ray_GrazingEdge", 1},
    {"UnitCube_Q1", "Ray_GrazingVertex", 1},
    {"UnitCube_Q1", "Ray_Inside", 1},

    {"UnitCube_Q2", "Ray_PosX", 0},
    {"UnitCube_Q2", "Ray_NegX", 1},
    {"UnitCube_Q2", "Ray_PosY", 1},
    {"UnitCube_Q2", "Ray_NegY", 0},
    {"UnitCube_Q2", "Ray_PosZ", 1},
    {"UnitCube_Q2", "Ray_NegZ", 0},
    {"UnitCube_Q2", "Ray_Diagonal", 1},
    {"UnitCube_Q2", "Ray_GrazingEdge", 1},
    {"UnitCube_Q2", "Ray_GrazingVertex", 1},
    {"UnitCube_Q2", "Ray_Inside", 1},

    {"UnitCube_Q3", "Ray_PosX", 0},
    {"UnitCube_Q3", "Ray_NegX", 0},
    {"UnitCube_Q3", "Ray_PosY", 0},
    {"UnitCube_Q3", "Ray_NegY", 1},
    {"UnitCube_Q3", "Ray_PosZ", 1},
    {"UnitCube_Q3", "Ray_NegZ", 0},
    {"UnitCube_Q3", "Ray_Diagonal", 1},
    {"UnitCube_Q3", "Ray_GrazingEdge", 1},
    {"UnitCube_Q3", "Ray_GrazingVertex", 1},
    {"UnitCube_Q3", "Ray_Inside", 1},

    {"UnitCube_Q8", "Ray_PosX", 0},
    {"UnitCube_Q8", "Ray_NegX", 1},
    {"UnitCube_Q8", "Ray_PosY", 0},
    {"UnitCube_Q8", "Ray_NegY", 1},
    {"UnitCube_Q8", "Ray_PosZ", 0},
    {"UnitCube_Q8", "Ray_NegZ", 1},
    {"UnitCube_Q8", "Ray_Diagonal", 1},
    {"UnitCube_Q8", "Ray_GrazingEdge", 1},
    {"UnitCube_Q8", "Ray_GrazingVertex", 1},
    {"UnitCube_Q8", "Ray_Inside", 1},

    // Spanning Cube
    {"SpanningCube", "Ray_PosX", 1},
    {"SpanningCube", "Ray_NegX", 1},
    {"SpanningCube", "Ray_PosY", 1},
    {"SpanningCube", "Ray_NegY", 1},
    {"SpanningCube", "Ray_PosZ", 1},
    {"SpanningCube", "Ray_NegZ", 1},
    {"SpanningCube", "Ray_Diagonal", 1},
    {"SpanningCube", "Ray_GrazingEdge", 1},
    {"SpanningCube", "Ray_GrazingVertex", 1},
    {"SpanningCube", "Ray_Inside", 1}
};


// Main execution and validation
int
main(int ac, char *av[])
{
    if (ac > 1) {
	bu_log("Usage: %s\n", av[0]);
	return 1;
    }

    for (size_t i = 0; i < sizeof(arb8_configs) / sizeof(arb8_config_t); i++) {
        const arb8_config_t *arb = &arb8_configs[i];
        printf("Testing ARB8: %s\n", arb->label);

        for (size_t j = 0; j < sizeof(rays) / sizeof(ray_input_t); j++) {
            const ray_input_t *ray = &rays[j];
            printf("  Testing Ray: %s\n", ray->label);

            struct application ap = {0};
            VSET(ap.a_ray.r_pt, ray->origin[X], ray->origin[Y], ray->origin[Z]);
            VSET(ap.a_ray.r_dir, ray->direction[X], ray->direction[Y], ray->direction[Z]);

            // TODO - call rt_arb_shot() here, set hit/miss result
	    struct soltab stp;
	    struct xray rp;
	    struct seg seghead;
	    (void)(*OBJ[ID_ARB8].ft_shot)(&stp, &rp, &ap, &seghead);

            int hit = 0; // Placeholder

            // Validate against expected result
            int expected_hit = -1;
            for (size_t k = 0; k < sizeof(expected_results) / sizeof(expected_result_t); k++) {
                if (bu_strcmp(expected_results[k].arb8_label, arb->label) == 0 &&
                    bu_strcmp(expected_results[k].ray_label, ray->label) == 0) {
                    expected_hit = expected_results[k].expected_hit;
                    break;
                }
            }

            if (expected_hit != -1) {
                if (hit == expected_hit) {
                    printf("    PASS\n");
                } else {
                    printf("    FAIL (expected %d, got %d)\n", expected_hit, hit);
                }
            } else {
                printf("    SKIPPED (no expectation defined)\n");
            }
        }
    }

    return 0;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8 cino=N-s
 */
