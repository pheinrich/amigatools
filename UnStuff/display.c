 /*************************************************************************
 *
 *  UnStuff
 *  © 1990,1992  Peter Heinrich
 *
 *  display.c
 *
 *  This short module handles output to the screen.  Various bits of info
 *  are deposited there, such as archived file names, sizes, compression
 *  ratios, etc.  Error messages, when they're necessary (God forbid) are
 *  handled here, also.
 *
 * $Log$
 *************************************************************************/



#ifdef __MSDOS__
 #include <conio.h>
#endif
#include <ctype.h>
#include <stdio.h>

#include "unstuff.h"
#include "display.h"



static ulong totRsrc, totData, totSit;



bool ask( char *text )
/*
**  Prints the informative error message, then asks whether the user would
**  like to continue anyway.  Returns TRUE if they choose to continue.
*/
{
   word response;

   printf( "\n%s...\nContinue anyway? (Y/n)  ", text );
#ifdef __MSDOS__
   while( kbhit() );
   while( NOT kbhit() );
   response = tolower( getch() );
#else
   fgets( (char *)&response, 2, stdin );
   response = tolower( response >> 8 );
#endif

   if( 'n' == response )
   {
      puts( "NO" );
      return( FALSE );
   }
   else
   {
      puts( "YES" );
      return( TRUE );
   }
}



void print_file_info( const FileHeader *fileHdr )
/*
**  This function prints the resource and data fork sizes, as well as their
**  total stuffed length and compression ratio.  It also keeps a running
**  total of these values for use by print_total_info().
*/
{
   if( NOT brief() )
   {
      ulong r, d, comp;

      r = fileHdr->rsrcLength;
      d = fileHdr->dataLength;
      comp = fileHdr->compRLength + fileHdr->compDLength;

      /*  Keep track for later...  */
      totRsrc += r;
      totData += d;
      totSit += comp;

      printf( "%8lu  %8lu  %8lu  %3ld%%  ",
         r, d, comp, 100 - 100*comp/(r + d + 1) );
   }

   printf( "%s", fileHdr->fName );
   if( view() )
      puts( "" );
   else
      printf( "...  " );
}



void print_header( void )
/*
**  Printed just before we actually start looking at the archive in question.
*/
{
   if( query() )
      puts( "For each of the following files, please specify\n"
            "<R>esource, <D>ata, <B>oth, or <S>kip." );

   if( NOT brief() )
   {
      puts( "     Resource    Data    Stuffed!  Gain\n"
            "     --------  --------  --------  ----" );

      totRsrc = totData = totSit = 0L;
   }
}



void print_total_info( void )
/*
**  Printed after we've done all we're going to do.
*/
{
   if( NOT brief() )
      printf( "     --------  --------  --------  ----\n"
              "     %8lu  %8lu  %8lu  %3ld%%  ",
              totRsrc, totData, totSit,
              100 - 100*totSit/(totRsrc + totData + 1) );
}
