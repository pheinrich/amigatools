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
 *  Handles some nitty-gritty in-between steps in the extraction process.
 *  This module creates the output names, prompts the user for each file
 *  if necessary, and creates the Macintosh-format ".info" file if
 *  requested.
 *************************************************************************/



#ifdef __MSDOS__
 #include <conio.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "unstuff.h"
#include "checksum.h"
#include "expand.h"
#include "file.h"
#include "process.h"

#ifdef __MSDOS__
#pragma warn -pia
#endif



#ifdef SWAP
void swap_long( ulong *value )
/*
**  Converts a little-endian long value to a big-endian one (the format the
**  Macintosh uses).  This could be done faster, but this code is as clear
**  as can be.
*/
{
   ulong a, b, c, d;
   
   a = *value & 0x000000ff;
   b = *value & 0x0000ff00;
   c = *value & 0x00ff0000;
   d = *value & 0xff000000;

   *value = (a << 24) | (b << 8) | (c >> 8) | (d >> 24);
}



void swap_word( uword *value )
/*
**  Same as the above routine, but works on word values instead of longs.
**  Both routines swap one value at a time for simplicity (these routines
**  aren't called that much).
*/
{
   uword a, b;

   a = *value & 0x00ff;
   b = *value & 0xff00;

   *value = (a << 8) | (b >> 8);
}
#endif



static void filter_name( char *name )
/*
**  This routine checks for and removes invalid characters from the Mac
**  filename.  Each valid character on the platform running UnStuff is
**  represented by a '1' bit in the 'filter' array.
*/
{
   static char buffer[ FNAME_LEN ];
   char *src, *dst;
   const uword filter[ 16 ] =
      {
#ifdef __MSDOS__
         /*  These values look funny because of the PC's byte order  */
         0x0000, 0x0000, 0x20ba, 0x03ff, 0xffff, 0xc7ff, 0xffff, 0x6fff,
         0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x7fff
#else
 #ifdef AMIGA
         0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
         0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
 #else
  #error Need values for filter in filter_name()
 #endif
#endif
      };

   /*
   **  Copy the file name into 'buffer', character for character, skipping
   **  the bad ones.  The resulting name looks like the original with the
   **  illegal characters squeezed out.
   */
   for( src = name, dst = buffer; *src; src++ )
      if( VALID_CHAR( *src ) )
         *dst++ = *src;
   *dst = '\0';

   strcpy( name, buffer );
}



static bool get_command( void )
/*
**  Checks the status of the QUERY flag, prompting the user for input if
**  set.  This routine returns TRUE if the user makes a selection which
**  causes us to skip this file, else FALSE.
*/
{
   if( query() )
   {
      word response;

      printf( "R, D, B, S ?" );

#ifdef __MSDOS__
      while( kbhit() );
      while( NOT kbhit() );
      response = tolower( getch() );
#else
      fgets( (char *)&response, 2, stdin );
      response = tolower( response >> 8 );
#endif

      printf( BACK_TO_PROMPT "            " BACK_TO_PROMPT );

      switch( response )
      {
         case 'b':
            command |= RSRC | DATA;
            break;

         case 'd':
            command |= DATA;
            break;

         case 'r':
            command |= RSRC;
            break;

         case 's':
            /*  fall through  */

         default:
            puts( "Skipped." );
            return( TRUE );
      }
   }

   return( FALSE );
}



static char *get_name( FileHeader *fileHdr, enum Command fork )
/*
**  This routine creates an output name suitable to the platform running
**  UnStuff, taking into acount the fork we're currently working on.  If no
**  file should be generated, we'll return NULL.
*/
{
   static char buffer[ FNAME_LEN ];

   if( DATA == fork )
   {
      if( data() && fileHdr->dataLength )
      {
         if( raw() )
         {
            sprintf( buffer, RAW_DATA_NAME, fileHdr->fName );
            fileHdr->compDMethod = NONE;
         }
         else
            sprintf( buffer, DATA_NAME, fileHdr->fName );
      }
      else
         return( NULL );
   }
   else if( RSRC == fork )
   {
      if( resource() && fileHdr->rsrcLength )
      {
         if( raw() )
         {
            sprintf( buffer, RAW_RSRC_NAME, fileHdr->fName );
            fileHdr->compRMethod = NONE;
         }
         else
            sprintf( buffer, RSRC_NAME, fileHdr->fName );
      }
      else
         return( NULL );
   }
   else
   {
      if( info() )
         sprintf( buffer, INFO_NAME, fileHdr->fName );
      else
         return( NULL );
   }

   return( buffer );
}



