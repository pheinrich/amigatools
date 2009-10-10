 /**********************************************************
 *
 *    hunk.c
 *    © 9/3/91  Peter Heinrich
 *
 *    This program identifies the hunk structure of any
 *    AmigaDOS executable or object file (but not libraries).
 *
 * $Log:	hunk.c,v $
 * Revision 0.52  91/10/01  22:44:45  Peter
 * Corrected two small typos in output text.
 * 
 * Revision 0.5  91/09/03  10:47:50  Peter
 * Created
 * 
 **********************************************************/
 
 

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include "hunk.h"



static BYTEBITS show = HEXADECIMAL;
static FILE *in = NULL;



void main(UWORD argc, char *argv[])
/*
**    Entry point of program.  The name of the file under scrutiny should
**    be passed on the command line.  There's no support for the WorkBench
**    as of yet.
*/
{
   ULONG first, last, length, lword, type;
   UBYTE i, j;
   BOOL help = NO;
   char fname[MAXLEN], *str;
   
   fname[0] = '\0';
   for (i = 1; i < argc; i++)
   {
      if (argv[i][0] == '-')
      {
         switch (tolower(argv[i][1]))
         {
            case 'a':
               for (j = 0; ansi[j]; j++)
                  ansi[j] = NULL;
               break;

            case 'd':
               show &= ~HEXADECIMAL;
               break;

            case 'h':
               help = YES;
               break;

            case 's':
               for (j = 2; argv[i][j]; j++)
               {
                  switch (argv[i][j])
                  {
                     case 'a':
                        show |= DETAILS | NUMBERS | LENGTHS | NAMES |
                                POSITIONS | RES_LIBS | SYMBOLS;
                        break;

                     case 'd':
                        show |= DETAILS;
                        break;

                     case 'h':
                        show |= NUMBERS;
                        break;

                     case 'l':
                        show |= LENGTHS;
                        break;

                     case 'n':
                        show |= NAMES;
                        break;

                     case 'p':
                        show |= POSITIONS;
                        break;

                     case 'r':
                        show |= RES_LIBS;
                        break;

                     case 's':
                        show |= SYMBOLS;
                        break;

                     default:
                        print_name();
                        printf("Unrecognized option %.*s%s%c%s%s\n",
                           j, argv[i], ansi[A_ER_S], argv[i][j++],
                           ansi[A_E], &argv[i][j]);
                        exit(10);
                  }
               }
               break;

            default:
               print_name();
               printf("Unrecognized option -%s%s%s\n",
                  ansi[A_ER_S], &argv[i][1], ansi[A_E]);
               exit(10);
         }
      }
      else if (argv[i][0] == '?')
         help = YES;
      else
         strncpy(fname, argv[i], MAXLEN);
   }
   print_name();

   if (i == 1 || help == YES)
   {
      printf("Usage: hunk [-a] [-d] [-h] [-s<adhlnprs>] <filename>\n"
             "Where:\n"
             "      -a   turn off ANSI sequences\n"
             "      -d   decimal numbers\n"
             "      -h   this help screen\n"
             "      -sa  show all options (verbose)\n"
             "      -sd  show hunk details\n"
             "      -sh  show hunk numbers\n"
             "      -sl  show hunk lengths\n"
             "      -sn  show names\n"
             "      -sp  show hunk file positions\n"
             "      -sr  show resident libraries\n"
             "      -ss  show symbols\n\n");
      exit(5);
   }
   if (! strlen(fname))
   {
      printf("No filename specified!\n");
      exit(10);
   }
   
   if (in = fopen(fname, "rb"))
   {
      fseek(in, 0, SEEK_END);
      length = ftell(in);
      rewind(in);

      type = get_lword();
      while (!feof(in))
      {
         lword = get_lword();
         print_num(type);
         
         switch (type)
         {
            case HUNK_UNIT:
               print_hunk("Hunk_Unit");
               str = get_string(lword);
               if (show & NAMES)
                  printf(": %s%s%s", ansi[A_ST_S], str, ansi[A_E]);
               break;

            case HUNK_NAME:
               print_hunk("Hunk_Name");
               str = get_string(lword);
               if (show & NAMES)
                  printf(": %s%s%s", ansi[A_ST_S], str, ansi[A_E]);
               break;

            case HUNK_CODE:
               print_hunk("Hunk_Code");
               if (show & LENGTHS)
               {
                  printf(": ");
                  print_lword(lword);
                  print_many("longword", lword);
               }
               fseek(in, lword*sizeof(long), SEEK_CUR);
               break;

            case HUNK_DATA:
               print_hunk("Hunk_Data");
               if (show & LENGTHS)
               {
                  printf(": ");
                  print_lword(lword);
                  print_many("longword", lword);
               }
               fseek(in, lword*sizeof(long), SEEK_CUR);
               break;

            case HUNK_BSS:
               print_hunk("Hunk_BSS");
               if (show & LENGTHS)
               {
                  printf(": ");
                  print_lword(lword & 0x3fffffff);
                  print_many("longword", lword);
                  printf(" requested");
               }
               lword >>= 30;
               if (lword == 3)
                  lword = get_lword();
               if (show & DETAILS)
               {
                  if (lword == 1)
                     printf("\n   (in %sCHIP%s memory)",
                        ansi[A_MY_S], ansi[A_E]);
                  else if (lword == 2)
                     printf("\n   (in %sFAST%s memory)",
                        ansi[A_MY_S], ansi[A_E]);
                  else if (lword)
                  {
                     printf("\n   (of type %s", ansi[A_MY_S]);
                     print_lword(lword & 0x00ffffff);
                     printf("%s)", ansi[A_E]);
                  }
               }
               break;

            case HUNK_RELOC32:
               print_hunk("   Hunk_Reloc32");
               read_reloc(lword, sizeof(long));
               break;

            case HUNK_RELOC16:
               print_hunk("   Hunk_Reloc16");
               read_reloc(lword, sizeof(long));
               break;

            case HUNK_RELOC8:
               print_hunk("   Hunk_Reloc8");
               read_reloc(lword, sizeof(long));
               break;

            case HUNK_EXT:
               print_hunk("   Hunk_Ext");
               read_SDUs(lword);
               break;

            case HUNK_SYMBOL:
               print_hunk("   Hunk_Symbol");
               read_SDUs(lword);
               break;

            case HUNK_DEBUG:
               print_hunk("   Hunk_Debug");
               if (show & LENGTHS)
               {
                  printf(": ");
                  print_lword(lword);
                  print_many("longword", lword);
               }
               fseek(in, lword*sizeof(long), SEEK_CUR);
               break;

            case HUNK_END:
               print_hunk("Hunk_End");
               if (ftell(in) != length)
                  fseek(in, -sizeof(long), SEEK_CUR);
               break;

            case HUNK_HEADER:
               print_hunk("Hunk_Header");
               while (lword)
               {
                  str = get_string(lword);
                  if (show & RES_LIBS)
                     printf("\n   Resident library: %s", str);
                  lword = get_lword();
               }
               
               lword = get_lword();
               if (show & DETAILS)
               {
                  printf("\n   Number of hunks: ");
                  print_lword(lword);
               }

               first = get_lword();
               if (show & DETAILS)
               {
                  printf("\n   First slot: ");
                  print_lword(first);
               }
                  
               last = get_lword();
               if (show & DETAILS)
               {
                  printf("\n   Last slot: ");
                  print_lword(last);
               }
               fseek(in, (last-first+1)*sizeof(long), SEEK_CUR);
               break;

            case HUNK_OVERLAY:
               print_hunk("Hunk_Overlay");
               if (show & LENGTHS)
               {
                  printf(": ");
                  print_lword(lword);
                  print_many("longword", lword);
               }
                  
               lword = get_lword() + 1;
               fseek(in, lword*sizeof(long), SEEK_CUR);
               fseek(in, (8+lword)*sizeof(long), SEEK_CUR);
               break;

            case HUNK_BREAK:
               print_hunk("Hunk_Break");
               fseek(in, -sizeof(long), SEEK_CUR);
               break;

            case HUNK_DRELOC32:
               print_hunk("   Hunk_DReloc32");
               read_reloc(lword, sizeof(long));
               break;

            case HUNK_DRELOC16:
               print_hunk("   Hunk_DReloc16");
               read_reloc(lword, sizeof(long));
               break;

            case HUNK_DRELOC8:
               print_hunk("   Hunk_DReloc8");
               read_reloc(lword, sizeof(long));
               break;

            case HUNK_RELOC32SHORT:
               print_hunk("   Hunk_Reloc32Short");
               read_reloc(lword, sizeof(WORD));
               break;

            default:
               lword = ftell(in);
               print_hunk("\nUnrecognized hunk type!");
               if (show & DETAILS)
               {
                  printf("\n   Type: ");
                  if (show & HEXADECIMAL)
                     printf("0x%lx", type);
                  else
                     printf("%lu", type);
               }
               if (show & POSITIONS)
               {
                  printf("\n   File position: ");
                  if (show & HEXADECIMAL)
                     printf("0x%lx", lword);
                  else
                     printf("%lu", lword);
               }
               fclose(in);
               printf("\n");
               exit(20);
               break;
         }
         printf("\n%s", ansi[A_E]);
         fread((void *) &type, sizeof(ULONG), 1, in);      
      }
      fclose(in);
   }
   else
      printf("Can't open file %s%s%s for input!\n",
         ansi[A_ER_S], fname, ansi[A_E]);
}
            


