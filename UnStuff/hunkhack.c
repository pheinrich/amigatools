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
 *  The 2.0 Version command uses an identifier string embedded in an
 *  executeable to provide version information about the program.  Once
 *  imploded, however, the string isn't recognizable.  This program adds
 *  a HUNK_DEBUG hunk (containing the string required by Version) to a file
 *  AFTER it's been imploded.
 *
 *  Additionally, this utility provides a rudimentary form of encryption.
 *  The first code block produced by the imploder is altered slightly to
 *  prevent "de-ploding".  Two NOP instructions are tacked onto the end
 *  so that the resulting hunk is no longer recognized by the imploding
 *  utility.
 *
 *  This program was written specifically for Bio, but it will work on
 *  any program which doesn't open any resident libraries as it's loaded.
 *  Of course, the version string should probably be changed, in that
 *  case.  (Unless you want it to say 'Bio v?.? (1990,1992)'.)
 *
 *  Use 'lc -L hunkhack' to compile and link.
 *************************************************************************/



#include <stdio.h>
#include <stdlib.h>



/*
**  Size of new hunk in longwords
*/

#define  HACKLEN                    8



/*
**  Static function prototypes
*/

static long get_long( void );
static void put_hack( void );
static void put_long( long );
static void put_rest( void );



/*
**  Global static variables
*/

static FILE *in, *out;



static long get_long( void )
/*
**  Reads and returns a long value from the source file.
*/
{
   long l;

   fread( (void *)&l, sizeof( long ), 1, in );
   return( l );
}



static void put_long( long l )
/*
**  Outputs a long value to the destination file.
*/
{
   fwrite( (void *)&l, sizeof( long ), 1, out );
}



void main( int argc, char *argv[] )
{
   /*  Make sure we were called correctly  */
   if( argc != 3 )
   {
      printf( "Usage: %s <in> <out>\n", argv[0] );
      exit( 5 );
   }

   /*  Try to open the input file  */
   if( in = fopen( argv[1], "rb" ) )
   {
      /*  Try to open the output file  */
      if( out = fopen( argv[2], "wb" ) )
      {
         long val, i;

         /*  Output the HUNK_HEADER hunk found at the start of the file  */
         put_long( get_long() );                /*  HUNK_HEADER  */
         put_long( get_long() );                /*  # of res libs (0)  */
         put_long( val = get_long() );          /*  # of hunks in file  */
         put_long( get_long() );                /*  first slot  */
         put_long( get_long() );                /*  last slot  */

         /*  Now output the sizes of all the hunks, in order  */
         put_long( get_long() + 1 );            /*  new size of 1st hunk  */
         for( i = 0; i < val-1; i++ )
            put_long( get_long() );

         /*  Finally, output the rest of the file  */
         put_rest();

         fclose( out );
      }
      fclose( in );
   }
}



static void put_rest( void )
/*
**  Alters slightly the first hunk output by Imploder, then inserts our
**  'special' hunk, and finally copies the rest of the source file into the
**  destination file unchanged.
*/
{
   long i, val;
   int ch;

   put_long( get_long() );             /*  HUNK_CODE  */
   put_long( (val = get_long()) + 1 ); /*  new size  */

   for( i = 0; i < val; i++ )          /*  original data  */
      put_long( get_long() );
   put_long( 0x4e714e71 );             /*  two 'NOP' instructions  */

   put_hack();                         /*  stick this in slyly  */

   ch = fgetc( in );                   /*  everything else  */
   while( ch != EOF )
   {
      fputc( ch, out );
      ch = fgetc( in );
   }
}



static void put_hack( void )
/*
**  Outputs the long values making up the new hunk to the destination file.
*/
{
   long i, hack[HACKLEN+2] =
      {
         1009,                      /*  HUNK_DEBUG  */
         HACKLEN,                   /*  size in longwords  */
         0x00245645, 0x523a2055,    /*  '\0$VER: UnStuff v2.0 (1990,1992)\0'  */
         0x6e537475, 0x66662076,
         0x322e3020, 0x28313939,
         0x302c3139, 0x39322900
      };

   for( i = 0; i < HACKLEN + 2; i++ )
      put_long( hack[i] );
}
