#!/bin/bash

VERSION=$1
INPUT=$2
NTHREADS=$3
NDIVS=$4

if [ -z "$VAR" ]; then
    NDIVS=${NTHREADS}
fi

### STREAMCLUSTER ARGS###
### Define arguments for the PARSEC Benchmark and different input sizes ###
case $INPUT in
  "native") ARGS="10 20 128 1000000 200000 5000 none output.txt ${NDIVS}";;
  "simlarge") ARGS="10 20 128 16384 16384 1000 none output.txt ${NDIVS}";;
  "simmedium") ARGS="10 20 64 8192 8192 1000 none output.txt ${NDIVS}";;
  "simsmall") ARGS="10 20 32 4096 4096 1000 none output.txt ${NDIVS}";;
  "simdev") ARGS="3 10 3 16 16 10 none output.txt ${NDIVS}";;
  "test") ARGS="2 5 1 10 10 5 none output.txt ${NDIVS}";;
esac

#SPECIFY RUNTIME THREADS
case ${VERSION} in
    ompss*)
        NX_ARGS="--threads=${NTHREADS} ${NX_ARGS}"
        ;;
    omp* )
	    export OMP_NUM_THREADS=${NTHREADS}
        ;;
    pthreads* | serial*)
        ;;
    *)
        echo -e "\033[0;31mVERSION = $VERSION not correct, stopping $BENCHID run\033[0m"
        exit
        ;;
esac

./streamcluster-${VERSION} ${ARGS}
