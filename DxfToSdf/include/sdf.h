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
 *  After the geometric primitives contained in a .DXF file have been
 *  loaded and sorted for each viewing angle, this module saves them to a
 *  .SDF file.
 *************************************************************************/



#ifndef  _SDF_H
#define  _SDF_H


#include <exec/types.h>

#include "dxftosdf.h"



/*
**  Macros and magic numbers
*/

#define  CENTER_LIST                ((setup.super ? 6 : 5)*sizeof( LONG ) +\
                                    4*sizeof( WORD ))



/*
**  Types and structures
*/

typedef enum sdfFlagsTag
   {
      SDFF_PRESORT = 0x4000,
      SDFF_SUPER   = 0x8000
   }
   SdfFlags;

typedef struct rPointTag
   {
      double angle, dist;
      double x, y, z;
   }
   RPoint;

typedef struct rNormalTag
   {
      UBYTE azimuth;
      UBYTE elevation;
   }
   RNormal;

typedef struct rPolyTag
   {
      UWORD color;
      UWORD numPoints;
      UWORD flags;

      RPoint center;
      RNormal normal;
      WORD index[ MAX_POLY_POINTS ];

      ULONG offset;
   }
   RPolygon;



/*
**  Function prototypes
*/

extern void save_sdf( void );


#endif   /*  _SDF_H  */
