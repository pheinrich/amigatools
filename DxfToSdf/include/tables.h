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
 *  The tables section of a .DXF file is parsed here.  Five table types
 *  are recognized, though not all are significant to us.
 *************************************************************************/



#ifndef  _TABLES_H
#define  _TABLES_H


#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "poly.h"



/*
**  Types and structures
*/

typedef struct layerTag
   {
      struct layerTag *next;
      char name[ MAX_VALUELEN ];

      Polygon *polygons;
      Point *points;
      UWORD numPolys, numPoints;
   }
   Layer;



/*
**  Function prototypes
*/

extern Layer *find_layer( void );
extern Layer *get_layer( void );
extern void add_polygon( Layer *layer, const Polygon *poly );
extern WORD find_point( Layer *layer, const Point *point );
extern void load_tables( void );
extern void unload_tables( void );


#endif   /*  _TABLES_H  */
