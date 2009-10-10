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



#include <m68881.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "poly.h"
#include "sdf.h"
#include "sort.h"
#include "tables.h"



/*
**  Global variables
*/

static VAngle *vAngles = NULL;
static Polygon *polys;
static Point *points, *screen;
static double radius;



/*
**  Functions
*/

void free_angles( void )
{
   VAngle *vAngle;

   vAngle = vAngles;
   while( vAngle )
   {
      vAngles = vAngle->next;
      if( vAngle->polygons )
         free( vAngle->polygons );
      free( vAngle );

      vAngle = vAngles;
   }
}



void add_angle( UWORD angle )
{
   VAngle *vAngle, *next = vAngles, **prev = &vAngles;

   while( next )
   {
      if( next->angle == angle )
         return;
      else if( next->angle > angle )
         break;

      prev = &next->next;
      next = *prev;      
   }

   vAngle = calloc( 1, sizeof( VAngle ) );
   if( vAngle )
   {
      vAngle->next = next;
      vAngle->angle = angle;
      *prev = vAngle;
   }
   else
      error( "Out of memory" );
}



static void transform( Point *dest, const RPoint *src, UWORD angle )
{
   double theta = -PI*angle / 512.0;

   dest->x = src->x*cos( theta ) - src->y*sin( theta );
   dest->y = src->x*sin( theta ) + src->y*cos( theta ) + radius;
   dest->z = src->z - setup.cameraZ;

#if 0
   printf( "Camera Ang.: %hu\n"
           " Adj. Angle: %.2lf°\n"
           "Cylindrical: (%.2lf, %.2lf, %.2lf)\n"
           "  Cartesian: (%.2lf, %.2lf, %.2lf)\n\n",
           angle,
           (theta * 180.0) / PI,
           src->x, src->y, src->z,
           dest->x, dest->y, dest->z );
#endif
}



static void project( Point *dest, const Point *src )
{
   double div = src->y;

   if( 0.0 == div )
      div = 1.0;
   
   dest->x = src->x * 256.0 / div + 160.0;
   dest->y = -src->z * 256.0 / div + 120.0;

#if 0
   printf( "3D Point: (%.2lf, %.2lf, %.2lf)\n"
           "2D Point: (%.2lf, %.2lf)\n\n",
           src->x, src->y, src->z, dest->x, dest->y );
#endif
}



static BOOL is_visible( const Polygon *poly )
{
   Point a, b;
   double dot, len;

   a.x = -poly->center.x;
   a.y = -poly->center.y;
   a.z = -poly->center.z;

   b.x = poly->normal.x;
   b.y = poly->normal.y;
   b.z = poly->normal.z;

   len  = sqrt( a.x*a.x + a.y*a.y + a.z*a.z );
   len *= sqrt( b.x*b.x + b.y*b.y + b.z*b.z );

   dot  = a.x*b.x + a.y*b.y + a.z*b.z;
   dot /= len;

#if 0
   printf( " Normal: (%.2lf, %.2lf, %.2lf)\n"
           "  Sight: (%.2lf, %.2lf, %.2lf)\n"
           "Visible: %s\n\n",
           poly->normal.x, poly->normal.y, poly->normal.z,
           a.x, a.y, a.z, YESNO( dot >= 0.0 ) );
#endif

   return( (BOOL)(dot >= 0.0) );
}



static void compute_depth( const Polygon *poly, double *minD, double *maxD )
{
   const Point *point;
   double len, maxDist = TINY, minDist = HUGE;
   UWORD i;

   for( i = 0; i < poly->numPoints; i++ )
   {
      point = points + poly->index[ i ];
      len = sqrt( point->x*point->x + point->y*point->y + point->z*point->z );

      if( maxDist < len )
      {
         maxDist = len;
         *maxD = point->y;
      }

      if( minDist > len )
      {
         minDist = len;
         *minD = point->y;
      }
   }
}



