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
 *  This program converts AutoCAD-format .DXF files into data useful to
 *  RSpace.
 *
 *  Polygons are parsed from the .DXF file, then welded together where
 *  possible, since shapes are triangulated in the CAD program.  The
 *  resultant (possibly concave) polygons are then re-split into convex
 *  ones, ensuring that each shape is formed from an optimal set of poly-
 *  gons.
 *
 *  Once split, the polygons are pre-sorted for each viewing angle and
 *  saved as an .SDF file.
 *
 *  The original color information for each polygon is retained and
 *  saved unaltered, with a single exception.  The high-bit of the color
 *  value is used to indicate light-source shading, and is stripped
 *  before each polygon is written.
 *************************************************************************/



#include <dos.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include "dxftosdf.h"
#include "dxf.h"
#include "poly.h"
#include "sdf.h"
#include "sort.h"



/*
**  Global variables
*/

static char inputFile[ FMSIZE ] = { NUL };
static char outputFile[ FMSIZE ] = { NUL };
static UBYTE *buffer; 

Setup setup =
   {
      /*
      **  Default setup...
      */
      NULL, NULL,                   /*  dxf, sdf  */
      { NUL },                      /*  layer  */
      NORM_THRESH,                  /*  normThresh  */
      WELD_THRESH,                  /*  weldThresh  */
      MAX_POLYS,                    /*  maxPolys  */
      0, NULL,                      /*  bufSize, buffer  */
      NUM_ANGLES,                   /*  angles  */
      CAM_HEIGHT,                   /*  cameraZ  */

      FALSE,                        /*  center  */
      TRUE,                         /*  coalesce  */
      TRUE,                         /*  convex  */
      FALSE,                        /*  cull  */
      FALSE,                        /*  intel  */
      TRUE,                         /*  output  */
      TRUE,                         /*  presort  */
      FALSE,                        /*  quiet  */
      FALSE,                        /*  super  */
      FALSE                         /*  verbose  */
   };



/*
**  Functions
*/

static void quick_usage( void )
{
   puts( "DxfToSdf\n"
         "Copyright © 1993  Dynamix, Inc.\n"
         "\n"
         "Usage: DxfToSdf [-abcfhijlmnopqrsuvwx] <infile[.dxf]> [outfile[.sdf]]\n"
         "\n"
         "For more information, use DxfToSdf -h\n" );

   exit( 5 );
}



static void usage( void )
{
   printf( "DxfToSdf\n"
           "Copyright © 1993  Dynamix, Inc.\n"
           "\n"
           "This program translates AutoCAD shapes into a form usable by\n"
           "RSpace® on the SEGA Genesis®.  Polygons may be pre-sorted for\n"
           "each viewing angle.  If light-source shading is enabled, bit 7\n"
           "of each color is used to indicate a shaded polygon, and is\n"
           "stripped.  If not, all color information is passed unaltered.\n"
           "\n"
           "Usage: DxfToSdf [-flag[-]] [-option<value>] <infile[.dxf]> "
           "[outfile[.sdf]]\n"
           "\n"
           "Where flag may be one or more of:\n"
           "   c   coalesce polygons where possible (default)\n"
           "   h   display this message\n"
           "   i   output Intel format (little-endian)\n"
           "   j   show conversion flags and options\n"
           "   o   create output file (default)\n"
           "   p   presort polygons (default)\n"
           "   q   be quiet (display only error messages)\n"
           "   r   center output at the origin\n"
           "   s   make a supershape from multiple layers\n"
           "   u   cull backfaces during sort\n"
           "   v   be verbose\n"
           "   x   make polygons convex (default)\n"
           "\n"
           "And options include:\n"
           "   a   number of viewing angles (default %hu)\n"
           "   b   file buffer size (default %hu bytes)\n"
           "   f   special viewing angle\n"
           "   l   name of layer to process\n"
           "   m   maximum polygons per layer (default %hu)\n"
           "   n   normal threshhold (default %.2lf)\n"
           "   w   weld threshhold (default %.2lf)\n"
           "   z   camera height for sorting (default %hu)\n"
           "\n"
           "Examples:\n"
           "   DxfToSdf -i -d- object.dxf -lGLOBE\n"
           "   DxfToSdf -o- mech -v -m150\n"
           "   DxfToSdf robot.dxf robot.sdf -v -w1.4356\n"
           "\n",
           NUM_ANGLES, BUFSIZ, MAX_POLYS, NORM_THRESH, WELD_THRESH, CAM_HEIGHT );

   exit( 5 );
}



static void show_config( void )
{
   printf( "Configuration:\n"
           "  Coalesce Polygons: %-3.3s      File Buffer Size:  %hu\n"
           "  Generate Output:   %-3.3s      Maximum Polygons:  %hu\n"
           "  Presort Polygons:  %-3.3s      Normal Threshhold: %.2lf\n"
           "  Cull Backfaces:    %-3.3s      Weld Threshhold:   %.2lf\n"
           "  Convex Polygons:   %-3.3s      Camera Height:     %hu\n"
           "  Make Supershape:   %-3.3s\n"
           "\n"
           "  Data Format:   %s\n"
           "  Layer Name:    %s\n"
           "  Center Output: %-3.3s\n"
           "\n"
           "  Input File:  %s\n"
           "  Output File: %s\n"
           "\n",
           YESNO( setup.coalesce ), setup.bufSize ? setup.bufSize : BUFSIZ,
           YESNO( setup.output ), setup.maxPolys,
           YESNO( setup.presort ), setup.normThresh,
           YESNO( setup.cull ), setup.weldThresh,
           YESNO( setup.convex ), setup.cameraZ,
           YESNO( setup.super ),
           setup.intel ? "Intel" : "Motorola",
           *setup.layer ? setup.layer : "ALL LAYERS",
           YESNO( setup.center ),
           inputFile,
           outputFile );
}



