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



#ifndef  _HEADER_H
#define  _HEADER_H


#include "dxftosdf.h"
#include "dxf.h"



/*
**  Types and structures
*/

typedef enum shadeTag
   {
      SHADED_COLOR,                 /*  faces shaded, edges entity color  */
      SHADED_BLACK,                 /*  faces shaded, edges black  */
      TRANSPARENT_COLOR,            /*  faces clear, edges entity color  */
      COLOR_BLACK,                  /*  faces entity color, edges black  */
      NUM_SHADES
   }
   Shade;
   
typedef struct headerTag
   {
      Point origin;
      Point x_dir, y_dir;
      Point min, max;
      
      Shade shade;
      UWORD color;
   }
   Header;
   
   
   
/*
**  Function prototypes
*/

extern void load_header( void );


#endif   /*  _HEADER_H  */
