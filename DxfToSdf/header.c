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
 *  The header portion of a .DXF file is parsed here.  Routines to read
 *  and set various header variables reside in this module.
 *************************************************************************/



#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "header.h"



/*
**  Global data
*/

static Header header;



/*
**  Functions
*/

static void set_entity_color( void )
{
   Group group;

   load_group( &group );
   if( IS_COLOR( group ) )
      header.color = atoi( group.value );
   else
      error( "Bad color group" );

   if( setup.verbose )
      message( "    ENTITY color: %hu", header.color );
}



static void set_extent_max( void )
{
   load_point( &header.max );

   if( setup.verbose )
      message( "    Maximum extent: (%f, %f, %f)",
               header.max.x, header.max.y, header.max.y );
}



static void set_extent_min( void )
{
   load_point( &header.min );

   if( setup.verbose )
      message( "    Minimum extent: (%f, %f, %f)",
               header.min.x, header.min.y, header.min.y );
}



static void set_face_shading( void )
{
   Group group;
   STRPTR shading[ NUM_SHADES ] =
      {
         "Faces shaded, edges entity color",
         "Faces shaded, edges black",
         "Faces transparent, edges entity color",
         "Faces entity color, edges black"
      };

   load_group( &group );
   if( IS_INTEGER( group ) )
      header.shade = atoi( group.value );
   else
      error( "Bad integer group" );

   if( setup.verbose )
      message( "    Face shading: %s", shading[ header.shade ] );
}



static void set_ucs_origin( void )
{
   load_point( &header.origin );

   if( setup.verbose )
      message( "    User origin: (%f, %f, %f)",
               header.origin.x, header.origin.y, header.origin.z );
}



static void set_ucs_x_dir( void )
{
   load_point( &header.x_dir );

   if( setup.verbose )
      message( "    User X axis: (%f, %f, %f)",
               header.x_dir.x, header.x_dir.y, header.x_dir.z );
}



static void set_ucs_y_dir( void )
{
   load_point( &header.y_dir );

   if( setup.verbose )
      message( "    User Y axis: (%f, %f, %f)",
               header.y_dir.x, header.y_dir.y, header.y_dir.z );
}



void load_header( void )
{
   Group group;

   load_group( &group );
   while( strcmp( group.value, "ENDSEC" ) )
   {
      if( strcmp( group.value, "$CECOLOR" ) == 0 )
         set_entity_color();
      else if( strcmp( group.value, "$EXTMAX" ) == 0 )
         set_extent_max();
      else if( strcmp( group.value, "$EXTMIN" ) == 0 )
         set_extent_min();
      else if( strcmp( group.value, "$SHADEEDGE" ) == 0 )
         set_face_shading();
      else if( strcmp( group.value, "$UCSORG" ) == 0 )
         set_ucs_origin();
      else if( strcmp( group.value, "$UCSXDIR" ) == 0 )
         set_ucs_x_dir();
      else if( strcmp( group.value, "$UCSYDIR" ) == 0 )
         set_ucs_y_dir();
      else if( setup.verbose && IS_VARIABLE( group ) )
         message( "    Skipping unsupported variable: %s", group.value );
         
      load_group( &group );
   }
}
