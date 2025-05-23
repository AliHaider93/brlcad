/*                           E L L . C
 * BRL-CAD
 *
 * Copyright (c) 1985-2025 United States Government as represented by
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
/** @addtogroup primitives */
/** @{ */
/** @file primitives/ell/ell.c
 *
 * Intersect a ray with a Generalized Ellipsoid.
 *
 */

#include "common.h"

#include <stddef.h>
#include <string.h>
#include <math.h>
#include "bio.h"

#include "bu/cv.h"
#include "vmath.h"
#include "rt/db4.h"
#include "nmg.h"
#include "rt/geom.h"
#include "raytrace.h"

#include "../../librt_private.h"


extern int rt_sph_prep(struct soltab *stp, struct rt_db_internal *ip,
		       struct rt_i *rtip);

const struct bu_structparse rt_ell_parse[] = {
    { "%f", 3, "V", bu_offsetofarray(struct rt_ell_internal, v, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "A", bu_offsetofarray(struct rt_ell_internal, a, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "B", bu_offsetofarray(struct rt_ell_internal, b, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "C", bu_offsetofarray(struct rt_ell_internal, c, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { {'\0', '\0', '\0', '\0'}, 0, (char *)NULL, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL }
};


static void nmg_sphere_face_snurb(struct faceuse *fu, const matp_t m);

/*
 * Algorithm:
 *
 * Given V, A, B, and C, there is a set of points on this ellipsoid
 *
 * { (x, y, z) | (x, y, z) is on ellipsoid defined by V, A, B, C }
 *
 * Through a series of Affine Transformations, this set will be
 * transformed into a set of points on a unit sphere at the origin
 *
 * { (x', y', z') | (x', y', z') is on Sphere at origin }
 *
 * The transformation from X to X' is accomplished by:
 *
 * X' = S(R(X - V))
 *
 * where R(X) =  (A/(|A|))
 *  		 (B/(|B|)) . X
 *  		 (C/(|C|))
 *
 * and S(X) =	(1/|A|   0     0)
 *  		(0    1/|B|   0) . X
 *  		(0      0   1/|C|)
 *
 * To find the intersection of a line with the ellipsoid, consider the
 * parametric line L:
 *
 * L : { P(n) | P + t(n) . D }
 *
 * Call W the actual point of intersection between L and the
 * ellipsoid.  Let W' be the point of intersection between L' and the
 * unit sphere.
 *
 * L' : { P'(n) | P' + t(n) . D' }
 *
 * W = invR(invS(W')) + V
 *
 * Where W' = k D' + P'.
 *
 * Let dp = D' dot P'
 * Let dd = D' dot D'
 * Let pp = P' dot P'
 *
 * and k = [ -dp +/- sqrt(dp*dp - dd * (pp - 1)) ] / dd
 * which is constant.
 *
 * Now, D' = S(R(D))
 * and  P' = S(R(P - V))
 *
 * Substituting,
 *
 *  W = V + invR(invS[ k *(S(R(D))) + S(R(P - V)) ])
 *    = V + invR((k * R(D)) + R(P - V))
 *    = V + k * D + P - V
 *    = k * D + P
 *
 * Note that ``k'' is constant, and is the same in the formulations
 * for both W and W'.
 *
 * NORMALS.  Given the point W on the ellipsoid, what is the vector
 * normal to the tangent plane at that point?
 *
 * Map W onto the unit sphere, i.e.:  W' = S(R(W - V)).
 *
 * Plane on unit sphere at W' has a normal vector of the same
 * value(!).  N' = W'
 *
 * The plane transforms back to the tangent plane at W, and this new
 * plane (on the ellipsoid) has a normal vector of N, viz:
 *
 * N = inverse[ transpose(inverse[ S o R ]) ] (N')
 *
 * because if H is perpendicular to plane Q, and matrix M maps from Q
 * to Q', then inverse[ transpose(M) ] (H) is perpendicular to Q'.
 * Here, H and Q are in "prime space" with the unit sphere.  [Somehow,
 * the notation here is backwards].  So, the mapping matrix M =
 * inverse(S o R), because S o R maps from normal space to the unit
 * sphere.
 *
 * N = inverse[ transpose(inverse[ S o R ]) ] (N')
 *   = inverse[ transpose(invR o invS) ] (N')
 *   = inverse[ transpose(invS) o transpose(invR) ] (N')
 *   = inverse[ inverse(S) o R ] (N')
 *   = invR o S (N')
 *
 *   = invR o S (W')
 *   = invR(S(S(R(W - V))))
 *
 * because inverse(R) = transpose(R), so R = transpose(invR),
 * and S = transpose(S).
 *
 * Note that the normal vector N produced above will not have unit
 * length.
 */

struct ell_specific {
    vect_t ell_V;	/* Vector to center of ellipsoid */
    vect_t ell_Au;	/* unit-length A vector */
    vect_t ell_Bu;
    vect_t ell_Cu;
    vect_t ell_invsq;	/* [ 1/(|A|**2), 1/(|B|**2), 1/(|C|**2) ] */
    mat_t ell_SoR;	/* Scale(Rot(vect)) */
    mat_t ell_invRSSR;	/* invRot(Scale(Scale(Rot(vect)))) */
};


#define ELL_NULL ((struct ell_specific *)0)

#ifdef USE_OPENCL
/* largest data members first */
struct clt_ell_specific {
    cl_double ell_V[3];         /* Vector to center of ellipsoid */
    cl_double ell_SoR[16];      /* Scale(Rot(vect)) */
    cl_double ell_invRSSR[16];  /* invRot(Scale(Scale(Rot(vect)))) */
};

size_t
clt_ell_pack(struct bu_pool *pool, struct soltab *stp)
{
    struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;
    struct clt_ell_specific *args;

    const size_t size = sizeof(*args);
    args = (struct clt_ell_specific*)bu_pool_alloc(pool, 1, size);

    VMOVE(args->ell_V, ell->ell_V);
    MAT_COPY(args->ell_SoR, ell->ell_SoR);
    MAT_COPY(args->ell_invRSSR, ell->ell_invRSSR);
    return size;
}

#endif /* USE_OPENCL */


/**
 * Compute the bounding RPP for an ellipsoid
 */
int
rt_ell_bbox(struct rt_db_internal *ip, point_t *min, point_t *max, const struct bn_tol *UNUSED(tol)) {
    vect_t w1, w2, P;
    vect_t Au, Bu, Cu;	/* A, B, C with unit length */
    fastf_t magsq_a, magsq_b, magsq_c, f;
    mat_t R;
    struct rt_ell_internal *eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    magsq_a = MAGSQ(eip->a);
    magsq_b = MAGSQ(eip->b);
    magsq_c = MAGSQ(eip->c);

    /* Try a shortcut - if this is a sphere, the calculation simplifies */
    /* Check whether |A|, |B|, and |C| are nearly equal */
    if (EQUAL(magsq_a, magsq_b) && EQUAL(magsq_a, magsq_c)) {
	fastf_t sph_rad = sqrt(magsq_a);
	(*min)[X] = eip->v[X] - sph_rad;
	(*max)[X] = eip->v[X] + sph_rad;
	(*min)[Y] = eip->v[Y] - sph_rad;
	(*max)[Y] = eip->v[Y] + sph_rad;
	(*min)[Z] = eip->v[Z] - sph_rad;
	(*max)[Z] = eip->v[Z] + sph_rad;
	return 0;
    }

    /* Create unit length versions of A, B, C */
    f = 1.0/sqrt(magsq_a);
    VSCALE(Au, eip->a, f);
    f = 1.0/sqrt(magsq_b);
    VSCALE(Bu, eip->b, f);
    f = 1.0/sqrt(magsq_c);
    VSCALE(Cu, eip->c, f);

    MAT_IDN(R);
    VMOVE(&R[0], Au);
    VMOVE(&R[4], Bu);
    VMOVE(&R[8], Cu);

    /* Compute bounding RPP */
    VSET(w1, magsq_a, magsq_b, magsq_c);

    /* X */
    VSET(P, 1.0, 0, 0);		/* bounding plane normal */
    MAT3X3VEC(w2, R, P);	/* map plane to local coord syst */
    VELMUL(w2, w2, w2);		/* square each term */
    f = VDOT(w1, w2);
    f = sqrt(f);
    (*min)[X] = eip->v[X] - f;	/* V.P +/- f */
    (*max)[X] = eip->v[X] + f;

    /* Y */
    VSET(P, 0, 1.0, 0);		/* bounding plane normal */
    MAT3X3VEC(w2, R, P);	/* map plane to local coord syst */
    VELMUL(w2, w2, w2);		/* square each term */
    f = VDOT(w1, w2);
    f = sqrt(f);
    (*min)[Y] = eip->v[Y] - f;	/* V.P +/- f */
    (*max)[Y] = eip->v[Y] + f;

    /* Z */
    VSET(P, 0, 0, 1.0);		/* bounding plane normal */
    MAT3X3VEC(w2, R, P);	/* map plane to local coord syst */
    VELMUL(w2, w2, w2);		/* square each term */
    f = VDOT(w1, w2);
    f = sqrt(f);
    (*min)[Z] = eip->v[Z] - f;	/* V.P +/- f */
    (*max)[Z] = eip->v[Z] + f;
    return 0;
}


/**
 * Given a pointer to a GED database record, and a transformation
 * matrix, determine if this is a valid ellipsoid, and if so,
 * precompute various terms of the formula.
 *
 * Returns -
 * 0 ELL is OK
 * !0 Error in description
 *
 * Implicit return -
 * A struct ell_specific is created, and its address is stored in
 * stp->st_specific for use by rt_ell_shot().
 */
int
rt_ell_prep(struct soltab *stp, struct rt_db_internal *ip, struct rt_i *rtip)
{
    register struct ell_specific *ell;
    struct rt_ell_internal *eip;
    fastf_t magsq_a, magsq_b, magsq_c;
    mat_t R;
    mat_t Rinv;
    mat_t SS;
    mat_t mtemp;
    vect_t Au, Bu, Cu;	/* A, B, C with unit length */
    fastf_t f;

    eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    /*
     * For a fast way out, hand this solid off to the SPH routine.  If
     * it takes it, then there is nothing to do, otherwise the solid
     * is an ELL.
     */
    if (rt_sph_prep(stp, ip, rtip) == 0) {
	return 0;		/* OK */
    }

    /* Validate that |A| > 0, |B| > 0, |C| > 0 */
    magsq_a = MAGSQ(eip->a);
    magsq_b = MAGSQ(eip->b);
    magsq_c = MAGSQ(eip->c);

    if (magsq_a < rtip->rti_tol.dist_sq || magsq_b < rtip->rti_tol.dist_sq || magsq_c < rtip->rti_tol.dist_sq) {
	bu_log("rt_ell_prep():  ell(%s) zero length A(%g), B(%g), or C(%g) vector\n",
	       stp->st_name, magsq_a, magsq_b, magsq_c);
	return 1;		/* BAD */
    }

    /* Create unit length versions of A, B, C */
    f = 1.0/sqrt(magsq_a);
    VSCALE(Au, eip->a, f);
    f = 1.0/sqrt(magsq_b);
    VSCALE(Bu, eip->b, f);
    f = 1.0/sqrt(magsq_c);
    VSCALE(Cu, eip->c, f);

    /* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
    f = VDOT(Au, Bu);
    if (! NEAR_ZERO(f, rtip->rti_tol.dist)) {
	bu_log("rt_ell_prep():  ell(%s) A not perpendicular to B, f=%f\n", stp->st_name, f);
	return 1;		/* BAD */
    }
    f = VDOT(Bu, Cu);
    if (! NEAR_ZERO(f, rtip->rti_tol.dist)) {
	bu_log("rt_ell_prep():  ell(%s) B not perpendicular to C, f=%f\n", stp->st_name, f);
	return 1;		/* BAD */
    }
    f = VDOT(Au, Cu);
    if (! NEAR_ZERO(f, rtip->rti_tol.dist)) {
	bu_log("rt_ell_prep():  ell(%s) A not perpendicular to C, f=%f\n", stp->st_name, f);
	return 1;		/* BAD */
    }

    /* Solid is OK, compute constant terms now */
    BU_GET(ell, struct ell_specific);
    stp->st_specific = (void *)ell;

    VMOVE(ell->ell_V, eip->v);

    VSET(ell->ell_invsq, 1.0/magsq_a, 1.0/magsq_b, 1.0/magsq_c);
    VMOVE(ell->ell_Au, Au);
    VMOVE(ell->ell_Bu, Bu);
    VMOVE(ell->ell_Cu, Cu);

    MAT_IDN(ell->ell_SoR);
    MAT_IDN(R);

    /* Compute R and Rinv matrices */
    VMOVE(&R[0], Au);
    VMOVE(&R[4], Bu);
    VMOVE(&R[8], Cu);
    bn_mat_trn(Rinv, R);			/* inv of rot mat is trn */

    /* Compute SoS (Affine transformation) */
    MAT_IDN(SS);
    SS[ 0] = ell->ell_invsq[0];
    SS[ 5] = ell->ell_invsq[1];
    SS[10] = ell->ell_invsq[2];

    /* Compute invRSSR */
    bn_mat_mul(mtemp, SS, R);
    bn_mat_mul(ell->ell_invRSSR, Rinv, mtemp);

    /* Compute SoR */
    VSCALE(&ell->ell_SoR[0], eip->a, ell->ell_invsq[0]);
    VSCALE(&ell->ell_SoR[4], eip->b, ell->ell_invsq[1]);
    VSCALE(&ell->ell_SoR[8], eip->c, ell->ell_invsq[2]);

    /* Compute bounding sphere */
    VMOVE(stp->st_center, eip->v);
    f = magsq_a;
    if (magsq_b > f)
	f = magsq_b;
    if (magsq_c > f)
	f = magsq_c;
    stp->st_aradius = stp->st_bradius = sqrt(f);

    /* Compute bounding RPP */
    if (stp->st_meth->ft_bbox(ip, &(stp->st_min), &(stp->st_max), &(rtip->rti_tol))) return 1;
    return 0;			/* OK */
}


void
rt_ell_print(register const struct soltab *stp)
{
    register struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;

    VPRINT("V", ell->ell_V);
    bn_mat_print("S o R", ell->ell_SoR);
    bn_mat_print("invRSSR", ell->ell_invRSSR);
}


/**
 * Intersect a ray with an ellipsoid, where all constant terms have
 * been precomputed by rt_ell_prep().  If an intersection occurs, a
 * struct seg will be acquired and filled in.
 *
 * Returns -
 * 0 MISS
 * >0 HIT
 */
int
rt_ell_shot(struct soltab *stp, register struct xray *rp, struct application *ap, struct seg *seghead)
{
    register struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;
    register struct seg *segp;

    vect_t dprime;	/* D' */
    vect_t pprime;	/* P' */
    vect_t xlated;	/* translated vector */
    fastf_t dp, dd;	/* D' dot P', D' dot D' */
    fastf_t k1, k2;	/* distance constants of solution */
    fastf_t root;	/* root of radical */

    /* out, Mat, vect */
    MAT4X3VEC(dprime, ell->ell_SoR, rp->r_dir);
    VSUB2(xlated, rp->r_pt, ell->ell_V);
    MAT4X3VEC(pprime, ell->ell_SoR, xlated);

    dp = VDOT(dprime, pprime);
    dd = VDOT(dprime, dprime);

    if ((root = dp*dp - dd * (VDOT(pprime, pprime)-1.0)) < 0)
	return 0;		/* No hit */
    root = sqrt(root);

    RT_GET_SEG(segp, ap->a_resource);
    segp->seg_stp = stp;
    if ((k1=(-dp+root)/dd) <= (k2=(-dp-root)/dd)) {
	/* k1 is entry, k2 is exit */
	segp->seg_in.hit_dist = k1;
	segp->seg_out.hit_dist = k2;
    } else {
	/* k2 is entry, k1 is exit */
	segp->seg_in.hit_dist = k2;
	segp->seg_out.hit_dist = k1;
    }
    segp->seg_in.hit_surfno = 0;
    segp->seg_out.hit_surfno = 0;
    BU_LIST_INSERT(&(seghead->l), &(segp->l));
    return 2;			/* HIT */
}


#define RT_ELL_SEG_MISS(SEG)	(SEG).seg_stp=RT_SOLTAB_NULL
/**
 * This is the Becker vector version.
 */
void
rt_ell_vshot(struct soltab **stp, struct xray **rp, struct seg *segp, int n, struct application *ap)
/* An array of solid pointers */
/* An array of ray pointers */
/* array of segs (results returned) */
/* Number of ray/object pairs */

{
    register int i;
    register struct ell_specific *ell;

    vect_t dprime;		/* D' */
    vect_t pprime;		/* P' */
    vect_t xlated;		/* translated vector */
    fastf_t dp, dd;		/* D' dot P', D' dot D' */
    fastf_t k1, k2;		/* distance constants of solution */
    fastf_t root;		/* root of radical */

    if (ap) RT_CK_APPLICATION(ap);

    /* for each ray/ellipse pair */
    for (i = 0; i < n; i++) {
	if (stp[i] == 0) continue; /* stp[i] == 0 signals skip ray */

	ell = (struct ell_specific *)stp[i]->st_specific;

	MAT4X3VEC(dprime, ell->ell_SoR, rp[i]->r_dir);
	VSUB2(xlated, rp[i]->r_pt, ell->ell_V);
	MAT4X3VEC(pprime, ell->ell_SoR, xlated);

	dp = VDOT(dprime, pprime);
	dd = VDOT(dprime, dprime);

	if ((root = dp*dp - dd * (VDOT(pprime, pprime)-1.0)) < 0) {
	    RT_ELL_SEG_MISS(segp[i]);		/* No hit */
	} else {
	    root = sqrt(root);

	    segp[i].seg_stp = stp[i];

	    if ((k1=(-dp+root)/dd) <= (k2=(-dp-root)/dd)) {
		/* k1 is entry, k2 is exit */
		segp[i].seg_in.hit_dist = k1;
		segp[i].seg_out.hit_dist = k2;
	    } else {
		/* k2 is entry, k1 is exit */
		segp[i].seg_in.hit_dist = k2;
		segp[i].seg_out.hit_dist = k1;
	    }
	    segp[i].seg_in.hit_surfno = 0;
	    segp[i].seg_out.hit_surfno = 0;
	}
    }
}


/**
 * Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_ell_norm(register struct hit *hitp, struct soltab *stp, register struct xray *rp)
{
    register struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;
    vect_t xlated;
    fastf_t scale;

    VJOIN1(hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir);
    VSUB2(xlated, hitp->hit_point, ell->ell_V);
    MAT4X3VEC(hitp->hit_normal, ell->ell_invRSSR, xlated);
    scale = 1.0 / MAGNITUDE(hitp->hit_normal);
    VSCALE(hitp->hit_normal, hitp->hit_normal, scale);

    /* tuck away this scale for the curvature routine */
    hitp->hit_vpriv[X] = scale;
}


/**
 * Return the curvature of the ellipsoid.
 */
void
rt_ell_curve(register struct curvature *cvp, register struct hit *hitp, struct soltab *stp)
{
    register struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;
    vect_t u, v;		/* basis vectors (with normal) */
    vect_t vec1, vec2;		/* eigen vectors */
    vect_t tmp;
    fastf_t a, b, c, scale;

    /*
     * choose a tangent plane coordinate system (u, v, normal) form a
     * right-handed triple
     */
    bn_vec_ortho(u, hitp->hit_normal);
    VCROSS(v, hitp->hit_normal, u);

    /* get the saved away scale factor */
    scale = - hitp->hit_vpriv[X];

    /* find the second fundamental form */
    MAT4X3VEC(tmp, ell->ell_invRSSR, u);
    a = VDOT(u, tmp) * scale;
    b = VDOT(v, tmp) * scale;
    MAT4X3VEC(tmp, ell->ell_invRSSR, v);
    c = VDOT(v, tmp) * scale;

    bn_eigen2x2(&cvp->crv_c1, &cvp->crv_c2, vec1, vec2, a, b, c);
    VCOMB2(cvp->crv_pdir, vec1[X], u, vec1[Y], v);
    VUNITIZE(cvp->crv_pdir);
}


/**
 * For a hit on the surface of an ELL, return the (u, v) coordinates
 * of the hit point, 0 <= u, v <= 1.
 *
 * u = azimuth
 * v = elevation
 */
void
rt_ell_uv(struct application *ap, struct soltab *stp, register struct hit *hitp, register struct uvcoord *uvp)
{
    register struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;
    vect_t work;
    vect_t pprime;
    fastf_t r;

    /* hit_point is on surface; project back to unit sphere, creating
     * a vector from vertex to hit point which always has length=1.0
     */
    VSUB2(work, hitp->hit_point, ell->ell_V);
    MAT4X3VEC(pprime, ell->ell_SoR, work);
    /* Assert that pprime has unit length */

    /* U is azimuth, atan() range: -pi to +pi */
    uvp->uv_u = bn_atan2(pprime[Y], pprime[X]) * M_1_2PI;
    if (uvp->uv_u < 0)
	uvp->uv_u += 1.0;
    /* V is elevation, atan() range: -pi/2 to +pi/2, because sqrt()
     * ensures that X parameter is always >0
     */
    uvp->uv_v = bn_atan2(pprime[Z],
			 sqrt(pprime[X] * pprime[X] + pprime[Y] * pprime[Y])) *
	M_1_PI + 0.5;

    /* approximation: r / (circumference, 2 * pi * aradius) */
    r = ap->a_rbeam + ap->a_diverge * hitp->hit_dist;
    uvp->uv_du = uvp->uv_dv =
	M_1_2PI * r / stp->st_aradius;
}


void
rt_ell_free(register struct soltab *stp)
{
    register struct ell_specific *ell =
	(struct ell_specific *)stp->st_specific;

    BU_PUT(ell, struct ell_specific);
}


/**
 * Also used by the TGC code
 */
#define ELLOUT(n) ov+(n-1)*3
void
rt_ell_16pnts(fastf_t *ov,
	     fastf_t *V,
	     fastf_t *A,
	     fastf_t *B)
{
    static fastf_t c, d, e, f, g, h;

    e = h = .92388;	/* cos(22.5 degrees) */
    c = d = M_SQRT1_2;	/* cos(45 degrees) */
    g = f = .382683;	/* cos(67.5 degrees) */

    /*
     * For angle theta, compute surface point as
     *
     * V + cos(theta) * A + sin(theta) * B
     *
     * note that sin(theta) is cos(90-theta), with arguments in degrees.
     */

    VADD2(ELLOUT(1), V, A);
    VJOIN2(ELLOUT(2), V, e, A, f, B);
    VJOIN2(ELLOUT(3), V, c, A, d, B);
    VJOIN2(ELLOUT(4), V, g, A, h, B);
    VADD2(ELLOUT(5), V, B);
    VJOIN2(ELLOUT(6), V, -g, A, h, B);
    VJOIN2(ELLOUT(7), V, -c, A, d, B);
    VJOIN2(ELLOUT(8), V, -e, A, f, B);
    VSUB2(ELLOUT(9), V, A);
    VJOIN2(ELLOUT(10), V, -e, A, -f, B);
    VJOIN2(ELLOUT(11), V, -c, A, -d, B);
    VJOIN2(ELLOUT(12), V, -g, A, -h, B);
    VSUB2(ELLOUT(13), V, B);
    VJOIN2(ELLOUT(14), V, g, A, -h, B);
    VJOIN2(ELLOUT(15), V, c, A, -d, B);
    VJOIN2(ELLOUT(16), V, e, A, -f, B);
}

struct ell_draw_configuration {
    struct bu_list *vlfree;
    struct bu_list *vhead;
    vect_t ell_center;
    vect_t ell_axis_vector_a;
    vect_t ell_axis_vector_b;
    vect_t ell_travel_vector;
    int num_cross_sections;
    int points_per_section;
};

struct ellipse_cross_section {
    vect_t a;
    vect_t b;
    vect_t translation;
    double ellipsoid_travel_axis_mag;
    double ellipsoid_travel_axis_position;
};

static double
cross_section_axis_magnitude(
	struct ellipse_cross_section cross_section,
	double target_axis_mag)
{
    double travel_mag, travel_pos;
    double fixed_term;

    travel_mag = cross_section.ellipsoid_travel_axis_mag;
    travel_pos = cross_section.ellipsoid_travel_axis_position;

    fixed_term = (travel_pos * travel_pos) / (travel_mag * travel_mag);

    return sqrt(target_axis_mag * target_axis_mag * (1.0 - fixed_term));
}

/* Draws elliptical cross-sections along an ell vector (the "travel vector").
 *
 * algorithm overview:
 *  1 Pretend the ellipsoid is at the origin with the ell vectors axis-aligned.
 *  2 Step along the travel vector t from -mag(t) to mag(t).
 *  3 Calculate the axis vectors for the cross-section ellipse at that step.
 *  4 Draw multiple points along the ellipse to define the cross-section,
 *    using a parametric vector equation to calculate ellipse points for
 *    given radian values.
 */
static void
draw_cross_sections_along_ell_vector(struct ell_draw_configuration config)
{
    int i;
    vect_t ell_a, ell_b, ell_t;
    double ellipse_a_mag, ellipse_b_mag;
    double ell_a_mag, ell_b_mag, ell_t_mag;
    double num_cross_sections, points_per_section;
    double position_step;
    struct ellipse_cross_section cross_section = {VINIT_ZERO, VINIT_ZERO, VINIT_ZERO, 0.0, 0.0};

    VMOVE(ell_a, config.ell_axis_vector_a);
    VMOVE(ell_b, config.ell_axis_vector_b);
    VMOVE(ell_t, config.ell_travel_vector);

    ell_a_mag = MAGNITUDE(ell_a);
    ell_b_mag = MAGNITUDE(ell_b);
    ell_t_mag = MAGNITUDE(ell_t);

    points_per_section = config.points_per_section;
    num_cross_sections = config.num_cross_sections;

    position_step = 2.0 * ell_t_mag / (num_cross_sections + 1);

    cross_section.ellipsoid_travel_axis_mag = ell_t_mag;
    cross_section.ellipsoid_travel_axis_position = -ell_t_mag;
    for (i = 0; i < num_cross_sections; ++i) {
	cross_section.ellipsoid_travel_axis_position += position_step;

	ellipse_a_mag = cross_section_axis_magnitude(cross_section, ell_a_mag);
	ellipse_b_mag = cross_section_axis_magnitude(cross_section, ell_b_mag);

	VSCALE(cross_section.a, ell_a, ellipse_a_mag / ell_a_mag);
	VSCALE(cross_section.b, ell_b, ellipse_b_mag / ell_b_mag);
	VSCALE(cross_section.translation, ell_t,
		cross_section.ellipsoid_travel_axis_position / ell_t_mag);
	VADD2(cross_section.translation, cross_section.translation, config.ell_center);

	plot_ellipse(config.vlfree, config.vhead, cross_section.translation, cross_section.a,
		     cross_section.b, points_per_section);
    }
}

/* decide how many ellipse points are needed to satisfy point spacing */
static int
ell_ellipse_points(
	const struct rt_ell_internal *ell,
	fastf_t point_spacing)
{
    fastf_t avg_radius, avg_circumference;
    fastf_t ell_mag_a, ell_mag_b, ell_mag_c;

    ell_mag_a = MAGNITUDE(ell->a);
    ell_mag_b = MAGNITUDE(ell->b);
    ell_mag_c = MAGNITUDE(ell->c);

    avg_radius = (ell_mag_a + ell_mag_b + ell_mag_c) / 3.0;
    avg_circumference = M_2PI * avg_radius;

    return avg_circumference / point_spacing;
}

int
rt_ell_adaptive_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct bn_tol *tol, const struct bview *v, fastf_t s_size)
{
    struct ell_draw_configuration config;
    struct rt_ell_internal *eip;

    BU_CK_LIST_HEAD(vhead);
    RT_CK_DB_INTERNAL(ip);
    struct bu_list *vlfree = &rt_vlfree;
    eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    fastf_t point_spacing = solid_point_spacing(v, s_size);

    config.vlfree = vlfree;
    config.vhead = vhead;
    VMOVE(config.ell_center, eip->v);

    config.points_per_section = ell_ellipse_points(eip, point_spacing);

    if (config.points_per_section < 4) {
	BV_ADD_VLIST(vlfree, vhead, eip->v, BV_VLIST_POINT_DRAW);
	return 0;
    }

    config.num_cross_sections = primitive_curve_count(ip, tol, v->gv_s->curve_scale, s_size);

    VMOVE(config.ell_travel_vector, eip->a);
    VMOVE(config.ell_axis_vector_a, eip->b);
    VMOVE(config.ell_axis_vector_b, eip->c);
    draw_cross_sections_along_ell_vector(config);

    config.num_cross_sections = 1;

    VMOVE(config.ell_travel_vector, eip->b);
    VMOVE(config.ell_axis_vector_a, eip->a);
    VMOVE(config.ell_axis_vector_b, eip->c);
    draw_cross_sections_along_ell_vector(config);

    VMOVE(config.ell_travel_vector, eip->c);
    VMOVE(config.ell_axis_vector_a, eip->a);
    VMOVE(config.ell_axis_vector_b, eip->b);
    draw_cross_sections_along_ell_vector(config);

    return 0;
}

int
rt_ell_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct bg_tess_tol *UNUSED(ttol), const struct bn_tol *UNUSED(tol), const struct bview *UNUSED(info))
{
    register int i;
    struct rt_ell_internal *eip;
    fastf_t top[16*3];
    fastf_t middle[16*3];
    fastf_t bottom[16*3];

    BU_CK_LIST_HEAD(vhead);
    RT_CK_DB_INTERNAL(ip);
    struct bu_list *vlfree = &rt_vlfree;
    eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    rt_ell_16pnts(top, eip->v, eip->a, eip->b);
    rt_ell_16pnts(bottom, eip->v, eip->b, eip->c);
    rt_ell_16pnts(middle, eip->v, eip->a, eip->c);

    BV_ADD_VLIST(vlfree, vhead, &top[15*ELEMENTS_PER_VECT], BV_VLIST_LINE_MOVE);
    for (i = 0; i < 16; i++) {
	BV_ADD_VLIST(vlfree, vhead, &top[i*ELEMENTS_PER_VECT], BV_VLIST_LINE_DRAW);
    }

    BV_ADD_VLIST(vlfree, vhead, &bottom[15*ELEMENTS_PER_VECT], BV_VLIST_LINE_MOVE);
    for (i = 0; i < 16; i++) {
	BV_ADD_VLIST(vlfree, vhead, &bottom[i*ELEMENTS_PER_VECT], BV_VLIST_LINE_DRAW);
    }

    BV_ADD_VLIST(vlfree, vhead, &middle[15*ELEMENTS_PER_VECT], BV_VLIST_LINE_MOVE);
    for (i = 0; i < 16; i++) {
	BV_ADD_VLIST(vlfree, vhead, &middle[i*ELEMENTS_PER_VECT], BV_VLIST_LINE_DRAW);
    }

    return 0;
}


/* Vertices of a unit octahedron.  These need to be organized properly
 * to give reasonable normals.
 *
 * static struct usvert {
 * 	int a;
 * 	int b;
 * 	int c;
 * } octahedron[8] = {
 *     { XPLUS, ZPLUS, YPLUS },
 *     { YPLUS, ZPLUS, XMIN  },
 *     { XMIN , ZPLUS, YMIN  },
 *     { YMIN , ZPLUS, XPLUS },
 *     { XPLUS, YPLUS, ZMIN  },
 *     { YPLUS, XMIN , ZMIN  },
 *     { XMIN , YMIN , ZMIN  },
 *     { YMIN , XPLUS, ZMIN  }
 * };
 */
struct ell_state {
    struct shell *s;
    struct rt_ell_internal *eip;
    mat_t invRinvS;
    mat_t invRoS;
    fastf_t theta_tol;
};


struct ell_vert_strip {
    int nverts_per_strip;
    int nverts;
    struct vertex **vp;
    vect_t *norms;
    int nfaces;
    struct faceuse **fu;
};


/**
 * Tessellate an ellipsoid.
 *
 * The strategy is based upon the approach of Jon Leech 3/24/89, from
 * program "sphere", which generates a polygon mesh approximating a
 * sphere by recursive subdivision. First approximation is an
 * octahedron; each level of refinement increases the number of
 * polygons by a factor of 4.  Level 3 (128 polygons) is a good
 * tradeoff if gouraud shading is used to render the database.
 *
 * At the start, points ABC lie on surface of the unit sphere.  Pick
 * DEF as the midpoints of the three edges of ABC.  Normalize the new
 * points to lie on surface of the unit sphere.
 *
 *	  1
 *	  B
 *	 /\
 *   3  /  \  4
 *   D /____\ E
 *    /\    /\
 *   /	\  /  \
 *  /____\/____\
 * A      F     C
 * 0      5     2
 *
 * Returns -
 * -1 failure
 * 0 OK.  *r points to nmgregion that holds this tessellation.
 */
int
rt_ell_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *tol)
{
    mat_t R;
    mat_t S;
    mat_t invR;
    mat_t invS;
    vect_t Au, Bu, Cu;	/* A, B, C with unit length */
    fastf_t Alen, Blen, Clen;
    fastf_t invAlen, invBlen, invClen;
    fastf_t magsq_a, magsq_b, magsq_c;
    fastf_t f;
    struct ell_state state;
    register int i;
    fastf_t radius;
    int nsegs;
    int nstrips;
    struct ell_vert_strip *strips;
    int j;
    struct vertex **vertp[4];
    int faceno;
    int stripno;
    int boff;		/* base offset */
    int toff;		/* top offset */
    int blim;		/* base subscript limit */
    int tlim;		/* top subscript limit */
    fastf_t dtol;	/* Absolutized relative tolerance */

    RT_CK_DB_INTERNAL(ip);
    state.eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(state.eip);

    /* Validate that |A| > 0, |B| > 0, |C| > 0 */
    magsq_a = MAGSQ(state.eip->a);
    magsq_b = MAGSQ(state.eip->b);
    magsq_c = MAGSQ(state.eip->c);
    if (magsq_a < tol->dist_sq || magsq_b < tol->dist_sq || magsq_c < tol->dist_sq) {
	bu_log("rt_ell_tess():  zero length A, B, or C vector\n");
	return -2;		/* BAD */
    }

    /* Create unit length versions of A, B, C */
    Alen = sqrt(magsq_a);
    Blen = sqrt(magsq_b);
    Clen = sqrt(magsq_c);
    invAlen = 1.0/Alen;
    invBlen = 1.0/Blen;
    invClen = 1.0/Clen;
    VSCALE(Au, state.eip->a, invAlen);
    VSCALE(Bu, state.eip->b, invBlen);
    VSCALE(Cu, state.eip->c, invClen);

    /* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
    f = VDOT(Au, Bu);
    if (! NEAR_ZERO(f, tol->dist)) {
	bu_log("rt_ell_tess():  A not perpendicular to B, f=%f\n", f);
	return -3;		/* BAD */
    }
    f = VDOT(Bu, Cu);
    if (! NEAR_ZERO(f, tol->dist)) {
	bu_log("rt_ell_tess():  B not perpendicular to C, f=%f\n", f);
	return -3;		/* BAD */
    }
    f = VDOT(Au, Cu);
    if (! NEAR_ZERO(f, tol->dist)) {
	bu_log("rt_ell_tess():  A not perpendicular to C, f=%f\n", f);
	return -3;		/* BAD */
    }

    {
	vect_t axb;
	VCROSS(axb, Au, Bu);
	f = VDOT(axb, Cu);
	if (f < 0) {
	    VREVERSE(Cu, Cu);
	    VREVERSE(state.eip->c, state.eip->c);
	}
    }

    /* Compute R and Rinv matrices */
    MAT_IDN(R);
    VMOVE(&R[0], Au);
    VMOVE(&R[4], Bu);
    VMOVE(&R[8], Cu);
    bn_mat_trn(invR, R);			/* inv of rot mat is trn */

    /* Compute S and invS matrices */
    /* invS is just 1/diagonal elements */
    MAT_IDN(S);
    S[ 0] = invAlen;
    S[ 5] = invBlen;
    S[10] = invClen;
    bn_mat_inv(invS, S);

    /* invRinvS, for converting points from unit sphere to model */
    bn_mat_mul(state.invRinvS, invR, invS);

    /* invRoS, for converting normals from unit sphere to model */
    bn_mat_mul(state.invRoS, invR, S);

    /* Compute radius of bounding sphere */
    radius = Alen;
    if (Blen > radius)
	radius = Blen;
    if (Clen > radius)
	radius = Clen;

    dtol = primitive_get_absolute_tolerance(ttol, radius);

    if (dtol > radius) {
	dtol = radius;
    }

    /* Convert distance tolerance into a maximum permissible angle
     * tolerance.  'radius' is largest radius.
     */
    state.theta_tol = 2 * acos(1.0 - dtol / radius);

    /* To ensure normal tolerance, remain below this angle */
    if (ttol->norm > 0.0 && ttol->norm < state.theta_tol) {
	state.theta_tol = ttol->norm;
    }

    *r = nmg_mrsv(m);	/* Make region, empty shell, vertex */
    state.s = BU_LIST_FIRST(shell, &(*r)->s_hd);

    /* Find the number of segments to divide 90 degrees worth into */
    nsegs = (int)(M_PI_2 / state.theta_tol + 0.999);
    if (nsegs < 2) nsegs = 2;

    /* Find total number of strips of vertices that will be needed.
     * nsegs for each hemisphere, plus the equator.  Note that faces
     * are listed in the stripe ABOVE, i.e., toward the poles.
     * Thus, strips[0] will have 4 faces.
     */
    nstrips = 2 * nsegs + 1;
    strips = (struct ell_vert_strip *)bu_calloc(nstrips,
						sizeof(struct ell_vert_strip), "strips[]");

    /* North pole */
    strips[0].nverts = 1;
    strips[0].nverts_per_strip = 0;
    strips[0].nfaces = 4;
    /* South pole */
    strips[nstrips-1].nverts = 1;
    strips[nstrips-1].nverts_per_strip = 0;
    strips[nstrips-1].nfaces = 4;
    /* equator */
    strips[nsegs].nverts = nsegs * 4;
    strips[nsegs].nverts_per_strip = nsegs;
    strips[nsegs].nfaces = 0;

    for (i = 1; i < nsegs; i++) {
	strips[i].nverts_per_strip =
	    strips[nstrips-1-i].nverts_per_strip = i;
	strips[i].nverts =
	    strips[nstrips-1-i].nverts = i * 4;
	strips[i].nfaces =
	    strips[nstrips-1-i].nfaces = (2 * i + 1)*4;
    }
    /* All strips have vertices and normals */
    for (i = 0; i < nstrips; i++) {
	strips[i].vp = (struct vertex **)bu_calloc(strips[i].nverts,
						   sizeof(struct vertex *), "strip vertex[]");
	strips[i].norms = (vect_t *)bu_calloc(strips[i].nverts,
					      sizeof(vect_t), "strip normals[]");
    }
    /* All strips have faces, except for the equator */
    for (i = 0; i < nstrips; i++) {
	if (strips[i].nfaces <= 0) continue;
	strips[i].fu = (struct faceuse **)bu_calloc(strips[i].nfaces,
						    sizeof(struct faceuse *), "strip faceuse[]");
    }

    /* First, build the triangular mesh topology */
    /* Do the top. "toff" in i-1 is UP, towards +B */
    for (i = 1; i <= nsegs; i++) {
	faceno = 0;
	tlim = strips[i-1].nverts;
	blim = strips[i].nverts;
	for (stripno = 0; stripno < 4; stripno++) {
	    toff = stripno * strips[i-1].nverts_per_strip;
	    boff = stripno * strips[i].nverts_per_strip;

	    /* Connect this quarter strip */
	    for (j = 0; j < strips[i].nverts_per_strip; j++) {

		/* "Right-side-up" triangle */
		vertp[0] = &(strips[i].vp[j+boff]);
		vertp[1] = &(strips[i-1].vp[(j+toff)%tlim]);
		vertp[2] = &(strips[i].vp[(j+1+boff)%blim]);
		if ((strips[i-1].fu[faceno++] = nmg_cmface(state.s, vertp, 3)) == 0) {
		    bu_log("rt_ell_tess() nmg_cmface failure\n");
		    goto fail;
		}
		if (j+1 >= strips[i].nverts_per_strip) break;

		/* Follow with interior "Up-side-down" triangle */
		vertp[0] = &(strips[i].vp[(j+1+boff)%blim]);
		vertp[1] = &(strips[i-1].vp[(j+toff)%tlim]);
		vertp[2] = &(strips[i-1].vp[(j+1+toff)%tlim]);
		if ((strips[i-1].fu[faceno++] = nmg_cmface(state.s, vertp, 3)) == 0) {
		    bu_log("rt_ell_tess() nmg_cmface failure\n");
		    goto fail;
		}
	    }
	}
    }
    /* Do the bottom.  Everything is upside down. "toff" in i+1 is DOWN */
    for (i = nsegs; i < nstrips-1; i++) {
	faceno = 0;
	tlim = strips[i+1].nverts;
	blim = strips[i].nverts;
	for (stripno = 0; stripno < 4; stripno++) {
	    toff = stripno * strips[i+1].nverts_per_strip;
	    boff = stripno * strips[i].nverts_per_strip;

	    /* Connect this quarter strip */
	    for (j = 0; j < strips[i].nverts_per_strip; j++) {

		/* "Right-side-up" triangle */
		vertp[0] = &(strips[i].vp[j+boff]);
		vertp[1] = &(strips[i].vp[(j+1+boff)%blim]);
		vertp[2] = &(strips[i+1].vp[(j+toff)%tlim]);
		if ((strips[i+1].fu[faceno++] = nmg_cmface(state.s, vertp, 3)) == 0) {
		    bu_log("rt_ell_tess() nmg_cmface failure\n");
		    goto fail;
		}
		if (j+1 >= strips[i].nverts_per_strip) break;

		/* Follow with interior "Up-side-down" triangle */
		vertp[0] = &(strips[i].vp[(j+1+boff)%blim]);
		vertp[1] = &(strips[i+1].vp[(j+1+toff)%tlim]);
		vertp[2] = &(strips[i+1].vp[(j+toff)%tlim]);
		if ((strips[i+1].fu[faceno++] = nmg_cmface(state.s, vertp, 3)) == 0) {
		    bu_log("rt_ell_tess() nmg_cmface failure\n");
		    goto fail;
		}
	    }
	}
    }

    /* Compute the geometry of each vertex.  Start with the location
     * in the unit sphere, and project back.  i=0 is "straight up"
     * along +B.
     */
    for (i = 0; i < nstrips; i++) {
	double alpha;		/* decline down from B to A */
	double beta;		/* angle around equator (azimuth) */
	fastf_t cos_alpha, sin_alpha;
	fastf_t cos_beta, sin_beta;
	point_t sphere_pt;
	point_t model_pt;

	alpha = (((double)i) / (nstrips-1));
	cos_alpha = cos(alpha*M_PI);
	sin_alpha = sin(alpha*M_PI);
	for (j = 0; j < strips[i].nverts; j++) {

	    beta = ((double)j) / strips[i].nverts;
	    cos_beta = cos(beta*M_2PI);
	    sin_beta = sin(beta*M_2PI);
	    VSET(sphere_pt,
		 cos_beta * sin_alpha,
		 cos_alpha,
		 sin_beta * sin_alpha);
	    /* Convert from ideal sphere coordinates */
	    MAT4X3PNT(model_pt, state.invRinvS, sphere_pt);
	    VADD2(model_pt, model_pt, state.eip->v);
	    /* Associate vertex geometry */
	    nmg_vertex_gv(strips[i].vp[j], model_pt);

	    /* Convert sphere normal to ellipsoid normal */
	    MAT4X3VEC(strips[i].norms[j], state.invRoS, sphere_pt);
	    /* May not be unit length anymore */
	    VUNITIZE(strips[i].norms[j]);
	}
    }

    /* Associate face geometry.  Equator has no faces */
    for (i = 0; i < nstrips; i++) {
	for (j = 0; j < strips[i].nfaces; j++) {
	    if (nmg_fu_planeeqn(strips[i].fu[j], tol) < 0)
		goto fail;
	}
    }

    /* Associate normals with vertexuses */
    for (i = 0; i < nstrips; i++) {
	for (j = 0; j < strips[i].nverts; j++) {
	    struct faceuse *fu;
	    struct vertexuse *vu;
	    vect_t norm_opp;

	    NMG_CK_VERTEX(strips[i].vp[j]);
	    VREVERSE(norm_opp, strips[i].norms[j]);

	    for (BU_LIST_FOR(vu, vertexuse, &strips[i].vp[j]->vu_hd)) {
		fu = nmg_find_fu_of_vu(vu);
		NMG_CK_FACEUSE(fu);
		/* get correct direction of normals depending on
		 * faceuse orientation
		 */
		if (fu->orientation == OT_SAME)
		    nmg_vertexuse_nv(vu, strips[i].norms[j]);
		else if (fu->orientation == OT_OPPOSITE)
		    nmg_vertexuse_nv(vu, norm_opp);
	    }
	}
    }

    /* Compute "geometry" for region and shell */
    nmg_region_a(*r, tol);

    /* Release memory */
    /* All strips have vertices and normals */
    for (i = 0; i < nstrips; i++) {
	bu_free((char *)strips[i].vp, "strip vertex[]");
	bu_free((char *)strips[i].norms, "strip norms[]");
    }
    /* All strips have faces, except for equator */
    for (i = 0; i < nstrips; i++) {
	if (strips[i].fu == (struct faceuse **)0) continue;
	bu_free((char *)strips[i].fu, "strip faceuse[]");
    }
    bu_free((char *)strips, "strips[]");
    return 0;
