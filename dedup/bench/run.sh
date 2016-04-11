#!/bin/bash

BENCHMARK=dedup
VERSION=$1
INPUT=$2
NTHREADS=$3
EXTRA_ARGS=$4

BENCHPATH=${ROOT}/${BENCHMARK}

case $INPUT in
	"native") ARGS="-c -p -v -i ${BENCHPATH}/inputs/FC-6-x86_64-disc1.iso -o ${BENCHPATH}/outputs/output.dat.ddp";;
	"simlarge") ARGS="-c -p -v -i ${BENCHPATH}/inputs/media_l.dat -o ${BENCHPATH}/outputs/output.dat.ddp";;
	"simmedium") ARGS="-c -p -v -i ${BENCHPATH}/inputs/media_m.dat -o ${BENCHPATH}/outputs/output.dat.ddp";;
	"simsmall") ARGS="-c -p -v -i ${BENCHPATH}/inputs/media_s.dat -o ${BENCHPATH}/outputs/output.dat.ddp";;
	"simdev") ARGS="-c -p -v -i ${BENCHPATH}/inputs/hamlet.dat -o ${BENCHPATH}/outputs/output.dat.ddp";;
	"test") ARGS="-c -p -v -i ${BENCHPATH}/inputs/test.dat -o ${BENCHPATH}/outputs/output.dat.ddp";;
esac
if [ $VERSION = "ompss" ] || [ $VERSION = "omp" ] 
then
	export OMP_NUM_THREADS=${NTHREADS}
	NX_ARGS="${EXTRA_ARGS} --threads=${NTHREADS}" ${BENCHPATH}/bin/${BENCHMARK}-${VERSION} $ARGS -t 1
else
	${BENCHPATH}/bin/${BENCHMARK}-${VERSION} $ARGS -t ${NTHREADS}
fi
