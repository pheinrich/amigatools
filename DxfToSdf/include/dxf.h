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



#ifndef  _DXF_H
#define  _DXF_H


#include <exec/types.h>

#include "dxftosdf.h"



/*
**  Macros and magic numbers
*/

#define  IS_ARRAYM( a )             (71 == (a).code)
#define  IS_ARRAYN( a )             (72 == (a).code)
#define  IS_COLOR( a )              (62 == (a).code)
#define  IS_COORD( a )              (9 < (a).code && 38 > (a).code)
#define  IS_ENTITY( a )             (0 == (a).code)
#define  IS_INTEGER( a )            (70 == (a).code)
#define  IS_LABEL( a )              (10 > (a).code)
#define  IS_LAYER( a )              (8 == (a).code)
#define  IS_NAME( a )               (2 == (a).code)
#define  IS_NEXT( a )               (0 == (a).code)
#define  IS_VARIABLE( a )           (9 == (a).code)
#define  IS_VERTEX( a )             (70 < (a).code && 75 > (a).code)
#define  IS_VERTLIST( a )           (66 == (a).code)
#define  IS_X( a )                  (9 < (a).code && 19 > (a).code)
#define  IS_Y( a )                  (19 < (a).code && 29 > (a).code)
#define  IS_Z( a )                  (29 < (a).code && 38 > (a).code)



/*
**  Types and structures
*/

typedef struct pointTag
   {
      double x, y, z;
   }
   Point;
   
typedef struct groupTag
   {
      UWORD code;
      char  value[ MAX_VALUELEN ];
   }
   Group;
   
   
   
/*
**  Function prototypes
*/

extern void load_group( Group *group );
extern void peek_group( Group *group );
extern void load_point( Point *point );
extern void load_dxf( void );
extern void unload_dxf( void );


#endif   /*  _DXF_H  */