fail:
    /* Release memory */
    /* All strips have vertices and normals */
    for (i = 0; i < nstrips; i++) {
	bu_free((char *)strips[i].vp, "strip vertex[]");
	bu_free((char *)strips[i].norms, "strip norms[]");
    }
    /* All strips have faces, except for equator */
    for (i = 0; i < nstrips; i++) {
	if (strips[i].fu == (struct faceuse **)0) continue;
	bu_free((char *)strips[i].fu, "strip faceuse[]");
    }
    bu_free((char *)strips, "strips[]");
    return -1;
}


/**
 * Import an ellipsoid/sphere from the database format to the internal
 * structure.  Apply modeling transformations as well.
 */
int
rt_ell_import4(struct rt_db_internal *ip, const struct bu_external *ep, register const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_ell_internal *eip;
    union record *rp;
    fastf_t vec[3*4];

    if (dbip) RT_CK_DBI(dbip);

    BU_CK_EXTERNAL(ep);
    rp = (union record *)ep->ext_buf;
    /* Check record type */
    if (rp->u_id != ID_SOLID) {
	bu_log("rt_ell_import4: defective record\n");
	return -1;
    }

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_ELL;
    ip->idb_meth = &OBJ[ID_ELL];
    BU_ALLOC(ip->idb_ptr, struct rt_ell_internal);

    eip = (struct rt_ell_internal *)ip->idb_ptr;
    eip->magic = RT_ELL_INTERNAL_MAGIC;

    /* Convert from database to internal format */
    flip_fastf_float(vec, rp->s.s_values, 4, (dbip && dbip->dbi_version < 0) ? 1 : 0);

    /* Apply modeling transformations */
    if (mat == NULL) mat = bn_mat_identity;
    MAT4X3PNT(eip->v, mat, &vec[0*3]);
    MAT4X3VEC(eip->a, mat, &vec[1*3]);
    MAT4X3VEC(eip->b, mat, &vec[2*3]);
    MAT4X3VEC(eip->c, mat, &vec[3*3]);

    return 0;		/* OK */
}


