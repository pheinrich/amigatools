 /*************************************************************************
 *
 *  crlf
 *  Copyright (c) 1991,2009  Peter Heinrich
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
 *  This program converts between Amiga, Macintosh, and IBM format text
 *  files, at least as far as end-of-line markers go.  When converting to
 *  IBM format, a CTRL-'Z' is added to the end of the file.  This same
 *  CTRL-'Z' is stripped out if converting to the other formats.  Nothing
 *  special is done with 8-bit characters; upper ASCII is passed un-
 *  altered, so graphics and foreign characters may need to be changed by
 *  hand.  This short module handles output to the screen.  Various bits
 *  of info are deposited there, such as archived file names, sizes, com-
 *  pression ratios, etc.  Error messages, when they're necessary (God
 *  forbid) are handled here, also.
 *************************************************************************/



#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define  NO          (2 + 2 == 5)
#define  YES         (2 + 2 == 4)

#define  AMIGA       1
#define  HELP        2
#define  IBM         3
#define  MAC         4
#define  MAXLEN      80

#define  C_RETURN    '\x0d'
#define  CTRL_Z      '\x1a'
#define  LINEFEED    '\x0a'



void add_entry(char *);
short check_entry(char *);
void convert(char *);
void do_files(char *);
void filter(FILE *, FILE *);
void free_namelist(void);
void main(short, char *[]);



short backup = NO, mode = HELP;
typedef struct nameNode
   {
      struct nameNode *next;
      char name[MAXLEN];
   } NAMENODE;
NAMENODE *nameList = NULL;



void main(short argc, char *argv[])
/*
**    Entry point to the program.  This should be fairly clear.  In fact,
**    if this WASN'T immediately apparent, perhaps you shouldn't even be
**    reading this; you may not have reached the level of programming
**    proficiency necessary to competently evaluate the purpose and/or the
**    exceptionally complicated operation of this piece of software
**    engineering.
*/
{
   short i;
   char fspec[MAXLEN];
   
   printf("CrLf v0.52  © 1991, Peter Heinrich\n");
   for (fspec[0] = '\0', i = 1; i < argc; i++)
   {
      if (argv[i][0] == '-')
      {
         switch (tolower(argv[i][1]))
         {
            case 'a':
               mode = AMIGA;
               break;

            case 'b':
               backup = YES;
               break;

            case 'i':
               mode = IBM;
               break;

            case 'm':
               mode = MAC;
               break;

            default:
               mode = HELP;
               break;
         }
      }
      else
         strncpy(fspec, argv[i], MAXLEN);
   }

   if (mode == HELP || !fspec[0])
   {
      printf("Usage: CrLf [-<a|i|m>] [-b] <filespec>\n"
             "Where,\n"
             "   -a   convert to Amiga format\n"
             "   -i   convert to IBM format\n"
             "   -m   convert to Macintosh format\n\n"
             "   -b   create backup copies of original files\n\n"
             "   filespec is a valid DOS name\n"
             "       (including wildcards)\n\n"
             );
      exit(5);
   }

   do_files(fspec);
}



void do_files(char *fspec)
/*
**    This routine finds all files in the current directory matching the
**    filespec passed, then converts each.
*/
{
   struct FILEINFO finfo;
   short done = NO;
   
   done = dfind(&finfo, fspec, 0);
   while (! done)
   {
      convert(finfo.fib_FileName);
      done = dnext(&finfo);
   }
   
   free_namelist();
}



void convert(char *fname)
/*
**    Converts a single file by filtering it and rewriting it to disk.  If
**    backups are requested, this function will handle them.
*/
{
   FILE *in = NULL, *out = NULL;
   short success = NO;
   char tempName[MAXLEN];
   
   
   if (check_entry(fname))
      return;
      
   strncpy(tempName, fname, 25);
   strcat(tempName, ".bak");
      
   if (! rename(fname, tempName))
   {
      if (in = fopen(tempName, "rb"))
      {
         if (out = fopen(fname, "wb"))
         {
            filter(in, out);
            fclose(out);
            printf("Converted file \"%s\"\n", fname);

            success = YES;
         }
         else
            printf("Couldn't open file \"%s\" for output!\n", fname);

         fclose(in);
      }
      else
         printf("Couldn't open file \"%s\" for input!\n", tempName);
      
      if (success)
      {
         if (! backup)
            remove(tempName);
         else
            add_entry(tempName);

         add_entry(fname);
      }
      else
      {
         remove(fname);
         rename(tempName, fname);
      }
   }
   else
      printf("Couldn't create temporary file \"%s\"!\n", tempName);
}



void filter(FILE *in, FILE *out)
/*
**    Filters the datastream for EOL markers not conforming to the desired
**    format and outputs the correct substitute.  For conversions involving
**    IBM-format files, a CTRL-'Z' is either appended to the end of the
**    file (to IBM), or stripped off (to Amiga or Mac).
*/
{
   int ch, last = 0;
   
   ch = getc(in);
   while (! feof(in))
   {
      switch(ch)
      {
         case LINEFEED:
         case C_RETURN:
            if ((last != LINEFEED && last != C_RETURN) || ch == last)
            {
               switch (mode)
               {
                  case AMIGA:
                     putc(LINEFEED, out);
                     break;

                  case IBM:
                     putc(C_RETURN, out);
                     putc(LINEFEED, out);
                     break;

                  case MAC:
                     putc(C_RETURN, out);
                     break;

                  default:
                     break;
               }
            }
            else
               ch = last;
            break;

         case CTRL_Z:
            break;

         default:
            putc(ch, out);
            break;
      }

      last = ch;
      ch = getc(in);
   }
   
   if (mode == IBM)
      putc(CTRL_Z, out);
}



void add_entry(char *name)
/*
**    Adds a name to the "processed" list, making it possible to avoid
**    converting files twice, as well as preventing us from converting
**    files created by our own program, such as possible backup files.
**    Nodes are added to a simple LIFO list.
*/
{
   NAMENODE *nameNode;
   
   if (nameNode = (NAMENODE *) calloc(1, sizeof(NAMENODE)))
   {
      strcpy(nameNode->name, name);
      nameNode->next = nameList;
      nameList = nameNode;
   }
}



short check_entry(char *name)
/*
**    Scans a list of names, returning YES if the one passed is present.
**    NO is returned otherwise.  The list is unsorted, so the search is
**    O(n), but with luck, we won't be processing gargantuan lists of
**    files, making it fairly painless.
*/
{
   NAMENODE *node;
   
   node = nameList;
   while (node)
   {
      if (! strcmp(node->name, name))
         return YES;
      node = node->next;
   }
   
   return NO;
}



void free_namelist(void)
/*
**    Frees memory allocated for each name on the "processed" list.  This
**    would be done when we exited, but we're good programmers here, so we
**    do it explicitly like we're supposed to.
*/
{
   NAMENODE *node;
   
   while (nameList)
   {
      node = nameList->next;
      free(nameList);
      nameList = node;
   }
}