static void compute_extent( const Polygon *poly, Point *pMin, Point *pMax )
{
   const Point * point;
   UWORD i;

   pMin->x =
   pMin->y = HUGE;

   pMax->x =
   pMax->y = TINY;

   for( i = 0; i < poly->numPoints; i++ )
   {
      point = screen + poly->index[ i ];

      pMin->x = min( pMin->x, point->x );
      pMin->y = min( pMin->y, point->y );

      pMax->x = max( pMax->x, point->x );
      pMax->y = max( pMax->y, point->y );
   }
}



static const Point *solve_plane( const Point *vertex, const Point *normal,
                                                           const Point *point )
{
   static Point result;
   double t;

   t  = normal->x*point->x + normal->y*point->y + normal->z*point->z;
   t /= normal->x*vertex->x + normal->y*vertex->y + normal->z*vertex->z;

   if( 0.0 <= t )
   {
      result.x = t*vertex->x;
      result.y = t*vertex->y;
      result.z = t*vertex->z;
   }
   else
   {
      result.x =
      result.y =
      result.z = HUGE;
   }

   return( &result );
}



static BOOL intersect( WORD s1, WORD e1, WORD s2, WORD e2 )
{
   double i, j, u, v;

   i = screen[ e1 ].x - screen[ s1 ].x;
   j = screen[ e1 ].y - screen[ s1 ].y;

   u = screen[ e2 ].x - screen[ s2 ].x;
   v = screen[ e2 ].y - screen[ s2 ].y;

   return( (BOOL)(0.0 != j*u - i*v) );
}



static BOOL overlap( const Polygon *poly1, const Polygon *poly2 )
{
   UWORD i, j;
   WORD s1, e1, s2, e2;

   for( i = 0; i < poly1->numPoints; i++ )
   {
      s1 = poly1->index[ i ];
      if( poly1->numPoints - 1 == i )
         e1 = *poly1->index;
      else
         e1 = poly1->index[ i + 1 ];

      for( j = 0; j < poly2->numPoints; j++ )
      {
         s2 = poly2->index[ j ];
         if( poly2->numPoints - 1 == i )
            e2 = *poly2->index;
         else
            e2 = poly2->index[ j + 1 ];

         if( intersect( s1, e1, s2, e2 ) )
            return( TRUE );
      }
   }

   return( FALSE );
}



static BOOL obscures( const Polygon *poly1, const Polygon *poly2 )
{
   Point p1Min, p2Min, p1Max, p2Max;
   const Point *point;
   double distP, distQ;
   UWORD i;

   /*  Phase 1  */
   compute_depth( poly1, &p1Min.y, &p1Max.y );
   compute_depth( poly2, &p2Min.y, &p2Max.y );
   if( p1Min.y >= p2Max.y )
      return( FALSE );

   /*  Phase 2  */
   compute_extent( poly1, &p1Min, &p1Max );
   compute_extent( poly2, &p2Min, &p2Max );
   if( p1Min.x >= p2Max.x || p1Max.x <= p2Min.x ||
       p1Min.y >= p2Max.y || p1Max.y <= p2Min.y )
      return( FALSE );

   /*  Phase 3  */
   if( poly2->normal.y )
   {
      for( i = 0; i < poly1->numPoints; i++ )
      {
         point = points + poly1->index[ i ];
         distP = sqrt( point->x*point->x + point->y*point->y + point->z*point->z );

         point = solve_plane( point, &poly2->normal, points + *poly2->index );
         distQ = sqrt( point->x*point->x + point->y*point->y + point->z*point->z );

         if( distP < distQ )
            break;
      }

      if( i == poly1->numPoints )
         return( FALSE );
   }

   /*  Phase 4  */
   if( poly1->normal.y )
   {
      for( i = 0; i < poly2->numPoints; i++ )
      {
         point = points + poly2->index[ i ];
         distQ = sqrt( point->x*point->x + point->y*point->y + point->z*point->z );

         point = solve_plane( point, &poly1->normal, points + *poly1->index );
         distP = sqrt( point->x*point->x + point->y*point->y + point->z*point->z );

         if( distQ > distP )
            break;
      }

      if( i == poly2->numPoints )
         return( FALSE );
   }

   /*  Phase 5  */
   return( overlap( poly1, poly2 ) );
}



