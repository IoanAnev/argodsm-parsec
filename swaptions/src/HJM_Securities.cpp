//HJM_Securities.cpp
//Routines to compute various security prices using HJM framework (via Simulation).
//Authors: Mark Broadie, Jatin Dewanwala
//Collaborator: Mikhail Smelyanskiy, Jike Chong, Intel
//OmpSs/OpenMP 4.0 versions written by Dimitrios Chasapis - Barcelona Supercomputing Center
//ArgoDSM/OpenMP version written by Ioannis Anevlavis - Eta Scale AB
//OmpSs-2 version written by Ioannis Anevlavis - Eta Scale AB

#ifdef ENABLE_ARGO
#include "argo.hpp"
#endif

#include <omp.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "nr_routines.h"
#include "HJM.h"
#include "HJM_Securities.h"
#include "HJM_type.h"


#ifdef ENABLE_THREADS
#include <pthread.h>
#define MAX_THREAD 1024
#endif //ENABLE_THREADS

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

// Macro for only node0 to do stuff
#define WEXEC(rank, inst) ({ if ((rank) == 0) inst; })

int NUM_TRIALS = DEFAULT_NUM_TRIALS;
int nThreads = 1;
int nSwaptions = 1;
int iN = 11; 
FTYPE dYears = 5.5; 
int iFactors = 3; 
parm *swaptions;

long seed = 1979; //arbitrary (but constant) default value (birth year of Christian Bienia)
long swaption_seed;

// =================================================
FTYPE *dSumSimSwaptionPrice_global_ptr;
FTYPE *dSumSquareSimSwaptionPrice_global_ptr;
int chunksize;

int numtasks;
int workrank;

// static bool *gflag;
// static long *gseed;

void distribute(int& beg, int& end, const int& loop_size,
		const int& beg_offset, const int& less_equal)
{
	int chunk = loop_size / numtasks;
	beg = workrank * chunk + ((workrank == 0) ? beg_offset : less_equal);
	end = (workrank != numtasks - 1) ? workrank * chunk + chunk : loop_size;
}

void write_to_file()
{
	int i, rv;
	FILE *file;
	char *outputFile = (char*)"out.swaptions";

	file = fopen(outputFile, "w");
	if(file == NULL) {
		printf("ERROR: Unable to open file `%s'.\n", outputFile);
		exit(1);
	}
	for (i = 0; i < nSwaptions; i++) {
		rv = fprintf(file,"Swaption%d: [SwaptionPrice: %.10lf StdError: %.10lf] \n", 
				i, swaptions[i].dSimSwaptionMeanPrice, swaptions[i].dSimSwaptionStdError);
		if(rv < 0) {
			printf("ERROR: Unable to write to file `%s'.\n", outputFile);
			fclose(file);
			exit(1);
		}
	}
	rv = fclose(file);
	if(rv != 0) {
		printf("ERROR: Unable to close file `%s'.\n", outputFile);
		exit(1);
	}
}

void * worker(void *arg)
#if defined(ENABLE_OMPSS) || defined(ENABLE_OMPSS_2)
{
	int iSuccess;
	FTYPE pdSwaptionPrice[2];
	for(int i=0; i < nSwaptions; i++) {
#if defined(ENABLE_OMPSS)
		#pragma omp task firstprivate(i) private(iSuccess, pdSwaptionPrice) inout(swaptions[i])
#elif defined(ENABLE_OMPSS_2)
		#pragma oss task firstprivate(i) private(iSuccess, pdSwaptionPrice) inout(swaptions[i])
#endif
		{
			iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice,  swaptions[i].dStrike, 
					swaptions[i].dCompounding, swaptions[i].dMaturity, 
					swaptions[i].dTenor, swaptions[i].dPaymentInterval,
					swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, 
					swaptions[i].pdYield, swaptions[i].ppdFactors,
					100, NUM_TRIALS, BLOCK_SIZE, 0);
			assert(iSuccess == 1);
			swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
			swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
		}
	}
#if defined(ENABLE_OMPSS)
	#pragma omp taskwait
#elif defined(ENABLE_OMPSS_2)
	#pragma oss taskwait
