//------------------------------------------------------------------------
//      ____                        _      _
//     / ___|____ _   _ ____   ____| |__  | |
//    | |   / ___| | | |  _  \/ ___|  _  \| |
//    | |___| |  | |_| | | | | |___| | | ||_|
//     \____|_|  \_____|_| |_|\____|_| |_|(_) Media benchmarks
//
//   � 2006, Intel Corporation, licensed under Apache 2.0
//
//  file :   ParticleFilterOMPSS.h
//  author : Scott Ettinger - scott.m.ettinger@intel.com
//
//  description : Generic Particle Filter class.  Supports annealed
//                particle filtering.  Templated on a model object
//                which evaluates the particle likelihoods. Vector width
//                is determined by the model initial state.  Number of
//                annealing steps is determined by model StdDevs() function.
//
//                Model object must support member functions :
//                  std::vector<fpType> InitialState();
//                  fpType LogLikelihood(std::vector<fpType> &v, bool &valid);
//                  std::vector<std::vector<fpType> > StdDevs();
//                  void GetObservation(fpType timeval);
//
//  modified : Chi Ching Chi <chi.c.chi@tu-berlin.de>
//--------------------------------------------------------------------------

# include "config.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <fstream>
#include <sys/types.h>
#include <sys/time.h>
#include "RandomGenerator.h"
#include "AnnealingFactor.h"
#include "ParticleFilterOMPSS.h"

//------------------------------------------ Implementation -----------------------------------------------------
#ifndef uint
#define uint unsigned int
#endif

//vector multiply by scalar
template<class T1, class T2>
inline void operator*=(std::vector<T1> &v, const T2 c)
{	for(uint i = 0; i < v.size(); i++)
        v[i] *= c;
}

//vector constant subtraction
template<class T1, class T2>
inline void operator-=(std::vector<T1> &v, const T2 c)
{	for(uint i = 0; i < v.size(); i++)
        v[i] -= c;
}

ParticleFilterOMPSS::ParticleFilterOMPSS(const string &path, int cameras, int layers, int particles):
    mModel(path, cameras, layers, particles),
    mIndex(particles),
    valid(particles)
{
    mMinParticles = 5;
    InitializeParticles(particles);
    mInitialized = true;
}

//Generate a set of inital particles
void ParticleFilterOMPSS::InitializeParticles(int n) {
    //initialize random number generators
    mRnd.resize(n);
    for(int i = 0; i < n; i++)
        mRnd[i].Seed(i * 2);
    mNParticles = n;
    Vectorf initVector = mModel.InitialState();						//get inital state vector
    if(initVector.size() != mModel.StdDevs()[0].size() )
        std::cout << "Warning!! PF::Model initial Vector and stdDev vector are not equal lengths!" << std::endl;
    bool minValid = false;
    while(!minValid){
        mParticles.resize(n);
        //calculate initial weights and remove any particles that are invalid by model prior
        InitWeights(mParticles, initVector);
// 		ProcessWeights(mParticles); //not visibly affecting output if not done
        //repeat until minimum number of valid particles is met
        minValid = (int)mParticles.size() >= mMinParticles;
// 		if(!minValid)
// 			std::cout << "Warning : initial particle set does not meet minimum number of particles. Resampling.." << std::endl;
    }
    mCdf.resize(n);         //allocate space
    mSamples.resize(n);
    mBins.resize(n);
    mInitialized = true;
}

//calculate particle weights (mWeights) and find highest likelihood particle.
//computes an optimal annealing factor and scales the likelihoods.
//Also removes any particles reported as invalid by the model.
void ParticleFilterOMPSS::InitWeights(vector<Vectorf > &particles, Vectorf &initVector){
    mWeights.resize(particles.size());
    valid.resize(particles.size());

    //create new particle with randomized initial value
    for(uint i = 0; i < particles.size(); i++){
        Vectorf &p = particles[i];
        p = initVector;
        //distribute particles randomly about the initial vector
        AddGaussianNoise(p, mModel.StdDevs()[0], mRnd[i]);
    }

}

//distribute particle randomly according to given standard deviations
void ParticleFilterOMPSS::AddGaussianNoise(Vectorf &p, const Vectorf &stdDevs, RandomGenerator &rnd) const {
    for(uint i = 0; i < stdDevs.size(); i++)
        p[i] += (fpType)rnd.RandN() * stdDevs[i];
}

//calculate the CDF from particle weights
void ParticleFilterOMPSS::CalcCDF(const Vectorf &weights, Vectorf &dst){
    dst.resize(weights.size());
    dst[0] = weights[0];
    //compute cumulative sum
    for(uint i = 1; i < weights.size(); i++)
        dst[i] = dst[i - 1] + weights[i];
    dst *= fpType(1.0) / dst[dst.size() - 1];	//normalize cdf
}

