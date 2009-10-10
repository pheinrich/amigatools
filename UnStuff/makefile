#
#  UnStuff v2.07
#  © 1990,1992  Peter Heinrich
#
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