#endif

	return NULL;    
}
#elif defined(ENABLE_OMP4)
{
	#pragma omp parallel 
	{
		#pragma omp single 
		{
			int iSuccess;
			FTYPE pdSwaptionPrice[2];
			for(int i=0; i < nSwaptions; i++) {
				#pragma omp task firstprivate(i) private(iSuccess, pdSwaptionPrice) //depend(inout: swaptions[i])
				#pragma omp task firstprivate(i) private(iSuccess, pdSwaptionPrice) depend(inout: swaptions[i])
				{
					iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice,  swaptions[i].dStrike, 
							swaptions[i].dCompounding, swaptions[i].dMaturity, 
							swaptions[i].dTenor, swaptions[i].dPaymentInterval,
							swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, 
							swaptions[i].pdYield, swaptions[i].ppdFactors,
							100, NUM_TRIALS, BLOCK_SIZE, 0);
					assert(iSuccess == 1);
					swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
					swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
				}
			}
			#pragma omp taskwait
		} //end of single  
	} //end of parallel region
	return NULL;    
}
#elif defined(ENABLE_OMP2)
{
	#pragma omp parallel 
	{
		int iSuccess;
		FTYPE pdSwaptionPrice[2];
		#pragma omp for schedule(SCHED_POLICY)
		for(int i=0; i < nSwaptions; i++) {
			iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice,  swaptions[i].dStrike, 
					swaptions[i].dCompounding, swaptions[i].dMaturity, 
					swaptions[i].dTenor, swaptions[i].dPaymentInterval,
					swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, 
					swaptions[i].pdYield, swaptions[i].ppdFactors,
					100, NUM_TRIALS, BLOCK_SIZE, 0);
			assert(iSuccess == 1);
			swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
			swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
		}
		#pragma omp taskwait
	} //end of parallel region
	return NULL;    
}
#elif defined(ENABLE_ARGO)
{
	int beg, end;
	distribute(beg, end, nSwaptions, 0, 0);

	#pragma omp parallel 
	{
		int iSuccess;
		FTYPE pdSwaptionPrice[2];
		#pragma omp for schedule(SCHED_POLICY)
		for(int i=beg; i < end; i++) {
			iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice,  swaptions[i].dStrike, 
					swaptions[i].dCompounding, swaptions[i].dMaturity, 
					swaptions[i].dTenor, swaptions[i].dPaymentInterval,
					swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, 
					swaptions[i].pdYield, swaptions[i].ppdFactors,
					100, NUM_TRIALS, BLOCK_SIZE, 0);
			assert(iSuccess == 1);
			swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
			swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
		}
		#pragma omp taskwait
	} //end of parallel region
	return NULL;    
}
#else
{
	int tid = *((int *)arg);
	FTYPE pdSwaptionPrice[2];

	int beg, end, chunksize;
	if (tid < (nSwaptions % nThreads)) {
		chunksize = nSwaptions/nThreads + 1;
		beg = tid * chunksize;
		end = (tid+1)*chunksize;
	} else {
		chunksize = nSwaptions/nThreads;
		int offsetThread = nSwaptions % nThreads;
		int offset = offsetThread * (chunksize + 1);
		beg = offset + (tid - offsetThread) * chunksize;
		end = offset + (tid - offsetThread + 1) * chunksize;
	}

	if(tid == nThreads -1 )
		end = nSwaptions;

	for(int i=beg; i < end; i++) {
		int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice,  swaptions[i].dStrike, 
				swaptions[i].dCompounding, swaptions[i].dMaturity, 
				swaptions[i].dTenor, swaptions[i].dPaymentInterval,
				swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, 
				swaptions[i].pdYield, swaptions[i].ppdFactors,
				swaption_seed+i, NUM_TRIALS, BLOCK_SIZE, 0);
		assert(iSuccess == 1);
		swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
		swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
	}

	return NULL;
}
#endif // ENABLE_OMPSS || ENABLE_OMPSS_2

//print a little help message explaining how to use this program
void print_usage(char *name) {
	fprintf(stderr,"Usage: %s OPTION [OPTIONS]...\n", name);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"\t-ns [number of swaptions (should be > number of threads]\n");
	fprintf(stderr,"\t-sm [number of simulations]\n");
	fprintf(stderr,"\t-nt [number of threads]\n");
	fprintf(stderr,"\t-sd [random number seed]\n");
}

//Please note: Whenever we type-cast to (int), we add 0.5 to ensure that the value is rounded to the correct number. 
//For instance, if X/Y = 0.999 then (int) (X/Y) will equal 0 and not 1 (as (int) rounds down).
//Adding 0.5 ensures that this does not happen. Therefore we use (int) (X/Y + 0.5); instead of (int) (X/Y);

