//-------------------------------------------------------------
//      ____                        _      _
//     / ___|____ _   _ ____   ____| |__  | |
//    | |   / ___| | | |  _  \/ ___|  _  \| |
//    | |___| |  | |_| | | | | |___| | | ||_|
//     \____|_|  \_____|_| |_|\____|_| |_|(_) Media benchmarks
//
//	  2006, Intel Corporation, licensed under Apache 2.0
//
//  file : TrackingModelOMPSS.h
//  author : Scott Ettinger - scott.m.ettinger@intel.com
//  description : Observation model for kinematic tree body
//				  tracking.

//				  Derived classes implement multi-threaded
//				  versions of this class, however some of the
//				  capability for multithreading is implemented
//				  here.  Memory is allocated for separate threads
//				  if necessary and the likelihood function takes
//				  a thread id.  Defaults are for 1 thread.
//
//  modified : Chi Ching Chi <chi.c.chi@tu-berlin.de>
//--------------------------------------------------------------

#ifndef TRACKINGMODELOMPSS_H
#define TRACKINGMODELOMPSS_H

#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif

#include <vector>
#include <string>
#include "../FlexImageLib/FlexImage.h"
#include "ImageMeasurements.h"
#include "ImageProjection.h"
#include "CameraModel.h"
#include "BodyGeometry.h"
#include "../FlexImageLib/BinaryImage.h"

#define FlexImage8u  FlexImage<Im8u,1>
#define FlexImage32f FlexImage<Im32f,1>

using namespace std;

class TrackingModelOMPSS{
protected:

    vector< vector<float> >	mStdDevs;           // standard deviations for each layer of annealed particle filtering
    vector<ImageMeasurements>   mImageMeasurement;  // Image measurement objects for error computation
    int                     mNCameras;          // number of cameras used
    string                  mPath;              // dataset path

    //Load Camera(s) parameters from configuration files
    bool InitCameras(std::vector<std::string> &calibFiles);

    //Initialize body model geometry parameters and initial pose from configuration file
    bool InitGeometry(const std::string &fname) {return mBody[0].InitBodyShape(fname); };

    //Load initial state from a file
    bool LoadInitialState(const std::string &fname) {return mPose[0].Initialize(fname); };

    //Load Body Pose Parameters from a file
    bool LoadPoseParameters(const std::string &fname) {return mPose[0].InitParams(fname); };

public:
    MultiCamera                         mCameras;           // All cameras
    vector<BodyPose>                    mPose;              // Body poses and displacement parameters
    vector<BodyGeometry>                mBody;              // Body geometry objects
    vector<BinaryImage>                 mFGMaps;            // Background segmented images from each camera
    vector<FlexImage8u>                 mEdgeMaps;          // edge processed images from each camera
    vector<MultiCameraProjectedBody>    mProjection;        // Image projections of the body on all cameras
    TrackingModelOMPSS();
    TrackingModelOMPSS(const string &path, int cameras, int layers, int particles);
    ~TrackingModelOMPSS() {};

    //Get function for standard deviations of pose motion
    std::vector<std::vector<float> > &StdDevs() {return mStdDevs; };

    //Load body parameters, camera calibrations, and initial state from dataset
    bool Initialize(const std::string &path, int cameras, int layers);
    int NumCameras() {return mNCameras;};
    //Returns body pose state - required by particle filter for initialization only
    vector<float> InitialState() {return mPose[0].Pose(); };

    //Calculates Log likelihood for a given set of body pose angles (and translation - all are in the vector)
// 	float LogLikelihood(const std::vector<float> &v, bool &valid);
    void  LogLikelihood(const vector<float> &v, float &weight, int tid, int &valid);
    MultiCameraProjectedBody& EstimatedProjection(const vector<float> &v);
};


#endif
