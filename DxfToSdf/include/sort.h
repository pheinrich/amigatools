 /*************************************************************************
 *
 *  DxfToSdf
 *  Copyright (c) 1993,2009  Peter Heinrich
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301, USA.
 *
 *************************************************************************
 *  This module of the program attempts to depth-arrange polygons in order
 *  of distance from the camera.
 *************************************************************************/



#ifndef  _SORT_H
#define  _SORT_H


#include <exec/types.h>

#include "dxftosdf.h"
#include "sdf.h"



/*
**  Types and structures
*/

typedef struct vAngleTag
   {
      struct vAngleTag *next;
      UWORD angle;

      WORD *polygons;
      UWORD numPolys;
   }
   VAngle;



/*
**  Function prototypes
*/

extern void build_shape( void );
extern void add_angle( UWORD angle );
extern void free_angles( void );
extern VAngle *sort_polys( const RPolygon *polys, UWORD numPolys,
                                    const RPoint *points, UWORD numPoints );


#endif   /*  _SORT_H  */
