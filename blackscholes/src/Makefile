
CC		= g++
SRC     = blackscholes.c
CSRC    = blackscholes.c
TARGET  = blackscholes-serial 
M4_BASE = .
M4      = m4
MACROS  = c.m4.pthreads
LDFLAGS += -lm


ifdef version
  ifeq "$(version)" "pthreads"
    M4_SRC  = blackscholes.m4.cpp
    CSRC    = $(M4_SRC)
    MT      = -DENABLE_THREADS
    CFLAGS	+= -pthread
    TARGET  = blackscholes-pthreads
  endif
  ifeq "$(version)" "omp2"
    MT        = -DENABLE_OPENMP
    CFLAGS    += -fopenmp -DSCHED_POLICY=static
    TARGET    = blackscholes-omp2
  endif
  ifeq "$(version)" "ompss"
    CC      = mcc
    CSRC	= blackscholes-ompss.c
    MT      = -DENABLE_OMPSS
    CFLAGS	+= --ompss --no-copy-deps
    TARGET  = blackscholes-ompss
  endif
  ifeq "$(version)" "ompss_instr"
    CC      = mcc
    CSRC	= blackscholes-ompss.c
    MT      = -DENABLE_OMPSS
    CFLAGS	+= --ompss --no-copy-deps --instrument
    TARGET  = blackscholes-ompss_instr
  endif
  ifeq "$(version)" "omp4"
    CC      = gcc
    CSRC	= blackscholes-omp.c
    MT      = -DENABLE_OPENMP
    CFLAGS	+= -fopenmp
    TARGET	= blackscholes-omp4
  endif

else
  MT        = 
endif

# Default build single precision version
NCO     = -DNCO=4

ifdef chk_err
ERR     = -DERR_CHK
endif

ifdef single
NCO = -DNCO=4
endif

ifdef size
SZ = -DN=$(size)
else
SZ = -DN=960
endif

ifdef double
NCO = -DNCO=2
endif

CFLAGS += $(MT) $(SZ) $(NCO) $(FUNC) $(ERR) $(CSRC)

all: $(TARGET)

$(TARGET): clean $(CSRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(TARGET)

$(M4_SRC): $(SRC)
	$(M4) $(M4_BASE)/$(MACROS) $< > $@

clean:
	rm -f $(TARGET) $(M4_SRC)

install:
	mkdir -p ../bin
	cp -f $(TARGET) ../bin/$(TARGET)
