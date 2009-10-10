 /*************************************************************************
 *
 *  UnStuff
 *  © 1990,1992  Peter Heinrich
 *
 *  unstuff.c
 *
 *  This program unstuffs file archives generated on the Macintosh with
 *  StuffIt.  Macintosh files are made up of a resource fork, a data fork, or
 *  both, and have another, auxilliary file associated with them having the
 *  suffix ".info".  UnStuff will extract any or all of these components,
 *  each to a file of its own.  UnStuff will also check the integrity of the
 *  archive without generating output files, if requested.
 *
 *  This is the main module of the program, parsing the command line and
 *  processing each file in the StuffIt archive.
 *
 * $Log$
 *************************************************************************/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unstuff.h"
#include "display.h"
#include "process.h"
#include "file.h"

#ifdef __MSDOS__
#pragma warn -pia
#endif



enum Command command = DATA | RSRC;
static char filename[ FNAME_LEN ];



static void usage( void )
/*
**  Displays an explanatory message if help is requested or if an unrecog-
**  nized option is present on the command-line.
*/
{
   puts( "Usage:  UnStuff [-<d|q|r|t|v>] [-<bhiw>] <file>\n"
         "Where\n"
         "      -b   use brief display mode\n"
         "      -d   extract data fork(s) only\n"
         "      -h   display this message\n"
         "      -i   create Macintosh \".info\" file\n"
         "      -q   prompt for each file\n"
         "      -r   extract resource fork(s) only\n"
         "      -t   test archive integrity\n"
         "      -v   view archive contents\n"
         "      -w   extract without decompression\n" );
   exit( 5 );
}



static void parse( uword argc, char *argv[] )
/*
**  This routine parses the command-line arguments, setting or clearing flags
**  as appropriate.
*/
{
   uword arg;
   char *option;

   /*
   **  Run through each argument on the command line.  An argument will
   **  read as a switch if it's preceeded by a '-' or '/', otherwise it's
   **  assumed to be the name of the file.  Options (and file names) can
   **  appear in any order on the command line; later ones simply super-
   **  cede earlier ones.
   */
   for( arg = 1; arg < argc; arg++ )
   {
      option = argv[ arg ];
      if( '-' == *option || '/' == *option )
      {
         /*
         **  Some switches are mutually exclusive, so they may clear
         **  certain flags while setting others.
         */
         switch( tolower( *(option + 1) ) )
         {
            case 'b':
               command |= BRIEF;
               break;

            case 'd':
               command |= DATA;
               command &= ~(QUERY | RSRC | TEST | VIEW);
               break;

            case 'h':
               command = HELP;
               break;

            case 'i':
               command |= INFO;
               break;

            case 'q':
               command |= QUERY;
               command &= ~(DATA | RSRC | TEST | VIEW);
               break;

            case 'r':
               command |= RSRC;
               command &= ~(DATA | QUERY | TEST | VIEW);
               break;

            case 't':
               command |= TEST;
               command &= ~(DATA | QUERY | RSRC | VIEW);
               break;

            case 'v':
               command |= VIEW;
               command &= ~(DATA | QUERY | RSRC | TEST);
               break;

            case 'w':
               command |= RAW;
               break;

            default:
               command = HELP;
               break;
         }
      }
      else if( '?' == *option )
         command = HELP;
      else
         strcpy( filename, argv[ arg ] );
   }

   /*
   **  If the user forgot a filename, remind them.  If they didn't, make
   **  sure it has the ".SIT" extension.
   */
   if( NOT *filename )
      command = HELP;
   else if( NOT strchr( filename, '.' ) )
      strcat( filename, ".sit" );

   /*  Yours truly...  */
   puts( "UnStuff v" US_VERSION " (c) " US_DATE "  Peter Heinrich" );
   if( command == HELP )
      usage();
}



void main( uword argc, char *argv[] )
/*
**  Entry point of the program.  If this wasn't immediately clear, perhaps
**  you haven't quite reached the I-can-read-C-code stage yet.  There are
**  lots of good books on the subject, though, so I'm sure you'll be there
**  in no time.
*/
{
   FILE *in;

   /*  Find out what we're supposed to do  */
   parse( argc, argv );

   /*  Open the archive  */
   if( in = fopen( filename, "rb" ) )
   {
      SitHeader *sitHdr;

      /*
      **  Read the header structure identifying this file as a StuffIt
      **  archive.  If the header doesn't check out, bail.
      */
      printf( "Examining %s:\n", filename );
      if( sitHdr = get_sit_header( in ) )
      {
         uword i;

         /*
         **  For each file in the archive, we'll read the header inform-
         **  ation, print some of what we find, and either decompress it
         **  or move to the next one.
         */
         print_header();
         for( i = 0; i < sitHdr->numFiles; i++ )
         {
            FileHeader *fileHdr;

            if( fileHdr = get_file_header( in ) )
            {
               printf( "%3hu. ", i + 1 );
               print_file_info( fileHdr );

               if( view() )
                  move_to_next( in, fileHdr );
               else
                  process_file( in, fileHdr );

               free_file_header( fileHdr );
            }
         }

         print_total_info();
         printf( "%hu File%s total.\n", i, i == 1 ? "" : "s" );

         free_sit_header( sitHdr );
      }
      fclose( in );
   }
   else
      clean_exit( "Can't find that archive file" );
}



void clean_exit( char *text )
/*
**  Provides a quick exit from the program.  This routine is called whenever
**  a fatal error is encountered.  The message is printed to the screen and
**  then we're out of here.
*/
{
   printf( "\n%s!\n", text );
   exit( 20 );
}