int
rt_ell_export4(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_ell_internal *tip;
    union record *rec;

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_ELL && ip->idb_type != ID_SPH) return -1;
    tip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(tip);

    BU_CK_EXTERNAL(ep);
    ep->ext_nbytes = sizeof(union record);
    ep->ext_buf = (uint8_t *)bu_calloc(1, ep->ext_nbytes, "ell external");
    rec = (union record *)ep->ext_buf;

    rec->s.s_id = ID_SOLID;
    rec->s.s_type = GENELL;

    /* NOTE: This also converts to dbfloat_t */
    VSCALE(&rec->s.s_values[0], tip->v, local2mm);
    VSCALE(&rec->s.s_values[3], tip->a, local2mm);
    VSCALE(&rec->s.s_values[6], tip->b, local2mm);
    VSCALE(&rec->s.s_values[9], tip->c, local2mm);

    return 0;
}

int
rt_ell_mat(struct rt_db_internal *rop, const mat_t mat, const struct rt_db_internal *ip)
{
    if (!rop || !ip || !mat)
	return BRLCAD_OK;

    struct rt_ell_internal *tip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(tip);
    struct rt_ell_internal *top = (struct rt_ell_internal *)rop->idb_ptr;
    RT_ELL_CK_MAGIC(top);

    vect_t v, a, b, c;
    VMOVE(v, tip->v);
    VMOVE(a, tip->a);
    VMOVE(b, tip->b);
    VMOVE(c, tip->c);

    MAT4X3PNT(top->v, mat, v);
    MAT4X3VEC(top->a, mat, a);
    MAT4X3VEC(top->b, mat, b);
    MAT4X3VEC(top->c, mat, c);

    return BRLCAD_OK;
}

