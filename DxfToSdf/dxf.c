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
 *  This module handles loading a .DXF file.  Only groups that are
 *  significant to us are parsed.  All others are simply skipped.
 *
 *  Since a .DXF divides itself into LAYERs, each of which may contain a
 *  distinct object, we need to give the user the ability to specify
 *  which one they want to process.
 *************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "entities.h"
#include "header.h"
#include "tables.h"



/*
**  Functions
*/

void load_group( Group *group )
{
   STRPTR cp;
   long numFields;

   numFields = fscanf( setup.dxf, " %hd", &group->code );
   fgetc( setup.dxf );
   fgetc( setup.dxf );
   if( 1 != numFields )
      error( feof( setup.dxf ) ? "Premature end of file" : "Error during read" );
      
   cp = fgets( group->value, MAX_VALUELEN, setup.dxf );
   if( NOT cp )
      error( feof( setup.dxf ) ? "Premature end of file" : "Error during read" );

   while( *cp )
   {
      if( '\r' == *cp || '\n' == *cp )
         *cp = NUL;
      cp++;
   }
}



void peek_group( Group *group )
{
   long pos = ftell( setup.dxf );

   if( ERROR != pos )
   {
      load_group( group );
      if( fseek( setup.dxf, pos, SEEK_SET ) )
         error( "Error during seek" );
   }
   else
      error( "Error positioning file pointer" );
}



void load_point( Point *point )
{
   Group group;
   UBYTE i;
   
   for( i = 0; i < 3; i++ )
   {
      double value;
      
      load_group( &group );
      value = atof( group.value );
      
      if( IS_X( group ) )
         point->x = value;
      else if( IS_Y( group ) )
         point->y = value;
      else if( IS_Z( group ) )         
         point->z = value;
      else
         error( "Bad coordinate group" );
   }
}



static void load_blocks( void )
{
   Group group;

   load_group( &group );
   while( strcmp( group.value, "ENDSEC" ) )
   {
      if( setup.verbose && IS_LABEL( group ) )
         message( "    Found group: %s", group.value );

      load_group( &group );
   }
}



static void load_section( void )
{
   Group group;

   load_group( &group );
   message( "  Found section: %s", group.value );

   if( strcmp( group.value, "HEADER" ) == 0 )
      load_header();
   else if( strcmp( group.value, "TABLES" ) == 0 )
      load_tables();
   else if( strcmp( group.value, "BLOCKS" ) == 0 )
      load_blocks();
   else if( strcmp( group.value, "ENTITIES" ) == 0 )
      load_entities();
   else if( setup.verbose )
      message( "  Skipping unrecognized section: %s", group.value );
}



void load_dxf( void )
{
   Group group;

   message( "Parsing .DXF file..." );
   
   load_group( &group );
   while( strcmp( group.value, "EOF" ) )
   {
      if( strcmp( group.value, "SECTION" ) == 0 )
         load_section();
      else
         error( "Expected SECTION directive" );

      load_group( &group );
   }
}



void unload_dxf( void )
{
   unload_entities();
   unload_tables();
}
