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



#include <math.h>
#include <m68881.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "edge.h"
#include "poly.h"
#include "tables.h"



/*
**  Functions
*/

double normalize( Point *point )
{
   double mag;

   mag = sqrt( point->x*point->x + point->y*point->y + point->z*point->z );
   if( mag )
   {
      if( 2048.0 > mag )
      {
         mag = 2048.0 / mag;
         point->x *= mag;
         point->y *= mag;
         point->z *= mag;
      }
      else
      {
         mag /= 2048.0;
         point->x /= mag;
         point->y /= mag;
         point->z /= mag;
      }
   }

   return( mag );
}



void compute_center( Polygon *poly, const Point *points )
{
   Point total;
   UWORD i;

   total.x =
   total.y =
   total.z = 0.0;

   for( i = 0; i < poly->numPoints; i++ )
   {
      total.x += points[ poly->index[ i ] ].x;
      total.y += points[ poly->index[ i ] ].y;
      total.z += points[ poly->index[ i ] ].z;
   }

   if( i )
   {
      total.x /= i;
      total.y /= i;
      total.z /= i;
   }

   poly->center.x = total.x;
   poly->center.y = total.y;
   poly->center.z = total.z;
}



void compute_normal( Polygon *poly, const Point *points )
{
   if( 2 < poly->numPoints )
   {
      Point v1, v2, v3, prod;
      WORD index = 0;
      double mag;

      v1 = points[ poly->index[ index++ ] ];
      v2 = points[ poly->index[ index++ ] ];
      v3 = points[ poly->index[ index++ ] ];

      do
      {
         v2.x -= v1.x;
         v2.y -= v1.y;
         v2.z -= v1.z;

         v3.x -= v1.x;
         v3.y -= v1.y;
         v3.z -= v1.z;

         prod = *cross_product( &v2, &v3 );
         mag = sqrt( prod.x*prod.x + prod.y*prod.y + prod.z*prod.z );

         v1 = v2;
         v2 = v3;
         v3 = points[ poly->index[ index++ ] ];
      }
      while( poly->numPoints >= index && 0.0 == mag );

      if( mag )
      {
         poly->normal.x = prod.x;
         poly->normal.y = prod.y;
         poly->normal.z = prod.z;
      }
      else
         poly->flags |= POLYF_ALWAYS;
   }
   else
   {
      poly->flags |= POLYF_ALWAYS;

      poly->normal.x =
      poly->normal.y =
      poly->normal.z = 0.0;
   }

   if( POLYF_ALWAYS & poly->color )
   {
      poly->flags |= POLYF_ALWAYS;
      poly->color &= ~POLYF_ALWAYS;
   }
}



BOOL compare_points( const Point *pt1, const Point *pt2, double thresh )
{
   double dx, dy, dz;

   dx = pt1->x - pt2->x;
   dy = pt1->y - pt2->y;
   dz = pt1->z - pt2->z;

   return( (BOOL)(thresh > sqrt( dx*dx + dy*dy + dz*dz )) );
}



const Point *cross_product( const Point *a, const Point *b )
{
   static Point result;

   result.x = a->y*b->z - a->z*b->y;
   result.y = a->z*b->x - a->x*b->z;
   result.z = a->x*b->y - a->y*b->x;

   if( NOT normalize( &result ) )
   {
      result.x =
      result.y =
      result.z = 0.0;
   }

   return( &result );
}



BOOL concave_poly( const Polygon *poly, const Point *points )
{
   const Point *o;
   Point a, b;
   UWORD i;
   WORD index[ MAX_POLY_POINTS + 2 ];
   BOOL concave = FALSE;

   for( i = 1; i < poly->numPoints + 1; i++ )
      index[ i ] = poly->index[ i - 1 ];
   *index = index[ i - 1 ];
   index[ i ] = *poly->index;

   for( i = 0; i < poly->numPoints && NOT concave; i++ )
   {
      a = points[ index[ i ] ];
      o = points + index[ i + 1 ];
      b = points[ index[ i + 2 ] ];

      a.x -= o->x;
      a.y -= o->y;
      a.z -= o->z;

      b.x -= o->x;
      b.y -= o->y;
      b.z -= o->z;

      o = cross_product( &a, &b );
      if( compare_points( o, &poly->normal, setup.normThresh ) )
         concave = TRUE;
   }

   return( concave );
}