static void make_ext( STRPTR fileName, const STRPTR extension )
{
   STRPTR cp = strchr( fileName, '.' );
   
   if( cp )
      strcpy( cp + 1, extension );
   else
      strcat( strcat( fileName, "." ), extension );
}



static void parse( UWORD argc, const STRPTR argv[] )
{
   UWORD i, step = NUM_ANGLES;
   BOOL show = FALSE;

   for( i = 1; i < argc; i++ )
   {
      STRPTR arg = argv[ i ];
      
      if( '-' == *arg )
      {
         char option = arg[ 1 ];
         BOOL state = ('-' != arg[ 2 ]);

         switch( option )
         {
            case 'a':
               step = (UWORD)atof( arg + 2 );
               break;

            case 'b':
#ifndef DEVEL
               setup.bufSize = (UWORD)atof( arg + 2 );
#else
               message( "-b option not supported" );
#endif
               break;

            case 'c':
               setup.coalesce = state;
               if( NOT state )
                  setup.convex = state;
               break;

            case 'f':
               add_angle( (UWORD)atof( arg + 2 ) );
               break;
            
            case 'h':
               usage();
               /*  never gets here  */

            case 'i':
               setup.intel = state;
               break;

            case 'j':
               show = state;
               break;

            case 'l':
               strncpy( setup.layer, arg + 2, MAX_VALUELEN );
               break;

            case 'm':
               setup.maxPolys = (UWORD)atof( arg + 2 );
               break;

            case 'n':
               setup.normThresh = atof( arg + 2 );
               break;

            case 'o':
               setup.output = state;
               break;

            case 'p':
               setup.presort = state;
               break;

            case 'q':
               setup.quiet = state;
               break;

            case 'r':
               setup.center = state;
               break;

            case 's':
               setup.super = state;
               break;

            case 'u':
               setup.cull = state;
               break;

            case 'v':
               setup.verbose = state;
               break;

            case 'w':
               setup.weldThresh = atof( arg + 2 );
               break;

            case 'x':
               setup.convex = state;
               break;

            case 'z':
               setup.cameraZ = (UWORD)atof( arg + 2 );
               break;

            default:
               error( "Unrecognized option: %c", option );
         }
      }
      else if( '?' != *arg )
      {
         if( NOT *inputFile )
            strcpy( inputFile, arg );
         else if( NOT *outputFile )
            strcpy( outputFile, arg );
         else
            error( "Multiple input/output files not allowed" );
      }
      else
         quick_usage();
   }

   if( step )
   {
      step = 1024 / step;
      for( i = 0; i < 1024; i += step )
         add_angle( i );
   }

   if( setup.cull && NOT setup.presort )
      error( "Must presort to cull backfaces (use -p or -u-)" );

   if( setup.convex && NOT setup.coalesce )
      error( "Must coalesce to make convex (use -c or -x-)" );

   if( setup.presort && NOT setup.convex )
      error( "Only convex polygons may be presorted (use -x or -p-)" );

   if( setup.super && *setup.layer )
   {
      message( "Supershapes use all layers (-l option ignored)" );
      *setup.layer = NUL;
   }

   if( NOT *inputFile )
      error( "No input file specified" );
   make_ext( inputFile, "dxf" );
      
   if( NOT *outputFile )
      strcpy( outputFile, inputFile );
   make_ext( outputFile, "sdf" );

   if( show )
      show_config();
}



void error( const char *format, ... )
{
   char buffer[ MAX_ERRORLEN ];
   va_list argp;

   va_start( argp, format );
   vsprintf( buffer, format, argp );
   va_end( argp );

   fprintf( stderr, "ERROR: %s\n", buffer );
   exit( 10 );
}



void message( const char *format, ... )
{
   if( NOT setup.quiet )
   {
      char buffer[ MAX_MESSAGELEN ];
      
      va_list argp;
   
      va_start( argp, format );
      vsprintf( buffer, format, argp );
      va_end( argp );
   
      puts( buffer );
   }
}



void main( UWORD argc, STRPTR argv[] )
{
   parse( argc, argv );

   if( setup.bufSize )
      setup.buffer = malloc( setup.bufSize );

#if 1
   setup.dxf = fopen( inputFile, "rb" );
   if( setup.dxf )
   {
      if( setup.buffer )
         setvbuf( setup.dxf, setup.buffer, _IOFBF, setup.bufSize );

      load_dxf();
      fclose( setup.dxf );
   }
   else
      error( "Couldn't open input file: %s", inputFile );
#else
   build_shape();
#endif

   if( setup.output )
   {
      weld_polys();
   
      setup.sdf = fopen( outputFile, "wb" );
      if( setup.sdf )
      {
         if( setup.buffer )
            setvbuf( setup.sdf, setup.buffer, _IOFBF, setup.bufSize );

         save_sdf();
         fclose( setup.sdf );
      }
      else
         error( "Couldn't open output file: %s", outputFile );
   }
   
   unload_dxf();
   if( setup.buffer )
      free( setup.buffer );
}