//Monte Carlo resampling given a cdf.  Uses incremental exponential random values to
//create a sorted uniform random sample for fast inverse of the cdf for all samples in 1 pass
void ParticleFilterOMPSS::Resample(Vectorf &cdf, Vectorf &bins, Vectorf &samples, int n) {
    RandomGenerator r;
    samples[0] = (fpType)r.RandExp();
    //generate a new set of sorted random samples
    for(int i = 1; i < n; i++)
        samples[i] = samples[i - 1] + (fpType)r.RandExp();
    //normalize
    samples *= (fpType)1.0 / (samples[samples.size() - 1]);
    //prevent overrun due to numerical error
    cdf[cdf.size() - 1] += 1;

    //populate sample bins based on probability distribution
    int p = 0;
    bins.assign(cdf.size(), 0);
    for(uint i = 0; i < samples.size(); i++) {
        while(cdf[p] < samples[i])
            p++;
        bins[p]++;
    }
}

void ParticleFilterOMPSS::ProcessWeights(vector<Vectorf > &particles){
    mBestParticle = 0;
    fpType total = 0, best = 0, minWeight = 1e30f, annealingFactor = 1;

    uint i = 0;
    while(i < particles.size()) {
        //if not valid(model prior), remove the particle from the list
        if(!valid[i]){
            particles[i] = particles[particles.size() - 1];
            mWeights[i] = mWeights[particles.size() - 1];
            valid[i] = valid[valid.size() - 1];
            particles.pop_back(); mWeights.pop_back(); valid.pop_back();
            continue;
        }

        minWeight = std::min(mWeights[i++], minWeight);
    }
    //bail out if not enough valid particles
    if((int)particles.size() < mMinParticles) return;

    mWeights -= minWeight;  //shift weights to zero for numerical stability
    //calculate annealing factor if more than 1 step
    if(mModel.StdDevs().size() > 1)
        annealingFactor = BetaAnnealingFactor(mWeights, 0.5f);
    for(uint i = 0; i < mWeights.size(); i++) {
        double wa = annealingFactor * mWeights[i];
        mWeights[i] = (float)exp(wa);
        total += mWeights[i];             //save sum of all weights
        if(i == 0 || mWeights[i] > best)	//find highest likelihood particle
        {	best = mWeights[i];
            mBestParticle = i;
        }
    }
    mWeights *= fpType(1.0) / total;      //normalize weights
}

void ParticleFilterOMPSS::ComputeWeightTask(int tid, int k, int n){
    for (int i=tid; i< tid+n; i++){
        mNewParticles[i] = mParticles[mIndex[i]];
        AddGaussianNoise(mNewParticles[i], mModel.StdDevs()[k], mRnd[i]);
        //compute log-likelihood weights for each particle
        mModel.LogLikelihood(mNewParticles[i], mWeights[i], tid, valid[i]);
    }
}

void ParticleFilterOMPSS::ComputeWeights(int k){
    int n = 16; //ompss task grouping
    int p = 0;

    mNewParticles.resize(mNParticles);
    mWeights.resize(mNParticles);
    valid.resize(mNParticles);
    for(int i = 0; i < (int)mBins.size(); i++)
        for(uint j = 0; j < mBins[i]; j++)			//index particles to be regenerated
            mIndex[p++] = i;

    for(int i = 0; i < mNParticles; i+=n){
        #pragma omp task priority(1) 
        ComputeWeightTask( i, k, (i+n > mNParticles)? mNParticles-i: n );
    }
    #pragma omp taskwait
}


void UpdateTaskWrapper(ParticleFilterOMPSS *pf, int *timeval, vector<BinaryImage> *mFGMaps, vector<FlexImage8u> *mEdgeMaps, MultiCameraProjectedBody *mProjection, vector<float> *estimate) {
    cout << "Processing frame " << *timeval << endl;
    pf->Update( (float)*timeval, *mFGMaps, *mEdgeMaps, *mProjection, *estimate);
}

//Particle filter update (model and observation updates must be called first)
//weights have already been computed from previous step or initialization
bool ParticleFilterOMPSS::Update(fpType timeval, vector<BinaryImage> &mFGMaps, vector<FlexImage8u> &mEdgeMaps, MultiCameraProjectedBody &mProjection, vector<float> &estimate) {
    mModel.mFGMaps = mFGMaps;
    mModel.mEdgeMaps = mEdgeMaps;
    //loop over all annealing steps starting with highest
    for(int k = (int)mModel.StdDevs().size() - 1; k >= 0 ; k--){
        CalcCDF(mWeights, mCdf); 		//Monte Carlo re-sampling
        Resample(mCdf, mBins, mSamples, mNParticles);

        bool minValid = false;

        while(!minValid){
            //calculate particle weights (parallel hotspot)
            ComputeWeights(k);
            //remove any invalid particles and find best particle
            ProcessWeights(mNewParticles);
            //repeat if not enough valid particles
            minValid = (int)mNewParticles.size() >= mMinParticles;
            if(!minValid)
                std::cout << "Not enough valid particles - Resampling!!!" << std::endl;
        }

        mParticles = mNewParticles;		//save new particle set
    }

    Estimate(estimate);

    mProjection= mModel.EstimatedProjection(estimate);

    return true;
}

//calculate expected value of the particle set
void ParticleFilterOMPSS::Estimate(Vectorf &estimate){
    //clear estimate values
    estimate.assign(mParticles[0].size(), 0);
    //calculate expected value
    for(uint i = 0; i < mParticles.size(); i++)
        for(uint j = 0; j < estimate.size(); j++)
            estimate[j] += mParticles[i][j] * mWeights[i];
}