void print_num(ULONG type)
/*
**    Prints the number of a hunk if necessary.
*/
{
   static ULONG hunkNum = 0;
   
   switch (type)
   {
      case HUNK_CODE:
      case HUNK_DATA:
      case HUNK_BSS:
         if (show & NUMBERS)
         {
            if (show & HEXADECIMAL)
               printf("%s0x%0.4lx%s ", ansi[A_NM_S], hunkNum++, ansi[A_E]);
            else
               printf("%s%0.6lu%s ", ansi[A_NM_S], hunkNum++, ansi[A_E]);
         }
         break;

      default:
         break;
   }
}



void print_hunk(char *name)
/*
**    Prints a hunk name (or descriptive string, if necessary).
*/
{
   printf("%s%s%s", ansi[A_HK_S], name, ansi[A_E]);
   if (show & POSITIONS)
   {
      printf(" (");
      print_lword(ftell(in) - 2*sizeof(ULONG));
      printf(")");
   }
}



void print_lword(ULONG lword)
/*
**    Prints a number in decimal or hexadecimal, depending on the current
**    user preferences.
*/
{
   if (show & HEXADECIMAL)
      printf("0x%lx", lword);
   else
      printf("%lu", lword);
}
      
        

void print_name(void)
/*
**    Does nothing but print the name of the program and return.
*/
{
   printf("%sHunk v0.52, © 1991  Peter Heinrich%s\n",
      ansi[A_BG_S], ansi[A_E]);
}



