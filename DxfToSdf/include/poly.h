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
 *  After a .DXF file has been loaded and its various groups parsed into
 *  data structures, this module sorts its polygons, lines, and points.
 *  The structures are presorted for each viewing angle.
 *************************************************************************/



#ifndef  _POLY_H
#define  _POLY_H


#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "sdf.h"



/*
**  Macros and magic numbers
*/

#define  INVALID_INDEX              -1



/*
**  Types and structures
*/

typedef enum polyFlagsTag
   {
      POLYF_ALWAYS = 0x80
   }
   PolyFlags;

typedef struct polygonTag
   {
      UWORD color;
      UWORD numPoints;
      PolyFlags flags;

      Point center, normal;
      WORD index[ MAX_POLY_POINTS ];
   }
   Polygon;



/*
**  Function prototypes
*/

extern double normalize( Point *point );
extern void compute_center( Polygon *poly, const Point *points );
extern void compute_normal( Polygon *poly, const Point *points );
extern BOOL compare_points( const Point *pt1, const Point *pt2, double thresh );
extern const Point *cross_product( const Point *a, const Point *b );
extern void weld_polys( void );


#endif   /*  _POLY_H  */
