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
 *  This module has routines which deal with polygon edges.  They'll be
 *  called when we try to combine polygons, split polygons, etc.
 *************************************************************************/



#ifndef  _EDGE_H
#define  _EDGE_H


#include <exec/types.h>

#include "dxftosdf.h"
#include "poly.h"
#include "tables.h"



/*
**  Macros and magic numbers
*/

#define  CLOCKWISE                  0
#define  COUNTERCLOCKWISE           NOT CLOCKWISE
#define  MAX_PATHS                  32



/*
**  Types and structures
*/

typedef struct edgeTag
   {
      struct edgeTag *next;
      WORD poly;
      WORD start, end;
   }
   Edge;



/*
**  Function prototypes
*/

extern void combine_edges( Polygon *dest, const Layer *layer, const WORD *index, UWORD num );


#endif   /*  _EDGE_H  */