int main(int argc, char *argv[])
{
#ifdef ENABLE_ARGO
	argo::init(10*1024*1024*1024UL);
	
	workrank = argo::node_id();
	numtasks = argo::number_of_nodes();
#else
	workrank = 0;
#endif

	int i,j;
	int beg, end;
	int iSuccess = 0;

	FTYPE **factors=NULL;

	//****bench begins****//

	WEXEC(workrank, printf("PARSEC Benchmark Suite\n"));
	WEXEC(workrank, fflush(NULL));


#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_begin(__parsec_swaptions);
#endif

	if(argc == 1)
	{
		WEXEC(workrank, print_usage(argv[0]));
		exit(1);
	}

#if defined(ENABLE_OMPSS) || defined(ENABLE_OMPSS_2) || defined(ENABLE_OMP2) || defined(ENABLE_OMP4) || defined(ENABLE_ARGO)
	WEXEC(workrank, printf("Warning! Argumetn -nt is ignored, use NX_ARGS for OMPSs or OMP_NUM_THREADS for OpenMP 4.0\n"));
#endif

	for (int j=1; j<argc; j++) {
		if (!strcmp("-sm", argv[j])) {NUM_TRIALS = atoi(argv[++j]);}
		else if (!strcmp("-nt", argv[j])) {nThreads = atoi(argv[++j]);} 
		else if (!strcmp("-ns", argv[j])) {nSwaptions = atoi(argv[++j]);}
		else if (!strcmp("-sd", argv[j])) {seed = atoi(argv[++j]);}
		else {
			WEXEC(workrank, fprintf(stderr,"Error: Unknown option: %s\n", argv[j]));
			WEXEC(workrank, print_usage(argv[0]));
			exit(1);
		}
	}

	if(nSwaptions < nThreads) {
		WEXEC(workrank, fprintf(stderr,"Error: Fewer swaptions than threads.\n"));
		WEXEC(workrank, print_usage(argv[0]));
		exit(1);
	}

	WEXEC(workrank, printf("Number of Simulations: %d,  Number of threads: %d Number of swaptions: %d\n", NUM_TRIALS, nThreads, nSwaptions));
	swaption_seed = (long)(2147483647L * RanUnif(&seed));

#if defined(ENABLE_THREADS)
	pthread_t      *threads;
	pthread_attr_t  pthread_custom_attr;

	if ((nThreads < 1) || (nThreads > MAX_THREAD))
	{
		fprintf(stderr,"Number of threads must be between 1 and %d.\n", MAX_THREAD);
		exit(1);
	}
	threads = (pthread_t *) malloc(nThreads * sizeof(pthread_t));
	pthread_attr_init(&pthread_custom_attr);
#elif defined(ENABLE_OMPSS) || defined(ENABLE_OMPSS_2) || defined(ENABLE_OMP2) || defined(ENABLE_OMP4) || defined(ENABLE_ARGO)
	//ignore number of threads
	nThreads = nSwaptions;
#else
	if (nThreads != 1)
	{
		fprintf(stderr,"Number of threads must be 1 (serial version)\n");
		exit(1);
	}
#endif //ENABLE_THREADS

	// initialize input dataset
	factors = dmatrix(0, iFactors-1, 0, iN-2);
	//the three rows store vol data for the three factors
	factors[0][0]= .01;
	factors[0][1]= .01;
	factors[0][2]= .01;
	factors[0][3]= .01;
	factors[0][4]= .01;
	factors[0][5]= .01;
	factors[0][6]= .01;
	factors[0][7]= .01;
	factors[0][8]= .01;
	factors[0][9]= .01;

	factors[1][0]= .009048;
	factors[1][1]= .008187;
	factors[1][2]= .007408;
	factors[1][3]= .006703;
	factors[1][4]= .006065;
	factors[1][5]= .005488;
	factors[1][6]= .004966;
	factors[1][7]= .004493;
	factors[1][8]= .004066;
	factors[1][9]= .003679;

	factors[2][0]= .001000;
	factors[2][1]= .000750;
	factors[2][2]= .000500;
	factors[2][3]= .000250;
	factors[2][4]= .000000;
	factors[2][5]= -.000250;
	factors[2][6]= -.000500;
	factors[2][7]= -.000750;
	factors[2][8]= -.001000;
	factors[2][9]= -.001250;

	// setting up multiple swaptions
	swaptions = 
#ifdef ENABLE_ARGO
		argo::conew_array<parm>(nSwaptions);

		// gflag = argo::conew_array<bool>(numtasks);
		// gseed = argo::conew_<long>(seed);
#else
		(parm *)malloc(sizeof(parm)*nSwaptions);
#endif

	int k;
#ifdef ENABLE_ARGO
	distribute(beg, end, nSwaptions, 0, 0);

	/* It fails correctness with certain memory policies (issue!)
	// done to avoid remote accesses for dYears, dStrike (*X)
	if (workrank != 0) {
		while(!gflag[workrank-1]) {
			argo::backend::acquire();
		}
	}

	// (*X)
	for (i = beg; i < end; i++) {
		swaptions[i].dYears = 5.0 + ((int)(60*RanUnif(gseed)))*0.25; //5 to 20 years in 3 month intervals
		swaptions[i].dStrike =  0.1 + ((int)(49*RanUnif(gseed)))*0.1; //strikes ranging from 0.1 to 5.0 in steps of 0.1 
	}

	// (*X)
	gflag[workrank] = 1;
	argo::backend::release();
	*/

	// let master process do the initialization for dYears and dStrike till issue is investigated
	if (workrank == 0) {
		for (i = 0; i < nSwaptions; i++) {
			swaptions[i].dYears = 5.0 + ((int)(60*RanUnif(&seed)))*0.25; //5 to 20 years in 3 month intervals
			swaptions[i].dStrike =  0.1 + ((int)(49*RanUnif(&seed)))*0.1; //strikes ranging from 0.1 to 5.0 in steps of 0.1 
		}
	}
	argo::barrier();

	//#pragma omp parallel for private(i, k, j) schedule(SCHED_POLICY)
	for (i = beg; i < end; i++) {
#else
	for (i = 0; i < nSwaptions; i++) {
#endif
		swaptions[i].Id = i;
		swaptions[i].iN = iN;
		swaptions[i].iFactors = iFactors;
#ifndef ENABLE_ARGO
		swaptions[i].dYears = 5.0 + ((int)(60*RanUnif(&seed)))*0.25; //5 to 20 years in 3 month intervals
		swaptions[i].dStrike =  0.1 + ((int)(49*RanUnif(&seed)))*0.1; //strikes ranging from 0.1 to 5.0 in steps of 0.1 
#endif
		swaptions[i].dCompounding =  0;
		swaptions[i].dMaturity =  1.0;
		swaptions[i].dTenor =  2.0;
		swaptions[i].dPaymentInterval =  1.0;

		swaptions[i].pdYield = dvector(0,iN-1);;
		swaptions[i].pdYield[0] = .1;
		for(j=1;j<=swaptions[i].iN-1;++j)
			swaptions[i].pdYield[j] = swaptions[i].pdYield[j-1]+.005;

		swaptions[i].ppdFactors = dmatrix(0, swaptions[i].iFactors-1, 0, swaptions[i].iN-2);
		for(k=0;k<=swaptions[i].iFactors-1;++k)
			for(j=0;j<=swaptions[i].iN-2;++j)
				swaptions[i].ppdFactors[k][j] = factors[k][j];
	}


	// **********Calling the Swaption Pricing Routine*****************

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_begin();
#endif

	//****ROI begins****//
	int startt = time(NULL);
#ifdef ENABLE_THREADS

	int threadIDs[nThreads];
	for (i = 0; i < nThreads; i++) {
		threadIDs[i] = i;
		pthread_create(&threads[i], &pthread_custom_attr, worker, &threadIDs[i]);
	}
	for (i = 0; i < nThreads; i++) {
		pthread_join(threads[i], NULL);
	}

	free(threads);

#else
	int threadID=0;
	worker(&threadID);
#ifdef ENABLE_ARGO
	argo::barrier();
#endif
#endif //ENABLE_THREADS
	WEXEC(workrank, std::cout << "Critical code execution time: " << time(NULL) - startt << std::endl);
	//***ROI ends***//

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_end();
#endif

#ifdef ENABLE_OUTPUT
	WEXEC(workrank, write_to_file());
#endif

#ifdef ENABLE_ARGO
	for (i = beg; i < end; i++) {
#else
	for (i = 0; i < nSwaptions; i++) {
#endif
		free_dvector(swaptions[i].pdYield, 0, swaptions[i].iN-1);
		free_dmatrix(swaptions[i].ppdFactors, 0, swaptions[i].iFactors-1, 0, swaptions[i].iN-2);
	}

#ifdef ENABLE_ARGO
	argo::codelete_array(swaptions);
#else
	free(swaptions);
#endif

	//***********************************************************


	//****bench ends***//
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_end();
#endif

#ifdef ENABLE_ARGO
	argo::finalize();
#endif

	return iSuccess;
}
