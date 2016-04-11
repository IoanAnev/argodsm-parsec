TARGET=streamcluster-serial
OBJS=streamcluster.o

ifdef version
  ifeq "$(version)" "pthreads"
    CXXFLAGS := $(CXXFLAGS) -DENABLE_THREADS -pthread
    OBJS += parsec_barrier.o
	TARGET = streamcluster-pthreads
  endif
  ifeq "$(version)" "ompss"
    CXX := mcxx
    CXXFLAGS := $(CXXFLAGS) -DENABLE_OMPSS --ompss --no-copy-deps
    OBJS = ompss_streamcluster.o
	TARGET = streamcluster-ompss
  endif
  ifeq "$(version)" "ompss_instr"
    CXX := mcxx
    CXXFLAGS := $(CXXFLAGS) -DENABLE_OMPSS --ompss --no-copy-deps --instrument
    OBJS = ompss_streamcluster.o
	TARGET = streamcluster-pthreads
  endif
  ifeq "$(version)" "omp4"
    CXX := g++
    CXXFLAGS := $(CXXFLAGS) -DENABLE_OMPSS -fopenmp
    OBJS = omp_streamcluster.o
  endif
endif

all: $(MAKEOBJECTIVES) $(OBJS)
	$(CXX) -o $(TARGET) $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LIBS)

## Files *.o need *.cpp, compile them
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)
 
install:
	mkdir -p ../bin
	cp -f $(TARGET) ../bin/$(TARGET)-$(version)

