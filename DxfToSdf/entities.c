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
 *  Individual entities stored in the .DXF file are parsed here.  Not all
 *  entities are handled here, since some may reside in the BLOCKS section.
 *************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "entities.h"
#include "poly.h"
#include "polyline.h"
#include "tables.h"



/*
**  Functions
*/

static void load_generic_entity( Layer *layer, STRPTR type )
{
   Polygon poly = { 0, 0, { 0 } };
   Group group;

   peek_group( &group );
   while( NOT IS_NEXT( group ) )
   {
      Point point;

      if( IS_COORD( group ) )
      {
         load_point( &point );
         if( layer )
            poly.index[ poly.numPoints++ ] = find_point( layer, &point );
       }      
      else
      {
         load_group( &group );
         if( IS_COLOR( group ) )
            poly.color = atoi( group.value );
      }

      peek_group( &group );
   }

   if( layer )
   {
      message( "    Found entity: %s", type );
      add_polygon( layer, &poly );
   }
}



static void load_polyline_entity( Layer *layer )
{
   Group group;
   PolylineType type = UNKNOWN_POLYLINE;
   UWORD m, n;

   peek_group( &group );
   while( NOT IS_NEXT( group ) )
   {
      load_group( &group );
      if( IS_INTEGER( group ) )
         type = atoi( group.value );
      else if( IS_ARRAYM( group ) )
         m = atoi( group.value );
      else if( IS_ARRAYN( group ) )
         n = atoi( group.value );
      
      peek_group( &group );
   }

   switch( type )
   {
      case CLOSED_POLYLINE:
         if( layer )
            message( "    Found entity: CLOSED POLYLINE" );
         load_generic_polyline_entity( layer );
         break;

      case THREED_POLYLINE:
         if( layer )
            message( "    Found entity: 3D POLYLINE" );
         load_generic_polyline_entity( layer );
         break;

      case POLYFACE_MESH:
         if( layer )
            message( "    Found entity: POLYFACE MESH" );
         load_polyface_mesh_entity( layer, m, n );
         break;

      default:
         if( layer && setup.verbose )
            message( "    Skipping unsupported POLYLINE" );

         load_group( &group );
         while( strcmp( group.value, "SEQEND" ) )
            load_group( &group );
         break;
   }
}



void load_entities( void )
{
   Group group;
   
   load_group( &group );
   while( strcmp( group.value, "ENDSEC" ) )
   {
      if( strcmp( group.value, "LINE" )   == 0 ||
          strcmp( group.value, "3DLINE" ) == 0 ||
          strcmp( group.value, "3DFACE" ) == 0 )
             load_generic_entity( find_layer(), group.value );
      else if( strcmp( group.value, "POLYLINE" ) == 0 )
         load_polyline_entity( find_layer() );
      else if( setup.verbose && IS_ENTITY( group ) )
         message( "    Skipping unsupported entity: %s", group.value );

      load_group( &group );
   }
}



void unload_entities( void )
{
}