/**
 * Import an ellipsoid/sphere from the database format to the internal
 * structure.  Apply modeling transformations as well.
 */
int
rt_ell_import5(struct rt_db_internal *ip, const struct bu_external *ep, register const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_ell_internal *eip;

    /* must be double for import and export */
    double vec[ELEMENTS_PER_VECT*4];

    if (dbip) RT_CK_DBI(dbip);
    RT_CK_DB_INTERNAL(ip);
    BU_CK_EXTERNAL(ep);

    BU_ASSERT(ep->ext_nbytes == SIZEOF_NETWORK_DOUBLE * ELEMENTS_PER_VECT*4);

    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_ELL;
    ip->idb_meth = &OBJ[ID_ELL];
    BU_ALLOC(ip->idb_ptr, struct rt_ell_internal);

    eip = (struct rt_ell_internal *)ip->idb_ptr;
    eip->magic = RT_ELL_INTERNAL_MAGIC;

    /* Convert from database (network) to internal (host) format */
    bu_cv_ntohd((unsigned char *)vec, ep->ext_buf, ELEMENTS_PER_VECT*4);

    VMOVE(eip->v, &vec[0*ELEMENTS_PER_VECT]);
    VMOVE(eip->a, &vec[1*ELEMENTS_PER_VECT]);
    VMOVE(eip->b, &vec[2*ELEMENTS_PER_VECT]);
    VMOVE(eip->c, &vec[3*ELEMENTS_PER_VECT]);

    /* Apply modeling transformations */
    if (mat == NULL) mat = bn_mat_identity;
    return rt_ell_mat(ip, mat, ip);
}


