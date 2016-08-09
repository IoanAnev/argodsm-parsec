MCXX = g++
EXEC = swaptions-serial 

OBJS= CumNormalInv.o MaxFunction.o RanUnif.o nr_routines.o icdf.o \
	HJM_SimPath_Forward_Blocking.o HJM.o HJM_Swaption_Blocking.o  \
	HJM_Securities.o

CXXFLAGS  += -Wno-deprecated -Wno-write-strings
LIBS:=$(LIBS) -lm

ifdef version
  ifeq "$(version)" "pthreads" 
    DEF := $(DEF) -DENABLE_THREADS
    CXXFLAGS := $(CXXFLAGS) -pthread
    EXEC=swaptions-pthreads
  endif

  ifeq "$(version)" "ompss"
    MCXX=mcxx
    DEF := $(DEF) -DENABLE_OMPSS
    CXXFLAGS := $(CXXFLAGS) --ompss --no-copy-deps
    EXEC=swaptions-ompss
  endif

  ifeq "$(version)" "ompss_instr"
    MCXX=mcxx
    DEF := $(DEF) -DENABLE_OMPSS
    CXXFLAGS := $(CXXFLAGS) --ompss --no-copy-deps --instrument
    EXEC=swaptions-ompss_instr
  endif

  ifeq "$(version)" "omp2"
    MCXX=g++
    DEF := $(DEF) -DENABLE_OMP2
    CXXFLAGS := $(CXXFLAGS) -fopenmp -DSCHED_POLICY=static
    EXEC=swaptions-omp2
  endif


  ifeq "$(version)" "omp4"
    MCXX=g++
    DEF := $(DEF) -DENABLE_OMP4
    CXXFLAGS := $(CXXFLAGS) -fopenmp 
    EXEC=swaptions-omp4
  endif

endif

all: $(EXEC)

$(EXEC): $(OBJS)
	$(MCXX) $(CXXFLAGS) $(LDFLAGS) $(DEF) $(OBJS) $(INCLUDE) $(LIBS) -o $(EXEC)

.cpp.o:
	$(MCXX) $(CXXFLAGS) $(DEF) -c $*.cpp -o $*.o

.c.o:
	$(MCXX) $(CXXFLAGS) $(DEF) -c $*.c -o $*.o

clean:
	rm -f $(OBJS) $(EXEC)

install: $(EXEC)
	mkdir -p ../bin
	cp -f $(EXEC) ../bin/$(EXEC)
	
