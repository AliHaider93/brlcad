/*                     T E X T U R E _ C A M O . H
 *
 * @file texture_camo.h
 *
 * BRL-CAD
 *
 * Copyright (C) 2002-2005 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 *
 *  Comments -
 *      Texture Library - Camoflauge Header
 *
 *  Author -
 *      Justin L. Shumaker
 *
 *  Source -
 *      The U. S. Army Research Laboratory
 *      Aberdeen Proving Ground, Maryland  21005-5068  USA
 *
 * $Id$
 */

#ifndef _TEXTURE_CAMO_H
#define _TEXTURE_CAMO_H


#include "texture_internal.h"
#include "texture_perlin.h"


typedef struct texture_camo_s {
  tfloat size;
  int octaves;
  int absolute;
  TIE_3 color1;
  TIE_3 color2;
  TIE_3 color3;
  texture_perlin_t perlin;
} texture_camo_t;


extern	void	texture_camo_init(texture_t *texture, tfloat size, int octaves, int absolute, TIE_3 color1, TIE_3 color2, TIE_3 color3);
extern	void	texture_camo_free(texture_t *texture);
extern	void	texture_camo_work(texture_t *texture, common_mesh_t *mesh, tie_ray_t *ray, tie_id_t *id, TIE_3 *pixel);


#endif