/**
 * The external format is:
 * V point
 * A vector
 * B vector
 * C vector
 */
int
rt_ell_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_ell_internal *eip;

    /* must be double for import and export */
    double vec[ELEMENTS_PER_VECT*4];

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_ELL && ip->idb_type != ID_SPH) return -1;
    eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    BU_CK_EXTERNAL(ep);
    ep->ext_nbytes = SIZEOF_NETWORK_DOUBLE * ELEMENTS_PER_VECT*4;
    ep->ext_buf = (uint8_t *)bu_malloc(ep->ext_nbytes, "ell external");

    /* scale 'em into local buffer */
    VSCALE(&vec[0*ELEMENTS_PER_VECT], eip->v, local2mm);
    VSCALE(&vec[1*ELEMENTS_PER_VECT], eip->a, local2mm);
    VSCALE(&vec[2*ELEMENTS_PER_VECT], eip->b, local2mm);
    VSCALE(&vec[3*ELEMENTS_PER_VECT], eip->c, local2mm);

    /* Convert from internal (host) to database (network) format */
    bu_cv_htond(ep->ext_buf, (unsigned char *)vec, ELEMENTS_PER_VECT*4);

    return 0;
}


/**
 * Make human-readable formatted presentation of this solid.  First
 * line describes type of solid.  Additional lines are indented one
 * tab, and give parameter values.
 */