static void prune_polys( Layer *layer )
{
   Polygon *poly;
   UWORD i;
   WORD *index;

   index = calloc( layer->numPoints + 1, sizeof( WORD ) );
   if( NOT index )
      error( "Out of memory" );

   message( "    Pruning polygons" );
   if( setup.verbose )
      message( "      Eliminating duplicate points" );

   for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
   {
      UWORD j, k;

      for( j = 0; j < poly->numPoints; j++ )
         index[ j ] = poly->index[ j ];
      index[ j ] = *poly->index;

      for( j = 0, k = 0; j < poly->numPoints; j++ )
      {
         if( index[ j ] != index[ j + 1 ] )
            poly->index[ k++ ] = index[ j ];
      }
      poly->numPoints = k;
   }

   if( setup.verbose )
      message( "      Eliminating duplicate polygons" );

   for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
   {
      Polygon *poly2;
      UWORD j, k;

      poly2 = poly + 1;
      for( j = i + 1; j < layer->numPolys && poly->numPoints; j++, poly2++ )
      {
         UWORD match = 0;

         memset( (void *)index, 0, (layer->numPoints + 1) * sizeof( WORD ) );
         for( k = 0; k < poly->numPoints; k++ )
            index[ poly->index[ k ] ]++;

         for( k = 0; k < poly2->numPoints; k++ )
            if( index[ poly2->index[ k ] ] )
               match++;

         if( 2 < match )
         {
            if( compare_points( &poly->normal, &poly2->normal, setup.normThresh ) )
            {
               if( match == poly->numPoints )
                  poly->numPoints = 0;
               else if( match == poly2->numPoints )
                  poly2->numPoints = 0;
               else if( setup.verbose )
                  message( "        WARNING:  coplanar overlapping polygons" );
            }
         }
         else
         {
            if( match == poly->numPoints )
               poly->numPoints = 0;
            else if( match == poly2->numPoints )
               poly2->numPoints = 0;
         }
      }
   }

   free( index );
}



static void coalesce_polys( Layer *layer )
{
   Polygon *poly, *big;
   UWORD i, numNew = 0;
   WORD *coplanar, *touching;
   BOOL *removed, *points;

   message( "    Coalescing coplanar polygons" );

   big = calloc( setup.maxPolys, sizeof( Polygon ) );
   coplanar = calloc( setup.maxPolys, sizeof( WORD ) );
   touching = calloc( setup.maxPolys, sizeof( WORD ) );
   removed = calloc( setup.maxPolys, sizeof( BOOL ) );
   points = calloc( layer->numPoints, sizeof( BOOL ) );

   if( NOT big || NOT coplanar || NOT touching || NOT removed || NOT points )
      error( "Out of memory" );

   memset( (void *)removed, FALSE, sizeof( removed ) );
   for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
      if( NOT poly->numPoints )
         removed[ i ] = TRUE;

   for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
   {
      Polygon *poly2;
      UWORD j, numFlat = 0, numTouch = 0;
      
      if( removed[ i ] )
         continue;

      for( j = i + 1, poly2 = poly + 1; j < layer->numPolys; j++, poly2++ )
      {
         if( removed[ j ] )
            continue;

         if( poly->color == poly2->color &&
             compare_points( &poly->normal, &poly2->normal, setup.normThresh ) )
               coplanar[ numFlat++ ] = j;
      }

      touching[ numTouch++ ] = i;
      big[ numNew ] = *poly;

      for( j = 0; j < numTouch; j++ )
      {
         UWORD k;

         memset( (void *)points, 0, layer->numPoints * sizeof( BOOL ) );
         poly2 = layer->polygons + touching[ j ];

         for( k = 0; k < poly2->numPoints; k++ )
            points[ poly2->index[ k ] ] = TRUE;

         for( k = 0; k < numFlat; k++ )
         {
            if( INVALID_INDEX != coplanar[ k ] )
            {
               Polygon *dest = layer->polygons + coplanar[ k ];
               UWORD l, match = 0;

               for( l = 0; l < dest->numPoints; l++ )
                  if( points[ dest->index[ l ] ] )
                     match++;

               if( 1 < match )
               {
                  touching[ numTouch++ ] = coplanar[ k ];
                  if( setup.convex )
                  {
                     combine_edges( big + numNew, layer, touching, numTouch );

                     if( concave_poly( big + numNew, layer->points ) )
                        numTouch--;
                     else
                        coplanar[ k ] = INVALID_INDEX;
                  }
                  else
                     coplanar[ k ] = INVALID_INDEX;
               }
            }
         }
      }

      combine_edges( big + numNew++, layer, touching, numTouch );
      for( j = 0; j < numTouch; j++ )
         removed[ touching[ j ] ] = TRUE;
   }

   for( i = 0; i < numNew; i++ )
   {
      compute_center( big + i, layer->points );
      layer->polygons[ i ] = big[ i ];
   }
   layer->numPolys = i;

   free( points );
   free( removed );
   free( touching );
   free( coplanar );
   free( big );
}



