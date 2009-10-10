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
 *  General information useful to all modules of the program.  This
 *  includes types, defines, macros, etc.
 *************************************************************************/



#ifndef  _DXFTOSDF_H
#define  _DXFTOSDF_H


#include <stdio.h>

#include <exec/types.h>

#define  DEVEL



/*
**  Macros and magic numbers
*/

#define  ERROR                      -1
#define  NUL                        '\0'
#define  NOT                        !

#define  YESNO( a )                 ((a) ? "YES" : "NO")
#define  ONOFF( a )                 ((a) ? "ON" : "OFF")

#define  MAX_ERRORLEN               255
#define  MAX_MESSAGELEN             255
#define  MAX_VALUELEN               32

#define  MAX_POLY_POINTS            32
#define  MAX_POLYS                  64

#define  CAM_HEIGHT                 64
#define  NORM_THRESH                3.5
#define  NUM_ANGLES                 8
#define  WELD_THRESH                2.0



/*
**  Types and structures
*/

typedef struct setupTag
   {
      FILE *dxf, *sdf;              /*  file handles  */
      char layer[ MAX_VALUELEN ];   /*  name of layer to process  */
      double normThresh;            /*  normal threshhold  */
      double weldThresh;            /*  weld threshhold  */
      UWORD maxPolys;               /*  maximum polygons per layer  */
      UWORD bufSize;                /*  size of our file buffer  */
      UBYTE *buffer;                /*  our file buffer  */
      UWORD angles;                 /*  number of viewing angles  */
      UWORD cameraZ;                /*  camera height for sorting  */

      BOOL center;                  /*  center output at origin  */ 
      BOOL coalesce;                /*  coalesce polygons where possible  */
      BOOL convex;                  /*  make polygons convex  */
      BOOL cull;                    /*  cull backfaces during sort  */
      BOOL intel;                   /*  output Intel format (little-endian)  */
      BOOL output;                  /*  create output file  */
      BOOL presort;                 /*  presort polygons  */
      BOOL quiet;                   /*  be quiet (display only error messages)  */
      BOOL super;                   /*  make a supershape from multiple layers  */
      BOOL verbose;                 /*  be verbose  */
   }
   Setup;



/*
**  Function prototypes
*/

extern void error( const char *format, ... );
extern void message( const char *format, ... );



/*
**  Global variables
*/

extern Setup setup;



/*
**  .SDF File format:
**
**    dc.w  num points
**    dc.l  Zlist offset
**    dc.l  Polylist offset
**    dc.l  Auxlist offset
**    dc.l  Polarlist offset
**
**    dc.w  num polys
**    dc.l  Centerlist offset
**    dc.w  max height
**    dc.w  max radius
**    dc.l  Next ShapeDef offset (if supershape)
**
**   CenterList:
**    dc.w  dist  * num polys
**    dc.w  angle * num polys    [0, 1023]
**    dc.w  Z     * num polys
**
**   Polarlist:
**    dc.w  dist  * num points
**    dc.w  angle * num points   [0, 1023]
**
**   Zlist:
**    dc.w  Z     * num points
**
**   Polylist:
**    dc.w  num polys
**    dc.l  poly pointer * num polys
**
**   |dc.b  color               |
**   |dc.b  azimuth     [0, 255]|
**   |dc.b  elevation   [0, 255]|
**   |dc.b  flags               | * num polys
**   |                          |
**   |dc.w  count               |
**   |dc.w  index * count       |
**
**   Auxlist:
**    dc.w  num angles
**    dc.w  angle * num angles
**    dc.l  list pointer * num angles
**
**   |dc.b  count               | * num angles
**   |dc.b  index * count       |
**
**
**  Flags in num points:
**
**    0x2000   supershape
**    0x4000   presorted
**
**
**  Polygon flags:
**
**    0x80     always visible
**
*/


#endif   /*  _DXFTOSDF_H  */
