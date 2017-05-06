PREFIX	= .

CROSS	?=
CFLAGS 	?= -static -Wall  -O2
LDFLAGS ?= -all-static -Wall -O2 

Q	?= 
CC 	:= $(CROSS)gcc
AR 	:= $(CROSS)ar
RANLIB 	:= $(CROSS)ranlib
OBJDUMP	:= $(CROSS)objdump
CFLAGS 	+= -I.

ARFLAGS = -cr

OBJS	= fylla.o jport.o jstate.o processor.o flash.o
INC	= jport.h jstate.h processor.h debug.h flash.h

LIBS	=
EXE	= fylla

.PHONY: all clean install uninstall

all: Makefile $(EXE)

%.o:%.c
	@echo CC $@
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<
	
$(EXE): $(OBJS) $(INC)
	@echo LD $@
	$(Q)$(CC) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

clean:
	$(Q)rm -vf *.o *.out *~ $(EXE)

install:
	$(Q)cp $(EXE) $(PREFIX)
	
uninstall:
	$(Q)rm -vf $(PREFIX)/$(EXE)

