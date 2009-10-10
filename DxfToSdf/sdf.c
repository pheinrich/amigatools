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



#include <math.h>
#include <m68881.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dtan.h"
#include "dxf.h"
#include "poly.h"
#include "sdf.h"
#include "sort.h"
#include "tables.h"



/*
**  Functions
*/

static void put_byte( BYTE value )
{
   fputc( value, setup.sdf );
}



static void put_word( WORD value )
{
   UWORD i;

   if( setup.intel )
   {
      for( i = 0; i < sizeof( WORD ); i++, value >>= 8 )
         fputc( value & 0xff, setup.sdf );
   }
   else
   {
      for( i = 0; i < sizeof( WORD ); i++, value <<= 8 )
         fputc( (value & 0xff00) >> 8, setup.sdf );
   }
}



static void put_long( LONG value )
{
   UWORD i;

   if( setup.intel )
   {
      for( i = 0; i < sizeof( LONG ); i++, value >>= 8 )
         fputc( value & 0xff, setup.sdf );
   }
   else
   {
      for( i = 0; i < sizeof( LONG ); i++, value <<= 8 )
         fputc( (value & 0xff000000) >> 24, setup.sdf );
   }
}



static void convert_point( RPoint *rPoint, const Point *point )
{
   double angle = PID2 - atan2( point->y, point->x );

   if( 0.0 > angle )
      angle += PI + PI;

   rPoint->dist = sqrt( point->x*point->x + point->y*point->y );
   rPoint->angle = (512.0*angle) / PI;

   rPoint->x = point->x;
   rPoint->y = point->y;
   rPoint->z = point->z;
}



static void convert_normal( RNormal *rNormal, Point *source )
{
   RPoint rPoint;
   Point point;

   convert_point( &rPoint, source );
   rNormal->azimuth = rPoint.angle / 4;

   point.x = rPoint.dist;
   point.y = source->z;
   convert_point( &rPoint, &point );

   if( rPoint.angle )
      rNormal->elevation = (rPoint.angle - 512.0) / 2;
   else
      rNormal->elevation = 255;
}



static void write_sdf( RPolygon *polys, UWORD numPolys, RPoint *points, UWORD numPoints )
{
   RPolygon *poly;
   RPoint *point;
   VAngle *vAngle, *curAng;
   ULONG offset, angleOff;
   UWORD i, j, numAngles = 0, pad = 0;
   double minZ, maxZ, maxD;

   maxZ =
   maxD = TINY;
   minZ = HUGE;

   for( i = 0; i < numPoints; i++ )
   {
      minZ = min( points[ i ].z, minZ );
      maxZ = max( points[ i ].z, maxZ );
      maxD = max( points[ i ].dist, maxD );
   }
   minZ = abs( minZ );

   i = numPoints;
   if( setup.presort )
   {
      vAngle = sort_polys( polys, numPolys, points, numPoints );
      i |= SDFF_PRESORT;
   }
   if( setup.super )
      i |= SDFF_SUPER;
   put_word( i );

   if( setup.verbose )
      message( "    Writing table offset information" );
   else
      message( "    Writing header" );

   offset = CENTER_LIST + (numPolys * (3 * sizeof( WORD ))) +
                                          (numPoints * 2 * sizeof( WORD ));
   put_long( offset );
   offset += numPoints * sizeof( WORD );
   put_long( offset ); // Polylist offset

   offset += sizeof( WORD ) + numPolys * sizeof( LONG );
   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
   {
      poly->offset = offset;
      offset += (3 + poly->numPoints) * sizeof( WORD );
   }
   put_long( offset ); // Auxlist offset
   put_long( CENTER_LIST + numPolys * (3 * sizeof( WORD )) ); // Polarlist offset

   put_word( numPolys );
   put_long( (ULONG)CENTER_LIST );

   put_word( (WORD)(maxZ + minZ) );
   put_word( (WORD)maxD );

   if( setup.presort )
   {
      offset += sizeof( UWORD );
      if( vAngle )
      {
         curAng = vAngle;
         while( curAng )
         {
            offset += sizeof( UWORD ) + sizeof( ULONG );
            numAngles++;

            curAng = curAng->next;
         }

         if( numAngles )
            offset -= sizeof( UWORD );

         angleOff = offset;
         curAng = vAngle;
         while( curAng->next )
         {
            offset += curAng->numPolys + 1;
            curAng = curAng->next;
         }
      }
      else
         offset += numPolys + 1;
   }

   if( setup.super )
   {
      while( offset & 3 )
      {
         pad++;
         offset++;
      }
      put_long( offset );
   }

   if( setup.verbose )
      message( "    Writing center points" );
   else
      message( "    Writing points data" );

   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
      put_word( (WORD)poly->center.dist );

   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
      put_word( (WORD)poly->center.angle );

   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
      put_word( (WORD)(poly->center.z + minZ) );

   if( setup.verbose )
      message( "    Writing polygonal points" );

   for( i = 0, point = points; i < numPoints; i++, point++ )
      put_word( (WORD)point->dist );

   for( i = 0, point = points; i < numPoints; i++, point++ )
      put_word( (WORD)point->angle );

   for( i = 0, point = points; i < numPoints; i++, point++ )
      put_word( (WORD)(point->z + minZ) );

   if( setup.verbose )
      message( "    Writing polygonal offsets" );

   put_word( numPolys );
   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
      put_long( poly->offset );

   message( "    Writing polygons" );
   for( i = 0, poly = polys; i < numPolys; i++, poly++ )
   {
      put_byte( (BYTE)poly->color );
      put_byte( (BYTE)poly->normal.azimuth );
      put_byte( (BYTE)poly->normal.elevation );
      put_byte( (BYTE)poly->flags );

      put_word( (WORD)poly->numPoints );

      for( j = 0; j < poly->numPoints; j++ )
         put_word( poly->index[ j ] );
   }

   if( setup.presort )
   {
      put_word( numAngles ? numAngles - 1 : 0 );
      if( vAngle )
      {
         curAng = vAngle;
         while( curAng )
         {
            if( curAng->angle )
               put_word( curAng->angle );
            curAng = curAng->next;
         }

         curAng = vAngle;
         if( curAng )
         {
            while( curAng->next )
               curAng = curAng->next;

            put_long( angleOff );
            angleOff += curAng->numPolys + 1;
         }

         curAng = vAngle;
         while( curAng->next )
         {
            put_long( angleOff );
            angleOff += curAng->numPolys + 1;

            curAng = curAng->next;
         }

         curAng = vAngle;
         while( curAng->next )
            curAng = curAng->next;

         put_byte( (BYTE)curAng->numPolys );
         for( i = 0; i < curAng->numPolys; i++ )
            put_byte( (BYTE)curAng->polygons[ i ] );

         curAng = vAngle;
         while( curAng->next )
         {
            put_byte( (BYTE)curAng->numPolys );
            for( i = 0; i < curAng->numPolys; i++ )
               put_byte( (BYTE)curAng->polygons[ i ] );

            curAng = curAng->next;
         }
      }
      else
      {
         put_byte( (BYTE)numPolys );
         for( i = 0; i < numPolys; i++ )
            put_byte( (BYTE)i );
      }

      free_angles();
   }

   if( setup.super )
      for( i = 0; i < pad; i++ )
         put_byte( 0 );
}