static void make_info( char *name, FileHeader *fileHdr )
/*
**  This function creates a Macintosh ".info" file.  Although this file will
**  have the same extension as an icon (on the Amiga), it is NOT an Amiga
**  icon.
*/
{
   FILE *info;

   /*  Open the file  */
   if( info = fopen( name, "wb" ) )
   {
      InfoHeader infoHdr;

      /*
      **  Initialize the structure from the information in 'fileHdr'.
      **  All fields not explicitly initialized should be set to 0.
      */
      memset( (void *)&infoHdr, 0, sizeof( InfoHeader ) );
      strcpy( infoHdr.iName + 1, fileHdr->fName );
      *infoHdr.iName = strlen( fileHdr->fName );
      infoHdr.iType = fileHdr->fType;
      infoHdr.iAuthor = fileHdr->fCreator;
      infoHdr.FndrFlags = fileHdr->FndrFlags & ~INITED_FLAG;
      infoHdr.dataLength = fileHdr->dataLength;
      infoHdr.rsrcLength = fileHdr->rsrcLength;
      infoHdr.creationDate = fileHdr->creationDate;
      infoHdr.modDate = fileHdr->modDate;

#ifdef SWAP
      swap_long( (ulong *)&infoHdr.iType );
      swap_long( (ulong *)&infoHdr.iAuthor );
      swap_word( (uword *)&infoHdr.FndrFlags );
      swap_long( &infoHdr.dataLength );
      swap_long( &infoHdr.rsrcLength );
      swap_long( &infoHdr.creationDate );
      swap_long( &infoHdr.modDate );
#endif

      putc( 0, info );
      fwrite( (void *)&infoHdr, sizeof( InfoHeader ), 1, info );
      putc( 0, info );
      fclose( info );
   }
   else
      printf( "Can't open info file  " );
}



void process_file( FILE *in, FileHeader *fileHdr )
/*
**  This routine will process the member file described by the FileHeader
**  passed.  If an output file is generated, the destination name depends on
**  the platform running UnStuff.
*/
{
   char *name;
   bool rsrcOk = TRUE, dataOk = TRUE;

   /*  Prompt if necessary...  */
   if( get_command() )
   {
      move_to_next( in, fileHdr );
      return;
   }

   /*
   **  Filter the Macintosh file name for characters illegal on the
   **  platform running UnStuff, then use the new name to generate the
   **  resource and data forks, as well as the info file.  Note that the
   **  resource fork MUST be done first, since it's physically positioned
   **  ahead of the data fork in the archive file.
   */
   filter_name( fileHdr->fName );

   if( test() || (name = get_name( fileHdr, RSRC )) )
   {
      CheckSum crc;

      crc = expand( in, fileHdr, name, RSRC );
      if( NOT raw() && crc != fileHdr->rsrcCRC )
         rsrcOk = FALSE;
   }
   else
      fseek( in, fileHdr->compRLength, SEEK_CUR );

   if( test() || (name = get_name( fileHdr, DATA )) )
   {
      CheckSum crc;

      crc = expand( in, fileHdr, name, DATA );
      if( NOT raw() && crc != fileHdr->dataCRC )
         dataOk = FALSE;
   }
   else
      fseek( in, fileHdr->compDLength, SEEK_CUR );

   if( name = get_name( fileHdr, INFO ) )
      make_info( name, fileHdr );

   /*  Update our progress indicator  */
   if( dataOk && rsrcOk )
   {
      if( test() )
         puts( "Ok." );
      else
         puts( "Unstuffed." );
   }
   else if( test() )
      puts( "Bad Checksum!" );
}
