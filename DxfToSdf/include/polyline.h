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
 *  Polyline entities are handled here.  We'll only recognize three types:
 *  closed, 3D, and polyface mesh.
 *************************************************************************/



#ifndef  _POLYLINE_H
#define  _POLYLINE_H


#include <exec/types.h>

#include <dxftosdf.h>
#include <tables.h>



/*
**  Types and structures
*/

typedef enum polylineTypeTag
   {
      UNKNOWN_POLYLINE = 0x00,
      CLOSED_POLYLINE  = 0x01,
      THREED_POLYLINE  = 0x08,
      POLYFACE_MESH    = 0x40
   }
   PolylineType;



/*
**  Function prototypes
*/

extern void load_generic_polyline_entity( Layer *layer );
extern void load_polyface_mesh_entity( Layer *layer, UWORD m, UWORD n );


#endif   /*  _POLYLINE_H  */