static int is_closer( const Polygon *poly1, const Polygon *poly2 )
{
   const Point *c1, *c2;
   double d1, d2;

   c1 = &poly1->center;
   c2 = &poly2->center;

   d1 = sqrt( c1->x*c1->x + c1->y*c1->y + c1->z*c1->z );
   d2 = sqrt( c2->x*c2->x + c2->y*c2->y + c2->z*c2->z );

   if( d1 < d2 )
      return( 1 );
   else if( d1 > d2 )
      return( -1 );
   else
      return( 0 );
}



static int compare_polys( const void *item1, const void *item2 )
{
   const Polygon *poly1, *poly2;

   poly1 = polys + *((const WORD *)item1);
   poly2 = polys + *((const WORD *)item2);

   if( obscures( poly1, poly2 ) )
      return( 1 );
   else if( obscures( poly2, poly1 ) )
      return( -1 );
   else
      return( is_closer( poly1, poly2 ) );
}



static void cull_polys( WORD *visible, UWORD numPolys )
{
   Polygon *poly;
   UWORD i;

   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
   {
      if( setup.cull && NOT (POLYF_ALWAYS & poly->flags) )
      {
         if( is_visible( poly ) )
            visible[ i ]++;
      }
      else
         visible[ i ]++;
   }
}



static void convert_polys( const RPolygon *rPolys, UWORD numPolys )
{
   Polygon *poly;
   UWORD i;

   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
   {
      poly->numPoints = rPolys[ i ].numPoints;
      poly->flags = rPolys[ i ].flags;
      memcpy( (void *)poly->index, (void *)rPolys[ i ].index,
                                    poly->numPoints * sizeof( WORD ) );

      compute_center( poly, points );
      compute_normal( poly, points );
   }
}



VAngle *sort_polys( const RPolygon *rPolys, UWORD numPolys,
                                     const RPoint *rPoints, UWORD numPoints )
{
   if( vAngles )
   {
      VAngle *vAngle = vAngles;
      UWORD i, numVis;
      WORD *visible;

      message( "    Sorting polygons" );

      polys = calloc( numPolys, sizeof( Polygon ) );
      points = calloc( numPoints, sizeof( Point ) );
      screen = calloc( numPoints, sizeof( Point ) );
      visible = calloc( numPolys, sizeof( WORD ) );
      if( NOT polys || NOT points || NOT screen || NOT visible )
         error( "Out of memory" );

      radius = TINY;
      for( i = 0; i < numPoints; i++ )
         radius = max( radius, rPoints[ i ].dist );
      radius += 1500;

      while( vAngle )
      {
         UWORD end, start = vAngle->angle;

         memset( (void *)visible, 0, numPolys * sizeof( WORD ) );
         if( vAngle->next )
            end = vAngle->next->angle;
         else
            end = 1024;

         for( i = 0; i < numPoints; i++ )
            transform( points + i, rPoints + i, start );
         convert_polys( rPolys, numPolys );
         cull_polys( visible, numPolys );

         for( i = 0; i < numPoints; i++ )
            transform( points + i, rPoints + i, end & 1023 );
         convert_polys( rPolys, numPolys );
         cull_polys( visible, numPolys );

         for( i = 0; i < numPoints; i++ )
         {
            transform( points + i, rPoints + i, start );
            project( screen + i, points + i );
         }
         convert_polys( rPolys, numPolys );

         for( i = 0, numVis = 0; i < numPolys; i++ )
            if( visible[ i ] )
               visible[ numVis++ ] = i;

         qsort( visible, numVis, sizeof( WORD ), compare_polys );

         vAngle->numPolys = numVis;
         if( numVis )
         {
            vAngle->polygons = calloc( numVis, sizeof( WORD ) );
            if( NOT vAngle->polygons )
               error( "Out of memory" );

            memcpy( (void *)vAngle->polygons, (void *)visible,
                                                   numVis * sizeof( WORD ) );
         }

         vAngle = vAngle->next;
      }

      free( visible );
      free( screen );
      free( points );
      free( polys );
   }

   return( vAngles );
}
