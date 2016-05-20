//------------------------------------------------------------------------
//      ____                        _      _
//     / ___|____ _   _ ____   ____| |__  | |
//    | |   / ___| | | |  _  \/ ___|  _  \| |
//    | |___| |  | |_| | | | | |___| | | ||_|
//     \____|_|  \_____|_| |_|\____|_| |_|(_) Media benchmarks
//
//	 � 2006, Intel Corporation, licensed under Apache 2.0
//
//  file :	 ParticleFilterOMPSS.h
//  author : Scott Ettinger - scott.m.ettinger@intel.com
//
//  description : Generic Particle Filter class.  Supports annealed
//				  particle filtering.  Templated on a model object
//				  which evaluates the particle likelihoods. Vector width
//				  is determined by the model initial state.  Number of
//				  annealing steps is determined by model StdDevs() function.
//
//				  Model object must support member functions :
//					std::vector<fpType> InitialState();
//					fpType LogLikelihood(std::vector<fpType> &v, bool &valid);
//					std::vector<std::vector<fpType> > StdDevs();
//					void GetObservation(fpType timeval);
//
//  modified : Chi Ching Chi <chi.c.chi@tu-berlin.de>
//--------------------------------------------------------------------------

#ifndef PARTICLEFILTEROMPSS_H
#define PARTICLEFILTEROMPSS_H

#include "config.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <fstream>
#include <sys/types.h>
#include <sys/time.h>
#include "RandomGenerator.h"
#include "AnnealingFactor.h"
#include "TrackingModelOMPSS.h"


#undef min
using namespace std;
//Generic particle filter class templated on model object
class ParticleFilterOMPSS{

public:
//Types
	typedef float fpType;
	typedef std::vector<fpType> Vectorf;

protected:
//variables
	TrackingModelOMPSS mModel;								// model object evaluates particle likelihoods
	std::vector<Vectorf > mParticles, mNewParticles;	//lists of particles
	Vectorf	mWeights, mCdf;								//particle weights, cumulative distribution
	Vectorf mBins, mSamples, mLikelihoods;				//resampling bins, new samples, particle likelihoods

	int mNParticles;									//number of particles used
	int mBestParticle;									//index of particle with highest likelihood
	int mMinParticles;									//minimum number of particles allowed
	vector<int> mIndex;
	vector<int> valid;
	vector<RandomGenerator> mRnd;					//random number generators - should be replaced with a single parallel leapfrog generator for better quality random numbers.
	bool mInitialized;									//particle initialization flag

//functions

	//calculate the cumulative distribution function from particle weights
	void CalcCDF(const Vectorf &weights, Vectorf &dst);

	//distribute particle randomly according to given standard deviations
	void AddGaussianNoise(Vectorf &p, const Vectorf &stdDevs, RandomGenerator &rnd) const;

	//Monte Carlo resampling given a CDF
	void Resample(Vectorf &cdf, Vectorf &bins, Vectorf &samples, int n);
	void ProcessWeights(vector<Vectorf > &particles);
	//calculate particle weights based on model likelihood
	void InitWeights(vector<Vectorf > &particles, Vectorf &initVector);

	void ComputeWeights(int k);


public:
   void ComputeWeightTask(int tid, int k, int n);
	//Constructors
	ParticleFilterOMPSS() {mMinParticles = 5; mInitialized = false;};
	ParticleFilterOMPSS(const string &path, int cameras, int layers, int particles);
	~ParticleFilterOMPSS() {};

	//Get Functions
	TrackingModelOMPSS &Model()						{return mModel; };
	int NumParticles() const					{return mNParticles; };
	int GoodParticles()	const					{return (int)mParticles.size(); };
	std::vector<Vectorf > &StdDevs() 			{return mModel.StdDevs(); };

	//Set Functions
	void SetModel(TrackingModelOMPSS &model)			{mModel = model; };
	void SetMinimumParticles(int n)				{mMinParticles = n;};

	//Set number of particles to n and generate initial values
	void InitializeParticles(int n);

	//Update filter to a new set of particles - returns false if model observation fails
	//calls model to get observation at given time, and to get likelihoods of each particle
	bool Update(fpType timeVal, vector<BinaryImage> &mFGMaps, vector<FlexImage8u> &mEdgeMaps, MultiCameraProjectedBody &mProjection, vector<float> &estimate);

	//State estimate (weighted mean of all particles)
	void Estimate(Vectorf &estimate);

	//Return particle with highest likelihood
	void BestParticle(Vectorf &p) {p = mParticles[mBestParticle]; };

};
#pragma omp task priority(0) inout(*pf, *timeval) in(*mFGMaps, *mEdgeMaps) out(*mProjection, *estimate) 
void UpdateTaskWrapper(ParticleFilterOMPSS *pf, int *timeval, vector<BinaryImage> *mFGMaps, vector<FlexImage8u> *mEdgeMaps, MultiCameraProjectedBody *mProjection, vector<float> *estimate);

#endif