static void prune_points( Layer *layer )
{
   Point *points;
   UWORD *index;
   BOOL *used;

   points = calloc( layer->numPoints, sizeof( Point ) );
   index = calloc( layer->numPoints, sizeof( UWORD ) );
   used = calloc( layer->numPoints, sizeof( BOOL ) );

   if( points && index && used )
   {
      Polygon *poly;
      Point *point;
      UWORD i, curIndex;

      for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
      {
         UWORD j;

         for( j = 0; j < poly->numPoints; j++ )
         {
            if( NOT used[ poly->index[ j ] ] )
               used[ poly->index[ j ] ] = TRUE;
         }
      }

      for( i = 0, point = points, curIndex = 0; i < layer->numPoints; i++ )
      {
         if( used[ i ] )
         {
            *point++ = layer->points[ i ];
            index[ i ] = curIndex++;
         }
      }

      for( i = 0; i < curIndex; i++ )
         layer->points[ i ] = points[ i ];
      layer->numPoints = i;

      for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
      {
         UWORD j;

         for( j = 0; j < poly->numPoints; j++ )
            poly->index[ j ] = index[ poly->index[ j ] ];
      }

      free( used );
      free( index );
      free( points );
   }
   else
      error( "Out of memory" );
}



void weld_polys( void )
{
   Layer *layer = get_layer();

   message( "Welding polygons" );
   while( layer )
   {
      if( layer->numPolys )
      {
         message( "  Layer %s:", layer->name );

         prune_polys( layer );
         if( setup.coalesce )
            coalesce_polys( layer );

         if( layer->numPoints )
            prune_points( layer );

#if 0
         {
            UWORD i, j;

            for( i = 0; i < layer->numPoints; i++ )
               printf( "Point %hu: (%.2lf, %.2lf, %.2lf)\n", i,
                  layer->points[ i ].x, layer->points[ i ].y, layer->points[ i ].z );

            for( i = 0; i < layer->numPolys; i++ )
            {
               printf( "Poly %hu:\n"
                       "  Normal: (%.2lf, %.2lf, %.2lf)\n"
                       "  Points: ",
                  i, layer->polygons[ i ].normal.x, layer->polygons[ i ].normal.y,
                  layer->polygons[ i ].normal.z );

               for( j = 0; j < layer->polygons[ i ].numPoints; j++ )
                  printf( "%hd ", layer->polygons[ i ].index[ j ] );
               printf( "\n" );
            }
         }
#endif
      }

      layer = layer->next;
   }
}