static void process_layer( Layer *layer, RPolygon *polys, RPoint *points,
                                          UWORD *numPolys, UWORD *numPoints )
{
   Polygon *poly;
   RPolygon *curPoly;
   RPoint *curPoint;
   UWORD i, j;

   if( setup.verbose )
      message( "    Converting to polar coordinates" );

   for( i = 0, poly = layer->polygons; i < layer->numPolys; i++, poly++ )
   {
      if( poly->numPoints )
      {
#ifdef DEVEL
         static UBYTE color = 0;
#endif

         curPoly = polys + *numPolys;
#ifdef DEVEL
         if( ++color & 15 == 0 )
            color = 1;
         curPoly->color = color;
#else
         curPoly->color = poly->color;
#endif
         convert_point( &curPoly->center, &poly->center );
         convert_normal( &curPoly->normal, &poly->normal );
         curPoly->numPoints = poly->numPoints;
         curPoly->flags = poly->flags;

         for( j = 0; j < poly->numPoints; j++ )
            curPoly->index[ j ] = poly->index[ j ] + *numPoints;

         curPoly++;
         (*numPolys)++;
      }
   }

   curPoint = points + *numPoints;
   for( i = 0; i < layer->numPoints; i++, curPoint++, (*numPoints)++ )
      convert_point( curPoint, layer->points + i );
}



static void center_shape( void )
{
   Layer *layer = get_layer();
   Point *point, average = { 0.0, 0.0, 0.0 };
   UWORD i, total = 0;

   while( layer )
   {
      for( i = 0, point = layer->points; i < layer->numPoints; i++, point++ )
      {
         average.x += point->x;
         average.y += point->y;
      }

      total += i;
      layer = layer->next;
   }

   if( total )
   {
      average.x /= total;
      average.y /= total;
   }

   layer = get_layer();
   while( layer )
   {
      for( i = 0, point = layer->points; i < layer->numPoints; i++, point++ )
      {
         point->x -= average.x;
         point->y -= average.y;
      }

      layer = layer->next;
   }
}



void save_sdf( void )
{
   Layer *layer = get_layer();
   RPolygon *polys;
   RPoint *points;
   UWORD numPolys = 0, numPoints = 0;

   if( setup.center )
      center_shape();

   if( setup.super )
   {
      message( "Saving supershape .SDF file..." );
      while( layer )
      {
         if( layer->numPolys && layer->numPoints )
         {
            polys = calloc( layer->numPolys, sizeof( RPolygon ) );
            points = calloc( layer->numPoints, sizeof( RPoint ) );
            if( NOT polys || NOT points )
               error( "Out of memory" );

            message( "  ShapeDef %s:", layer->name );

            numPolys  =
            numPoints = 0;
            if( NOT layer->next )
               setup.super = FALSE;

            process_layer( layer, polys, points, &numPolys, &numPoints );
            write_sdf( polys, numPolys, points, numPoints );

            free( points );
            free( polys );
         }
         else
            error( "No %s to process", layer->numPolys ? "points" : "polys" );

         layer = layer->next;
      }

      message( "    Faces: %hu, Vertices: %hu", numPolys, numPoints );
   }
   else
   {
      message( "Saving .SDF file..." );
      while( layer )
      {
         numPolys  += layer->numPolys;
         numPoints += layer->numPoints;

         layer = layer->next;
      }

      if( numPolys && numPoints )
      {
         polys = calloc( numPolys, sizeof( RPolygon ) );
         points = calloc( numPoints, sizeof( RPoint ) );
         if( NOT polys || NOT points )
            error( "Out of memory" );

         if( *setup.layer )
            message( "  Processing layer info" );
         else
            message( "  Merging layer info" );

         numPolys  =
         numPoints = 0;

         layer = get_layer();
         while( layer )
         {
            process_layer( layer, polys, points, &numPolys, &numPoints );
            layer = layer->next;
         }

         if( setup.verbose )
            message( "    Faces: %hu, Vertices: %hu", numPolys, numPoints );
         write_sdf( polys, numPolys, points, numPoints );

         free( points );
         free( polys );
      }
      else
         error( "No %s to process", numPolys ? "points" : "polys" );
   }
}
