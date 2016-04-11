#!/bin/bash

VERSION=$1
INPUT=$2
NTHREADS=$3
EXTRA_ARGS=$4

BENCHPATH=${ROOT}/swaptions

case $INPUT in
  "native") ARGS="-ns 128 -sm 1000000";;
  "simlarge") ARGS="-ns 64 -sm 20000";;
  "simmedium") ARGS="-ns 64 -sm 20000";;
  "simsmall") ARGS="-ns 16 -sm 5000";;
  "simdev") ARGS="-ns 3 -sm 50";;
  "test") ARGS="=-ns 1 -sm 5"
esac

if [ $VERSION = "omp" ]
then
	export OMP_NUM_THREADS=${NTHREADS}
fi

NX_ARGS="${EXTRA_ARGS} --threads=${NTHREADS}" ${BENCHPATH}/bin/swaptions-${VERSION}  ${ARGS} -nt ${NTHREADS}