int
rt_ell_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local)
{
    register struct rt_ell_internal *tip =
	(struct rt_ell_internal *)ip->idb_ptr;
    fastf_t mag_a, mag_b, mag_c;
    char buf[256];
    double angles[5];
    vect_t unitv;

    RT_ELL_CK_MAGIC(tip);
    bu_vls_strcat(str, "ellipsoid (ELL)\n");

    sprintf(buf, "\tV (%g, %g, %g)\n",
	    INTCLAMP(tip->v[X] * mm2local),
	    INTCLAMP(tip->v[Y] * mm2local),
	    INTCLAMP(tip->v[Z] * mm2local));
    bu_vls_strcat(str, buf);

    mag_a = MAGNITUDE(tip->a);
    mag_b = MAGNITUDE(tip->b);
    mag_c = MAGNITUDE(tip->c);

    sprintf(buf, "\tA (%g, %g, %g) mag=%g\n",
	    INTCLAMP(tip->a[X] * mm2local),
	    INTCLAMP(tip->a[Y] * mm2local),
	    INTCLAMP(tip->a[Z] * mm2local),
	    INTCLAMP(mag_a * mm2local));
    bu_vls_strcat(str, buf);

    sprintf(buf, "\tB (%g, %g, %g) mag=%g\n",
	    INTCLAMP(tip->b[X] * mm2local),
	    INTCLAMP(tip->b[Y] * mm2local),
	    INTCLAMP(tip->b[Z] * mm2local),
	    INTCLAMP(mag_b * mm2local));
    bu_vls_strcat(str, buf);

    sprintf(buf, "\tC (%g, %g, %g) mag=%g\n",
	    INTCLAMP(tip->c[X] * mm2local),
	    INTCLAMP(tip->c[Y] * mm2local),
	    INTCLAMP(tip->c[Z] * mm2local),
	    INTCLAMP(mag_c * mm2local));
    bu_vls_strcat(str, buf);

    if (!verbose) return 0;

    VSCALE(unitv, tip->a, 1/mag_a);
    rt_find_fallback_angle(angles, unitv);
    rt_pr_fallback_angle(str, "\tA", angles);

    VSCALE(unitv, tip->b, 1/mag_b);
    rt_find_fallback_angle(angles, unitv);
    rt_pr_fallback_angle(str, "\tB", angles);

    VSCALE(unitv, tip->c, 1/mag_c);
    rt_find_fallback_angle(angles, unitv);
    rt_pr_fallback_angle(str, "\tC", angles);

    return 0;
}


/**
 * Free the storage associated with the rt_db_internal version of this
 * solid.
 */
void
rt_ell_ifree(struct rt_db_internal *ip)
{
    RT_CK_DB_INTERNAL(ip);

    bu_free(ip->idb_ptr, "ell ifree");
    ip->idb_ptr = ((void *)0);
}


/* The U parameter runs south to north.  In order to orient loop CCW,
 * need to start with 0, 1-->0, 0 transition at the south pole.
 */
static const fastf_t rt_ell_uvw[5*ELEMENTS_PER_VECT] = {
    0, 1, 0,
    0, 0, 0,
    1, 0, 0,
    1, 1, 0,
    0, 1, 0
};


