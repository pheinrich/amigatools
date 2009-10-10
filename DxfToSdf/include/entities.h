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
 *  Individual entities stored in the .DXF file are parsed here.  Not all
 *  entities are handled here, since some may reside in the BLOCKS section.
 *************************************************************************/



#ifndef  _ENTITIES_H
#define  _ENTITIES_H


/*
**  Functions prototypes
*/

extern void load_entities( void );
extern void unload_entities( void );


#endif   /*  _ENTITIES_H  */