void print_many(char *string, ULONG num)
/*
**    Prints the character string passed, appending 's' to pluralize if
**    num != 1.
*/
{
   printf(" %s%s", string, (num == 1) ? "" : "s");
}



char *get_string(ULONG num)
/*
**    Reads num longwords from the input stream and returns the NULL-
**    terminated string they comprise.
*/
{
   static char buffer[MAXLEN];
   
   num *= sizeof(long);
   memset(buffer, '\0', MAXLEN);
   fread(buffer, sizeof(char), num, in);
   
   return buffer;
}



ULONG get_lword(void)
/*
**    Reads one longword from the input stream.
*/
{
   ULONG temp;
   
   fread((void *) &temp, sizeof(ULONG), 1, in);
   return temp;
}



void read_reloc(ULONG lword, UWORD size)
/*
**    Scans a list of relocation blocks until the 0 terminator is found.
*/
{
   ULONG hunkNum;
   
   while (lword)
   {
      hunkNum = 0;
      fread((void *) &hunkNum, size, 1, in);
      if (show & DETAILS)
      {
         printf("\n      Hunk ");

         if (show & HEXADECIMAL)
            printf("0x%0.4lx: 0x%lx", hunkNum, lword);
         else
            printf("#%0.6lu: %lu", hunkNum, lword);

         print_many("offset", lword);
      }
      fseek(in, lword*size, SEEK_CUR);
      lword = get_lword();
   }
}