int
rt_ell_tnurb(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bn_tol *tol)
{
    mat_t R;
    mat_t S;
    mat_t invR;
    mat_t invS;
    mat_t invRinvS;
    mat_t invRoS;
    mat_t unit2model;
    mat_t xlate;
    vect_t Au, Bu, Cu;	/* A, B, C with unit length */
    fastf_t Alen, Blen, Clen;
    fastf_t invAlen, invBlen, invClen;
    fastf_t magsq_a, magsq_b, magsq_c;
    fastf_t f;
    register int i;
    struct rt_ell_internal *eip;
    struct vertex *verts[8];
    struct vertex **vertp[4];
    struct faceuse *fu;
    struct shell *s;
    struct loopuse *lu;
    struct edgeuse *eu;
    point_t pole;

    RT_CK_DB_INTERNAL(ip);
    eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    /* Validate that |A| > 0, |B| > 0, |C| > 0 */
    magsq_a = MAGSQ(eip->a);
    magsq_b = MAGSQ(eip->b);
    magsq_c = MAGSQ(eip->c);
    if (magsq_a < tol->dist_sq || magsq_b < tol->dist_sq || magsq_c < tol->dist_sq) {
	bu_log("rt_ell_tnurb():  zero length A, B, or C vector\n");
	return -2;		/* BAD */
    }

    /* Create unit length versions of A, B, C */
    Alen = sqrt(magsq_a);
    Blen = sqrt(magsq_b);
    Clen = sqrt(magsq_c);
    invAlen = 1.0/Alen;
    invBlen = 1.0/Blen;
    invClen = 1.0/Clen;
    VSCALE(Au, eip->a, invAlen);
    VSCALE(Bu, eip->b, invBlen);
    VSCALE(Cu, eip->c, invClen);

    /* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
    f = VDOT(Au, Bu);
    if (! NEAR_ZERO(f, tol->dist)) {
	bu_log("rt_ell_tnurb():  A not perpendicular to B, f=%f\n", f);
	return -3;		/* BAD */
    }
    f = VDOT(Bu, Cu);
    if (! NEAR_ZERO(f, tol->dist)) {
	bu_log("rt_ell_tnurb():  B not perpendicular to C, f=%f\n", f);
	return -3;		/* BAD */
    }
    f = VDOT(Au, Cu);
    if (! NEAR_ZERO(f, tol->dist)) {
	bu_log("rt_ell_tnurb():  A not perpendicular to C, f=%f\n", f);
	return -3;		/* BAD */
    }

    {
	vect_t axb;
	VCROSS(axb, Au, Bu);
	f = VDOT(axb, Cu);
	if (f < 0) {
	    VREVERSE(Cu, Cu);
	    VREVERSE(eip->c, eip->c);
	}
    }

    /* Compute R and Rinv matrices */
    MAT_IDN(R);
    VMOVE(&R[0], Au);
    VMOVE(&R[4], Bu);
    VMOVE(&R[8], Cu);
    bn_mat_trn(invR, R);			/* inv of rot mat is trn */

    /* Compute S and invS matrices */
    /* invS is just 1/diagonal elements */
    MAT_IDN(S);
    S[ 0] = invAlen;
    S[ 5] = invBlen;
    S[10] = invClen;
    bn_mat_inv(invS, S);

    /* invRinvS, for converting points from unit sphere to model */
    bn_mat_mul(invRinvS, invR, invS);

    /* invRoS, for converting normals from unit sphere to model */
    bn_mat_mul(invRoS, invR, S);

    MAT_IDN(xlate);
    MAT_DELTAS_VEC(xlate, eip->v);
    bn_mat_mul(unit2model, xlate, invRinvS);

    /*
     * --- Build Topology ---
     *
     * There is a vertex at either pole, and a single longitude line.
     * There is a single face, an snurb with singularities.
     *
     * vert[0] is the south pole, and is the first row of the ctl_points.
     * vert[1] is the north pole, and is the last row of the ctl_points.
     *
     * Somewhat surprisingly, the U parameter runs from south to north.
     */
    for (i = 0; i < 8; i++) verts[i] = (struct vertex *)0;

    *r = nmg_mrsv(m);	/* Make region, empty shell, vertex */
    s = BU_LIST_FIRST(shell, &(*r)->s_hd);

    vertp[0] = &verts[0];
    vertp[1] = &verts[0];
    vertp[2] = &verts[1];
    vertp[3] = &verts[1];

    if ((fu = nmg_cmface(s, vertp, 4)) == 0) {
	bu_log("rt_ell_tnurb(): nmg_cmface() fail on face\n");
	return -1;
    }

    /* March around the fu's loop assigning uv parameter values */
    lu = BU_LIST_FIRST(loopuse, &fu->lu_hd);
    NMG_CK_LOOPUSE(lu);
    eu = BU_LIST_FIRST(edgeuse, &lu->down_hd);
    NMG_CK_EDGEUSE(eu);

    /* Loop always has Counter-Clockwise orientation (CCW) */
    for (i = 0; i < 4; i++) {
	nmg_vertexuse_a_cnurb(eu->vu_p, &rt_ell_uvw[i*ELEMENTS_PER_VECT]);
	nmg_vertexuse_a_cnurb(eu->eumate_p->vu_p, &rt_ell_uvw[(i+1)*ELEMENTS_PER_VECT]);
	eu = BU_LIST_NEXT(edgeuse, &eu->l);
    }

    /* Associate vertex geometry */
    VSUB2(pole, eip->v, eip->c);		/* south pole */
    nmg_vertex_gv(verts[0], pole);
    VADD2(pole, eip->v, eip->c);
    nmg_vertex_gv(verts[1], pole);	/* north pole */

    /* Build snurb, transformed into final position */
    nmg_sphere_face_snurb(fu, unit2model);

    /* Associate edge geometry (trimming curve) -- linear in param space */
    eu = BU_LIST_FIRST(edgeuse, &lu->down_hd);
    NMG_CK_EDGEUSE(eu);
    for (i = 0; i < 4; i++) {
	nmg_edge_g_cnurb_plinear(eu);
	eu = BU_LIST_NEXT(edgeuse, &eu->l);
    }

    /* Compute "geometry" for region and shell */
    nmg_region_a(*r, tol);

    return 0;
}


/* u, v=(0, 0) is supposed to be the south pole, at Z=-1.0
 *
 * The V direction runs from the south to the north pole.
 */
static void
nmg_sphere_face_snurb(struct faceuse *fu, const matp_t m)
{
    struct face_g_snurb *fg;
    fastf_t root2_2;
    register fastf_t *op;

    NMG_CK_FACEUSE(fu);
    root2_2 = sqrt(2.0)*0.5;

    /* Let the library allocate all the storage */
    /* The V direction runs from south to north pole */
    nmg_face_g_snurb(fu,
		     3, 3,		/* u, v order */
		     8, 12,		/* Number of knots, u, v */
		     NULL, NULL,	/* initial u, v knot vectors */
		     9, 5,		/* n_rows, n_cols */
		     RT_NURB_MAKE_PT_TYPE(4, RT_NURB_PT_XYZ, RT_NURB_PT_RATIONAL),
		     NULL);		/* initial mesh */

    fg = fu->f_p->g.snurb_p;
    NMG_CK_FACE_G_SNURB(fg);

    fg->v.knots[ 0] = 0;
    fg->v.knots[ 1] = 0;
    fg->v.knots[ 2] = 0;
    fg->v.knots[ 3] = 0.25;
    fg->v.knots[ 4] = 0.25;
    fg->v.knots[ 5] = 0.5;
    fg->v.knots[ 6] = 0.5;
    fg->v.knots[ 7] = 0.75;
    fg->v.knots[ 8] = 0.75;
    fg->v.knots[ 9] = 1;
    fg->v.knots[10] = 1;
    fg->v.knots[11] = 1;

    fg->u.knots[0] = 0;
    fg->u.knots[1] = 0;
    fg->u.knots[2] = 0;
    fg->u.knots[3] = 0.5;
    fg->u.knots[4] = 0.5;
    fg->u.knots[5] = 1;
    fg->u.knots[6] = 1;
    fg->u.knots[7] = 1;

    op = fg->ctl_points;

/* Inspired by MAT4X4PNT */
#define M(x, y, z, w) { \
	*op++ = m[ 0]*(x) + m[ 1]*(y) + m[ 2]*(z) + m[ 3]*(w);\
	*op++ = m[ 4]*(x) + m[ 5]*(y) + m[ 6]*(z) + m[ 7]*(w);\
	*op++ = m[ 8]*(x) + m[ 9]*(y) + m[10]*(z) + m[11]*(w);\
	*op++ = m[12]*(x) + m[13]*(y) + m[14]*(z) + m[15]*(w); }


    M(0     ,   0     , -1.0     , 1.0);
    M(root2_2 ,   0     , -root2_2 , root2_2);
    M(1.0     ,   0     ,   0     , 1.0);
    M(root2_2 ,   0     , root2_2 , root2_2);
    M(0     ,   0     , 1.0     , 1.0);

    M(0     ,   0     , -root2_2 , root2_2);
    M(0.5     , -0.5     , -0.5     , 0.5);
    M(root2_2 , -root2_2 ,   0     , root2_2);
    M(0.5     , -0.5     , 0.5     , 0.5);
    M(0     ,   0     , root2_2 , root2_2);

    M(0     ,   0     , -1.0     , 1.0);
    M(0     , -root2_2 , -root2_2 , root2_2);
    M(0     , -1.0     ,   0     , 1.0);
    M(0     , -root2_2 , root2_2 , root2_2);
    M(0     ,   0     , 1.0     , 1.0);

    M(0     ,   0     , -root2_2 , root2_2);
    M(-0.5     , -0.5     , -0.5     , 0.5);
    M(-root2_2 , -root2_2 ,   0     , root2_2);
    M(-0.5     , -0.5     , 0.5     , 0.5);
    M(0     ,   0     , root2_2 , root2_2);

    M(0     ,   0     , -1.0     , 1.0);
    M(-root2_2 ,   0     , -root2_2 , root2_2);
    M(-1.0     ,   0     ,   0     , 1.0);
    M(-root2_2 ,   0     , root2_2 , root2_2);
    M(0     ,   0     , 1.0     , 1.0);

    M(0     ,   0     , -root2_2 , root2_2);
    M(-0.5     , 0.5     , -0.5     , 0.5);
    M(-root2_2 , root2_2 ,   0     , root2_2);
    M(-0.5     , 0.5     , 0.5     , 0.5);
    M(0     ,   0     , root2_2 , root2_2);

    M(0     ,   0     , -1.0     , 1.0);
    M(0     , root2_2 , -root2_2 , root2_2);
    M(0     , 1.0     ,   0     , 1.0);
    M(0     , root2_2 , root2_2 , root2_2);
    M(0     ,   0     , 1.0     , 1.0);

    M(0     ,   0     , -root2_2 , root2_2);
    M(0.5     , 0.5     , -0.5     , 0.5);
    M(root2_2 , root2_2 ,   0     , root2_2);
    M(0.5     , 0.5     , 0.5     , 0.5);
    M(0     ,   0     , root2_2 , root2_2);

    M(0     ,   0     , -1.0     , 1.0);
    M(root2_2 ,   0     , -root2_2 , root2_2);
    M(1.0     ,   0     ,   0     , 1.0);
    M(root2_2 ,   0     , root2_2 , root2_2);
    M(0     ,   0     , 1.0     , 1.0);
}


