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



#include <m68881.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "poly.h"
#include "tables.h"



/*
**  Global variables
*/

static Layer *layers = NULL;



/*
**  Functions
*/

Layer *new_layer( void )
{
   Layer *table;
   
   table = calloc( 1, sizeof( Layer ) );
   if( table )
   {
      table->next = layers;
      table->polygons = calloc( setup.maxPolys, sizeof( Polygon ) );
      table->points = calloc( MAX_POLY_POINTS * setup.maxPolys, sizeof( Point ) );

      if( NOT table->polygons || NOT table->points )
         error( "Out of memory" );

      layers = table;
   }
   else
      error( "Out of memory" );
   
   return( table );
}



static void free_layers( void )
{
   Layer *table;
   
   table = layers;
   while( table )
   {
      layers = table->next;
      free( table->polygons );
      free( table->points );
      free( table );
      
      table = layers;
   }
}



static void load_layer_tables( void )
{
   Layer *table = NULL;
   Group group;
   UWORD numTables;

   load_group( &group );
   numTables = atoi( group.value );

   load_group( &group );
   while( strcmp( group.value, "ENDTAB" ) )
   {
      if( IS_NEXT( group ) )
      {
         if( numTables )
         {
            table = new_layer();
            numTables--;
         }
         else
         {
            if( setup.verbose )
               message( "      Skipping extraneous layer" );
            table = NULL;
         }
      }
      else if( table )
      {
         if( IS_NAME( group ) )
         {
            strcpy( table->name, group.value );
            message( "      Found layer: %s", table->name );
         }
      }
      else if( numTables )
         error( "Layer not yet defined" );

      load_group( &group );
   }
   
   if( numTables && setup.verbose )
      message( "      %hu Layer%s missing",
               numTables, numTables == 1 ? "" : "s" );
}



Layer *find_layer( void )
{
   Layer *layer;
   Group group;
   
   load_group( &group );
   if( IS_LAYER( group ) )
   {
      layer = layers;
      while( layer )
      {
         if( strcmp( group.value, layer->name ) == 0 )
            break;
            
         layer = layer->next;
      }

      if( layer )
      {
         if( *setup.layer && strcmp( setup.layer, layer->name ) )
            layer = NULL;
      }
      else
         error( "Undefined layer: %s", group.value );
   }
   else
      error( "Expected LAYER directive" );

   return( layer );
}



Layer *get_layer( void )
{
   Layer *layer = layers;

   while( layer )
   {
      if( NOT *setup.layer || strcmp( setup.layer, layer->name ) == 0 )
         break;

      layer = layer->next;
   }

   return( layer );
}



void add_polygon( Layer *layer, const Polygon *poly )
{
   if( setup.maxPolys > layer->numPolys )
   {
      Polygon *dest = layer->polygons + layer->numPolys++;
      UWORD i;

      for( i = 0; i < poly->numPoints; i++ )
         dest->index[ i ] = poly->index[ i ];

      dest->color = poly->color;
      dest->numPoints = i;

      compute_center( dest, layer->points );
      compute_normal( dest, layer->points );
   }
   else
      error( "Exceeded limit of %d polygons", setup.maxPolys );
}



WORD find_point( Layer *layer, const Point *point )
{
   Point *source;
   WORD i;

   for( i = 0, source = layer->points; i < layer->numPoints; i++, source++ )
      if( compare_points( source, point, setup.weldThresh ) )
         return( i );

   if( setup.maxPolys * MAX_POLY_POINTS > layer->numPoints )
   {
      Point *dest = &layer->points[ layer->numPoints ];

      dest->x = point->x;
      dest->y = point->y;
      dest->z = point->z;

      return( (WORD)layer->numPoints++ );
   }
   else
      return( INVALID_INDEX );
}



void load_tables( void )
{
   Group group;

   load_group( &group );
   while( strcmp( group.value, "ENDSEC" ) )
   {
      if( strcmp( group.value, "TABLE" ) == 0 )
      {
         load_group( &group );

         if( strcmp( group.value, "LAYER" ) == 0 )
         {
            message( "    Found table: LAYER" );
            load_layer_tables();
         }
         else
         {
            if( setup.verbose )
               message( "    Skipping unsupported table: %s", group.value );

            do
               load_group( &group );
            while( strcmp( group.value, "ENDTAB" ) );
         }
      }
      else
         error( "Expected TABLE directive" );

      load_group( &group );
   }
}



void unload_tables( void )
{
   free_layers();
}
