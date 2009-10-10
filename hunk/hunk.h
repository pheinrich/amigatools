 /*************************************************************************
 *
 *  hunk
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
 *  This header file contains information useful to hunk.c, such as
 *  function prototypes, macros, magic numbers, etc.
 *************************************************************************/
 
 

/*
**  Magic numbers
*/

#define NO                 (2 + 2 == 5)
#define YES                (2 + 2 == 4)

#define MAXLEN             81

#define DETAILS            1
#define NUMBERS            2
#define LENGTHS            4
#define NAMES              8
#define POSITIONS          16
#define RES_LIBS           32
#define SYMBOLS            64
#define HEXADECIMAL        128

#define HUNK_BREAK         1014
#define HUNK_BSS           1003
#define HUNK_CODE          1001
#define HUNK_DATA          1002
#define HUNK_DEBUG         1009
#define HUNK_DRELOC16      1016
#define HUNK_DRELOC32      1015
#define HUNK_DRELOC8       1017
#define HUNK_END           1010
#define HUNK_EXT           1007
#define HUNK_HEADER        1011
#define HUNK_NAME          1000
#define HUNK_OVERLAY       1013
#define HUNK_RELOC16       1005
#define HUNK_RELOC32       1004
#define HUNK_RELOC32SHORT  1020
#define HUNK_RELOC8        1006
#define HUNK_SYMBOL        1008
#define HUNK_UNIT          999

#define EXT_ABS            2
#define EXT_COMMON         130
#define EXT_DEF            1
#define EXT_DREF16         134
#define EXT_DREF32         133
#define EXT_DREF8          135
#define EXT_RES            3
#define EXT_REF16          131
#define EXT_REF32          129
#define EXT_REF8           132
#define EXT_SYMB           0

#define A_BG_S             0
#define A_ER_S             1
#define A_HK_S             2
#define A_MY_S             3
#define A_NM_S             4
#define A_ST_S             5
#define A_E                6



/*
**  Function prototypes
*/

ULONG get_lword(void);
char *get_string(ULONG);
void print_hunk(char *);
void print_lword(ULONG);
void print_many(char *, ULONG);
void print_name(void);
void print_num(ULONG);
void read_reloc(ULONG, UWORD);
void read_SDUs(ULONG);



/*
**  Ansi escape strings
*/

char *ansi[] =
{
   "\x1b[32m",
   "\x1b[32m",
   "\x1b[1m",
   "\x1b[33m",
   "\x1b[32m",
   "\x1b[33m",
   "\x1b[0m\x1b[31m",
   NULL
};