/**
 * @brief Method for declaration of parameters so that it can be used
 * by Parametrics and Constraints library.
 *
 * allocates space for PC set , User is expect to free after use
 *
 * @return 0 on success
 * @return -1 on failure
 */
int
rt_ell_params(struct pc_pc_set *UNUSED(pcs), const struct rt_db_internal *UNUSED(ip))
{
    return -1;			/* FAIL */
}


/*
 * Used by EHY, EPA, HYP.  See librt_private.h for details.
 */
fastf_t
ell_angle(fastf_t *p1, fastf_t a, fastf_t b, fastf_t dtol, fastf_t ntol)
{
    fastf_t dist, intr, m, theta0, theta1;
    point_t mpt, p0;
    vect_t norm_line, norm_ell;

    VSET(p0, a, 0., 0.);
    /* slope and intercept of segment */
    m = (p1[Y] - p0[Y]) / (p1[X] - p0[X]);
    intr = p0[Y] - m * p0[X];
    /* point on ellipse with max dist between ellipse and line */
    mpt[X] = a / sqrt(b*b / (m*m*a*a) + 1);
    mpt[Y] = b * sqrt(1 - mpt[X] * mpt[X] / (a*a));
    mpt[Z] = 0;
    /* max distance between that point and line */
    dist = fabs(m * mpt[X] - mpt[Y] + intr) / sqrt(m * m + 1);
    /* angles between normal of line and of ellipse at line endpoints */
    VSET(norm_line, m, -1., 0.);
    VSET(norm_ell, b * b * p0[X], a * a * p0[Y], 0.);
    VUNITIZE(norm_line);
    VUNITIZE(norm_ell);
    theta0 = fabs(acos(VDOT(norm_line, norm_ell)));
    VSET(norm_ell, b * b * p1[X], a * a * p1[Y], 0.);
    VUNITIZE(norm_ell);
    theta1 = fabs(acos(VDOT(norm_line, norm_ell)));
    /* split segment at widest point if not within error tolerances */
    if (dist > dtol || theta0 > ntol || theta1 > ntol) {
	/* split segment */
	return ell_angle(mpt, a, b, dtol, ntol);
    } else
	return (acos(VDOT(p0, p1)
		    / (MAGNITUDE(p0) * MAGNITUDE(p1))));
}


/**
 * Computes volume of a ellipsoid.
 */
void
rt_ell_volume(fastf_t *volume, const struct rt_db_internal *ip)
{
    fastf_t mag_a, mag_b, mag_c;
    struct rt_ell_internal *eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    mag_a = MAGNITUDE(eip->a);
    mag_b = MAGNITUDE(eip->b);
    mag_c = MAGNITUDE(eip->c);
    *volume = 4.0/3.0 * M_PI * mag_a * mag_b * mag_c;
}


/**
 * Computes centroid of an ellipsoid
 */
void
rt_ell_centroid(point_t *cent, const struct rt_db_internal *ip)
{
    struct rt_ell_internal *eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);
    VMOVE(*cent,eip->v);
}


/**
 * Computes the surface area of an ellipsoid.
 * logs error if triaxial ellipsoid (cannot find surface area).
 */
#define PROLATE 1
#define OBLATE 2
void
rt_ell_surf_area(fastf_t *area, const struct rt_db_internal *ip)
{
    fastf_t mag_a, mag_b, mag_c;
#ifdef major        /* Some systems have these defined as macros!!! */
#undef major
#endif
#ifdef minor
#undef minor
#endif
    fastf_t major = 0, minor = 0;
    fastf_t major2, minor2;
    fastf_t ecc;
    int ell_type = 0;

    struct rt_ell_internal *eip = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(eip);

    mag_a = MAGNITUDE(eip->a);
    mag_b = MAGNITUDE(eip->b);
    mag_c = MAGNITUDE(eip->c);

    if (EQUAL(mag_a, mag_b) && EQUAL(mag_b, mag_c)) {
	/* case: sphere */
	*area = 4.0 * M_PI * mag_a * mag_a;
	return;
    }

    if (EQUAL(mag_a, mag_b)) {
	if (mag_a > mag_c) {
	    /* case: prolate spheroid */
	    ell_type = PROLATE;
	    major = mag_a;
	    minor = mag_c;
	} else {
	    /* case: oblate spheroid */
	    ell_type = OBLATE;
	    major = mag_c;
	    minor = mag_a;
	}
    } else if (EQUAL(mag_a, mag_c)) {
	if (mag_a > mag_b) {
	    /* case: prolate spheroid */
	    ell_type = PROLATE;
	    major = mag_a;
	    minor = mag_b;
	} else {
	    /* case: oblate spheroid */
	    ell_type = OBLATE;
	    major = mag_b;
	    minor = mag_a;
	}
    } else if (EQUAL(mag_b, mag_c)) {
	if (mag_a > mag_c) {
	    /* case: prolate spheroid */
	    ell_type = PROLATE;
	    major = mag_a;
	    minor = mag_c;
	} else {
	    /* case: oblate spheroid */
	    ell_type = OBLATE;
	    major = mag_c;
	    minor = mag_a;
	}
    }

    major2 = major * major;
    minor2 = minor * minor;
    ecc = sqrt(1.0 - (minor2 / major2));

    switch (ell_type) {
    case PROLATE:
	*area = (M_2PI * minor2) + (M_2PI * major * minor / ecc) * asin(ecc);
	break;
    case OBLATE:
	*area = (M_2PI * major2) + (M_PI * minor2 / ecc) * log((1.0 + ecc) / (1.0 - ecc));
	break;
    default:
	bu_log("rt_ell_surf_area(): triaxial ellipsoid, cannot find surface area");
    }
}

int
rt_ell_labels(struct rt_point_labels *pl, int pl_max, const mat_t xform, const struct rt_db_internal *ip, const struct bn_tol *UNUSED(tol))
{
    int lcnt = 4;
    if (!pl || pl_max < lcnt || !ip)
	return 0;

    struct rt_ell_internal *ell = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(ell);

    point_t work, pos_view;
    int npl = 0;

#define POINT_LABEL(_pt, _char) { \
    VMOVE(pl[npl].pt, _pt); \
    pl[npl].str[0] = _char; \
    pl[npl++].str[1] = '\0'; }

    MAT4X3PNT(pos_view, xform, ell->v);
    POINT_LABEL(pos_view, 'V');

    VADD2(work, ell->v, ell->a);
    MAT4X3PNT(pos_view, xform, work);
    POINT_LABEL(pos_view, 'A');

    VADD2(work, ell->v, ell->b);
    MAT4X3PNT(pos_view, xform, work);
    POINT_LABEL(pos_view, 'B');

    VADD2(work, ell->v, ell->c);
    MAT4X3PNT(pos_view, xform, work);
    POINT_LABEL(pos_view, 'C');

    return lcnt;
}

const char *
rt_ell_keypoint(point_t *pt, const char *keystr, const mat_t mat, const struct rt_db_internal *ip, const struct bn_tol *UNUSED(tol))
{
    if (!pt || !ip)
	return NULL;

    point_t mpt = VINIT_ZERO;
    struct rt_ell_internal *ell = (struct rt_ell_internal *)ip->idb_ptr;
    RT_ELL_CK_MAGIC(ell);

    static const char *default_keystr = "V";
    const char *k = (keystr) ? keystr : default_keystr;

    if (BU_STR_EQUAL(k, default_keystr)) {
	VMOVE(mpt, ell->v);
	goto ell_kpt_end;
    }
    if (BU_STR_EQUAL(k, "A")) {
	VADD2(mpt, ell->v, ell->a);
	goto ell_kpt_end;
    }
    if (BU_STR_EQUAL(k, "B")) {
	VADD2(mpt, ell->v, ell->b);
	goto ell_kpt_end;
    }
    if (BU_STR_EQUAL(k, "C")) {
	VADD2(mpt, ell->v, ell->c);
	goto ell_kpt_end;
    }

    // No keystr matches - failed
    return NULL;

ell_kpt_end:

    MAT4X3PNT(*pt, mat, mpt);

    return k;
}


/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
