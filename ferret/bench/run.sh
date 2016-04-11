#!/bin/bash

VERSION=$1
INPUT=$2
NTHREADS=$3
EXTRA_ARGS=$4

BENCHPATH=${ROOT}/ferret

case $INPUT in
  "native") ARGS="${BENCHPATH}/inputs/input_native/corel lsh ${BENCHPATH}/inputs/input_native/queries 50 20";;
  "simlarge") ARGS="${BENCHPATH}/inputs/input_simlarge/corel lsh ${BENCHPATH}/inputs/input_simlarge/queries 10 20";;
  "simmedium") ARGS="${BENCHPATH}/inputs/input_simmedium/corel lsh ${BENCHPATH}/inputs/input_simmedium/queries 10 20";;
  "simsmall") ARGS="${BENCHPATH}/inputs/input_simsmall/corel lsh ${BENCHPATH}/inputs/input_simsmall/queries 10 20";;
  "simdev") ARGS="${BENCHPATH}/inputs/input_simsmdev/corel lsh ${BENCHPATH}/inputs/input_simdev/queries 5 5";;
  "test") ARGS="${BENCHPATH}/inputs/input_test/corel lsh ${BENCHPATH}/inputs/input_test/queries 5 5";;
esac

if [ $VERSION = "omp" ]
then
	export OMP_NUM_THREADS=${NTHREADS}
fi
#export NX_STACK_SIZE=200000
#echo "${BENCHPATH}/bin/ferret-${VERSION} ${ARGS} ${NTHREADS} ${BENCHPATH}/outputs/output.txt"
NX_ARGS="${EXTRA_ARGS} --threads=${NTHREADS} --disable-ut" ${BENCHPATH}/bin/ferret-${VERSION} ${ARGS} ${NTHREADS} ${BENCHPATH}/outputs/output.txt