void print_symbol(char *block, char *symbol, ULONG value)
/*
**    Prints a symbol data unit which isn't a reference or common.
*/
{
   if (show & SYMBOLS)
   {
      printf("\n      %s: %s = ", block, symbol);
         
      if (show & HEXADECIMAL)
         printf("0x%lx", value);
      else
         printf("%lu", value);
   }
}



void print_ref(char *block, char *ref, ULONG num)
/*
**  Prints a symbol data unit which is a reference.
*/
{
   if (show & SYMBOLS)
   {
      printf("\n      %s: ", block);

      if (show & HEXADECIMAL)
         printf("0x%lx", num);
      else
         printf("%lu", num);

      print_many("reference", num);
      printf(" to %s", ref);
   }
   fseek(in, num*sizeof(long), SEEK_CUR);
}
   


void read_SDUs(ULONG lword)
/*
**    Reads a list of symbol data units from the input stream until the 0
**    terminator is encountered.
*/
{
   ULONG size, type;
   char *str;
   
   do
   {
      type = lword >> 24;
      lword &= 0x00ffffff;

      str = get_string(lword);      
      lword = get_lword();
      
      switch (type)
      {
         case EXT_SYMB:
            print_symbol("Ext_Symb", str, lword);
            break;

         case EXT_DEF:
            print_symbol("Ext_Def", str, lword);
            break;

         case EXT_ABS:
            print_symbol("Ext_Abs", str, lword);
            break;

         case EXT_RES:
            print_symbol("Ext_Res", str, lword);
            break;

         case EXT_REF32:
            print_ref("Ext_Ref32", str, lword);
            break;

         case EXT_COMMON:
            if (show & DETAILS)
            {
               printf("\n      Ext_Common");
               if (show & NAMES)
                  printf(": %s%s%s", ansi[A_ST_S], str, ansi[A_E]);
            }
               
            size = lword;
            lword = get_lword();

            if (show & DETAILS)
            {
               printf("\n         Size = ");
               if (show & HEXADECIMAL)
                  printf("0x%lx", size);
               else
                  printf("%lu", size);
               
               printf("\n         References: ");
               if (show & HEXADECIMAL)
                  printf("0x%lx", lword);
               else
                  printf("%lu", lword);
            }
            fseek(in, lword*sizeof(long), SEEK_CUR);
            break;

         case EXT_REF16:
            print_ref("Ext_Ref16", str, lword);
            break;

         case EXT_REF8:
            print_ref("Ext_Ref8", str, lword);
            break;

         case EXT_DREF32:
            print_ref("Ext_DRef32", str, lword);
            break;

         case EXT_DREF16:
            print_ref("Ext_DRef16", str, lword);
            break;

         case EXT_DREF8:
            print_ref("Ext_DRef8", str, lword);
            break;

         default:
            lword = ftell(in);
            printf("\n      Unrecognized symbold data unit type!");
            if (show & DETAILS)
            {
               printf("\n         Type: ");
               if (show & HEXADECIMAL)
                  printf("0x%lx", type);
               else
                  printf("%lu", type);
            }
            if (show & POSITIONS)
            {
               printf("\n         File position: ");
               if (show & HEXADECIMAL)
                  printf("0x%lx", lword);
               else
                  printf("%lu", lword);
            }
            fclose(in);
            printf("\n");
            exit(20);
            break;
      }
   }
   while (lword = get_lword());
}
