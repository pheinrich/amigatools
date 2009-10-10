 /*************************************************************************
 *
 *  UnStuff
 *  Copyright (c) 1990,1992,2009  Peter Heinrich
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
 *  This module of the program handles all the file input/output besides
 *  the decompression stuff--see expand.c.
 *************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unstuff.h"
#include "checksum.h"
#include "display.h"
#include "file.h"
#include "process.h"

#ifdef __MSDOS__
#pragma warn -pia
#endif



void free_file_header( FileHeader *fileHdr )
/*
**  Companion function for get_file_header().  This routine should be called
**  with a pointer to a FileHeader previously returned by get_file_header().
*/
{
   if( fileHdr )
      free( fileHdr );
}



void free_sit_header( SitHeader *sitHdr )
/*
**  Similar to the above function.  This routine is meant to cleanup after
**  a SitHeader is no longer needed.
*/
{
   if( sitHdr )
      free( sitHdr );
}



FileHeader *get_file_header( FILE *in )
/*
**  Creates a FileHeader structure and fills it with data beginning at the
**  current file location in 'in'.  Obviously, it's best if you've already
**  advanced to where a FileHeader is stored in the file.  If an error
**  occurs, NULL is returned.
*/
{
   FileHeader *fileHdr;

   if( fileHdr = (FileHeader *)malloc( FileHdrSize ) )
   {
      if( FileHdrSize == fread( (void *)fileHdr, sizeof( ubyte ),
         FileHdrSize, in ) || ask( "Read error on file header" ) )
      {
         CheckSum crc;

         crc = calc_crc( (ubyte *)fileHdr,
            FileHdrSize - sizeof( CheckSum ), INIT_CRC );
#ifdef SWAP
         /*  We MUST calculate the checksum BEFORE we swab the data!  */
         swap_long( (ulong *)&fileHdr->fType );
         swap_long( (ulong *)&fileHdr->fCreator );
         swap_word( (uword *)&fileHdr->FndrFlags );
         swap_long( &fileHdr->creationDate );
         swap_long( &fileHdr->modDate );
         swap_long( &fileHdr->rsrcLength );
         swap_long( &fileHdr->dataLength );
         swap_long( &fileHdr->compRLength );
         swap_long( &fileHdr->compDLength );
         swap_word( &fileHdr->rsrcCRC );
         swap_word( &fileHdr->dataCRC );
         swap_word( &fileHdr->hdrCRC );
#endif
         if( crc == fileHdr->hdrCRC || 
            ask( "Bad checksum on file header" ) )
         {
            ubyte len = (ubyte)*(fileHdr->fName);

            /*  Convert the Pascal-style name string to C-style  */
            memmove( fileHdr->fName, fileHdr->fName + 1, len );
            *(fileHdr->fName + len) = '\0';

            return( fileHdr );
         }
      }
      free( fileHdr );
   }
   else
      clean_exit( "Out of memory!" );

   return( NULL );
}



SitHeader *get_sit_header( FILE *in )
/*
**  This function creates a SitHeader structure and initializes it with the
**  data stored at the beginning of the file 'in'.  If an error occurs, this
**  routine returns NULL.
*/
{
   SitHeader *sitHdr;

   fseek( in, InfoSize, SEEK_SET );
   if( sitHdr = (SitHeader *)malloc( SitHdrSize ) )
   {
      if( SitHdrSize == fread( (void *)sitHdr, sizeof( byte ),
         SitHdrSize, in ) || ask( "Read error on SIT header" ) )
      {
#ifdef SWAP
         swap_long( (ulong *)&sitHdr->signature );
         swap_word( &sitHdr->numFiles );
         swap_long( &sitHdr->arcLength );
         swap_long( (ulong *)&sitHdr->signature2 );
#endif
         /*
         **  Verify the signature and version of this file's SitHeader.
         **  If not version 1, we may not be able to decompress this
         **  archive.
         */
         if( sitHdr->signature == 0x53495421 &&
            sitHdr->signature2 == 0x724c6175 )
         {
            if( sitHdr->version <= SIT_VERSION ||
               ask( "This archive wasn't created with StuffIt version 1" ) )
                  return( sitHdr );
         }
         else
            clean_exit( "This file is not a StuffIt archive" );
      }
      free( sitHdr );
   }
   else
      clean_exit( "Out of memory!" );

   return( NULL );
}



void move_to_next( FILE *in, FileHeader *fileHdr )
/*
**  This routine uses the information in 'fileHdr' to advance past it and
**  the data it introduces.  This effectively positions the file pointer of
**  'in' at the beginning of the next FileHeader structure in the file.
*/
{
   fseek( in, fileHdr->compRLength + fileHdr->compDLength, SEEK_CUR );
}
