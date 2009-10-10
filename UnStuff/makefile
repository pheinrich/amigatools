#
#  UnStuff
#  Copyright (c) 1990,1992,2009  Peter Heinrich
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301, USA.
#
# -----------
#  This makefile will produce UnStuff.  Duh.
#


NAME = UnStuff


#
#  Paths
#
S = Projects:$(NAME)/
I = -iINCLUDE: -i$(S)include/


#
#  Commands
#
ASM  = LC:asm
CC   = LC:lc
LINK = LC:blink


#
#  Assembler, compiler, and linker flags
#
AF = -d $(I) -s
CF = -cfist -d3 $(I) -j79i -v -w
LF = BATCH SMALLCODE SMALLDATA


#
#  Objects and libraries
#
#  Assembly objects
AOBJS =

#  C-language objects
COBJS = checksum.o display.o expand.o file.o process.o unstuff.o

OBJS = LIB:c.o $(AOBJS) $(COBJS)
LIBS = LIB:lcs.lib


#
#  The program
#
PROG = $(S)$(NAME)

$(PROG):  $(OBJS) $(LIBS) $(S)makefile debug
   $(LINK) >> $(NAME).lst <WITH < (prog.lnk)
      FROM     $(OBJS)
      TO       $(PROG)
      LIBRARY  $(LIBS)
      BUFSIZE  32768
      $(LF) NODEBUG
   <

debug:  $(OBJS) $(LIBS) $(S)makefile
   $(LINK) >> $(NAME).lst <WITH < (debug.lnk)
      FROM     $(OBJS)
      TO       debug
      LIBRARY  $(LIBS)
      BUFSIZE  32768
      $(LF)
   <


#
#  Transformation rules
#
.a.o:
   $(ASM) >> $(NAME).lst $(AF) $*

.c.o:
   $(CC) >> $(NAME).lst $(CF) $*
