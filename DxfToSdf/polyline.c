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



#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "poly.h"
#include "polyline.h"
#include "tables.h"



/*
**  Functions
*/

void load_generic_polyline_entity( Layer *layer )
{
   Polygon poly = { 0, 0, { 0 } };
   Group group;

   load_group( &group );
   while( strcmp( group.value, "SEQEND" ) )
   {
      if( strcmp( group.value, "VERTEX" ) == 0 )
      {
         Point point;

         if( find_layer() == layer )
         {
            peek_group( &group );
            while( NOT IS_NEXT( group ) )
            {
               if( IS_COORD( group ) )
               {
                  load_point( &point );
                  if( layer )
                  {
                     if( setup.verbose )
                        message( "      Vertex at: (%f, %f, %f)",
                                 point.x, point.y, point.z );

                     poly.index[ poly.numPoints++ ] = find_point( layer, &point );
                     if( 2 == poly.numPoints )
                     {
                        add_polygon( layer, &poly );
                        poly.numPoints = 0;
                     }
                  }
               }
               else
                  load_group( &group );

               peek_group( &group );
            }
         }
         else
            error( "Layers don't coincide" );
      }
      else
         error( "Expected VERTEX directive" );

      load_group( &group );
   }
}



void load_polyface_mesh_entity( Layer *layer, UWORD m, UWORD n )
{
   UWORD i;
   WORD *index;

   index = malloc( m * sizeof( WORD ) );
   if( index )
   {
      Polygon poly = { 0, 0, { 0 } };
      Group group;

      if( layer && setup.verbose )
         message( "      Faces: %hu, Vertices: %hu", n, m );

      for( i = 0; i < m; i++ )
      {
         load_group( &group );
         if( strcmp( group.value, "VERTEX" ) == 0 )
         {
            Point point;

            if( find_layer() == layer )
            {
               peek_group( &group );
               while( NOT IS_NEXT( group ) )
               {
                  if( IS_COORD( group ) )
                  {
                     load_point( &point );
                     if( layer )
                        index[ i ] = find_point( layer, &point );
                  }
                  else
                     load_group( &group );

                  peek_group( &group );
               }
            }
            else
               error( "Layers don't coincide" );
         }
         else
            error( "Expected VERTEX directive" );
      }

      for( i = 0; i < n; i++ )
      {
         load_group( &group );
         if( strcmp( group.value, "VERTEX" ) == 0 )
         {
            if( find_layer() == layer )
            {
               poly.numPoints = 0;

               peek_group( &group );
               while( NOT IS_NEXT( group ) )
               {
                  load_group( &group );
                  if( IS_VERTEX( group ) )
                  {
                     WORD which = atoi( group.value );

                     poly.index[ poly.numPoints++ ] = index[ abs( which ) - 1 ];
                  }

                  peek_group( &group );
               }

               if( layer && poly.numPoints )
                  add_polygon( layer, &poly );
            }
            else
               error( "Layers don't coincide" );
         }
         else
            error( "Expected VERTEX directive" );
      }

      load_group( &group );
      if( strcmp( group.value, "SEQEND" ) )
         error( "Expected ENDSEQ directive" );

      free( index );
   }
   else
      error( "Out of memory" );
}
