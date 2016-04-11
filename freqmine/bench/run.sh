#!/bin/bash

VERSION=$1
INPUT=$2
NTHREADS=$3
EXTRA_ARGS=$4

BENCHPATH=${ROOT}/freqmine

case $INPUT in
  "native") ARGS="${BENCHPATH}/inputs/webdocs_250k.dat 11000";;
  "simlarge") ARGS="${BENCHPATH}/inputs/kosarak_990k.dat 790";;
  "simmedium") ARGS="${BENCHPATH}/inputs/kosarak_500k.dat 410";;
  "simsmall") ARGS="${BENCHPATH}/inputs/webdocs_250k.dat 220";;
  "simdev") ARGS="${BENCHPATH}/inputs/T10I4D100K_1k.dat 3";;
  "test") ARGS="${BENCHPATH}/inputs/T10I4D100K_3.dat 1";;
esac

if [ "$VERSION" == "openmp" ] || [ "$VERSION" = "omp" ]
then
 	export OMP_NUM_THREADS=${NTHREADS}
fi

echo $OMP_NUM_THREADS

NX_ARGS="${EXTRA_ARGS} --threads=${NTHREADS}" ${BENCHPATH}/bin/freqmine-${VERSION} ${ARGS}
