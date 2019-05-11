#DEBUG = -g
DEBUG = -DNOCOUT -g
#DEBUG = -DNOCOUT -O3

CC=	g++	

SRC_OOO = \
	activelist.cpp \
	alu.cpp \
	busy.cpp \
	checkpoint.cpp \
	exception.cpp \
	fetch.cpp \
	instq.cpp \
	regfile.cpp \
	rmap.cpp \
	datapath.cpp \
	trace.cpp \
	magic.cpp \
	print.cpp \
	sim.cpp \
	main.cpp

OBJ_OOO = \
	activelist.o \
	alu.o \
	busy.o \
	checkpoint.o \
	exception.o \
	fetch.o \
	instq.o \
	regfile.o \
	rmap.o \
	datapath.o \
	trace.o \
	magic.o \
	print.o \
	sim.o \
	main.o

CC_OPTIONS = -c -Wall
LINK_OPTIONS = -Wall 
INCLUDE =

EXECUTABLE = ooo

all: $(EXECUTABLE)

regress1: $(EXECUTABLE)
	./$(EXECUTABLE)	> output
	diff -w output reference1 

regress2: $(EXECUTABLE)
	./$(EXECUTABLE)	> output
	diff -w output reference2 

$(EXECUTABLE): $(OBJ_OOO)
	$(CC) $(DEBUG) $(OBJ_OOO) -o $(EXECUTABLE) $(LINK_OPTIONS) 

depend:
	makedepend $(INCLUDE) $(SRC_OOO) 

.c.o:
	$(CC) $(GPROF) $(OPTIM) $(DEBUG) $(INCLUDE) $(CC_OPTIONS) $*.c

.cpp.o:
	$(CC) $(GPROF) $(OPTIM) $(DEBUG) $(INCLUDE) $(CC_OPTIONS) $*.cpp

clean:
	rm -f *.o *~ $(EXECUTABLE) Makefile.bak \#*\# libsim.a output

save: clean	
	tar -czf ./ver/`date +%s`.tgz *.cpp *.h Makefile README


# DO NOT DELETE

activelist.o: sim.h arch.h uarch.h magic.h print.h activelist.h regfile.h
activelist.o: rmap.h instq.h checkpoint.h
alu.o: sim.h arch.h uarch.h magic.h alu.h
busy.o: sim.h arch.h uarch.h busy.h
checkpoint.o: sim.h arch.h uarch.h checkpoint.h
exception.o: sim.h arch.h uarch.h magic.h exception.h checkpoint.h
fetch.o: sim.h arch.h uarch.h magic.h fetch.h trace.h
instq.o: sim.h arch.h uarch.h magic.h print.h instq.h checkpoint.h
regfile.o: sim.h arch.h uarch.h regfile.h
rmap.o: sim.h arch.h uarch.h rmap.h regfile.h checkpoint.h
datapath.o: sim.h arch.h uarch.h magic.h print.h datapath.h fetch.h trace.h
datapath.o: activelist.h regfile.h rmap.h instq.h alu.h busy.h exception.h
datapath.o: checkpoint.h
trace.o: sim.h arch.h uarch.h trace.h test.h
magic.o: sim.h arch.h uarch.h magic.h
print.o: sim.h arch.h uarch.h magic.h print.h checkpoint.h
sim.o: sim.h
main.o: sim.h arch.h uarch.h datapath.h fetch.h magic.h trace.h
