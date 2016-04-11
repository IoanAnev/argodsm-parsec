//-------------------------------------------------------------
//      ____                        _      _
//     / ___|____ _   _ ____   ____| |__  | |
//    | |   / ___| | | |  _  \/ ___|  _  \| |
//    | |___| |  | |_| | | | | |___| | | ||_|
//     \____|_|  \_____|_| |_|\____|_| |_|(_) Media benchmarks
//
//	  2006, Intel Corporation, licensed under Apache 2.0
//
//  file : TrackingModelOMPSS.cpp
//  author : Scott Ettinger - scott.m.ettinger@intel.com
//  description : Observation model for kinematic tree body
//				  tracking.
//
//  modified :Chi Ching Chi <chi.c.chi@tu-berlin.de>
//--------------------------------------------------------------

#include "config.h"
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/time.h>
#include "TrackingModelOMPSS.h"
#include "CovarianceMatrix.h"
#include "../FlexImageLib/FlexLib.h"
#include "system.h"

#ifndef uint
#define uint unsigned int
#endif

using namespace std;

//templated conversion to string with field width
template<class T>
inline string str(T n, int width = 0, char pad = '0'){
    stringstream ss;
    ss << setw(width) << setfill(pad) << n;
    return ss.str();
}

// -------------------------------- Initialization ----------------------------

TrackingModelOMPSS::TrackingModelOMPSS(const string &path, int cameras, int layers, int particles) {
    mPath = path;
    mNCameras = cameras;
    mFGMaps.resize(cameras);
    mEdgeMaps.resize(cameras);
    vector<string> calibFiles(cameras);

    // particles/tasks +1 as first is used for shared initial state
    mImageMeasurement.resize(particles+1);
    mPose.resize(particles+1);
    mBody.resize(particles+1);
    mProjection.resize(particles+1);

    for(int i = 0; i < cameras; i++)
        calibFiles[i] = path + "CALIB" + DIR_SEPARATOR + "Camera" + str(i + 1) + ".cal";

    if(!InitCameras(calibFiles)){
        exit(-1);
    }

    if(!InitGeometry(path + "BodyShapeParameters.txt")){
        exit(-1);
    }
    //initialize body pose angles and translations
    if(!LoadInitialState(path + "InitialPose.txt")){
        exit(-1);
    }
    //initialize pose statistics
    if(!LoadPoseParameters(path + "PoseParameters.txt")){
        exit(-1);
    }
    //generate annealing rates for particle filter using pose parameters
    GenerateStDevMatrices(layers, mPose[0].Params(), mStdDevs);
}


//Load camera calibration parameters from files
bool TrackingModelOMPSS::InitCameras(vector<string> &fileNames){
    mCameras.SetCameras((int)fileNames.size());									//set number of cameras
    for(int i = 0; i < (int)fileNames.size(); i++)
        if(!mCameras(i).LoadParams(fileNames[i].c_str()))						//load each camera calibration
            return false;
    return true;
}

// //Calculate the likelihood for the current observation
// float TrackingModelOMPSS::LogLikelihood(const vector<float> &v, bool &valid){
//     mPose.Set(v);                                           //set pose angles and translation
//     valid = false;
//     if(!mPose.Valid(mPose.Params()))                        //test for a valid pose (reject impossible body angles)
//         return -1e10;
//     mBody.ComputeGeometry(mPose, mBody.Parameters());       //compute 3D model geometry from pose (generate conic cylinders and their transforms)
//     if(!mBody.Valid())                                      //test for valid geometry (reject poses with intersecting body parts)
//         return -1e10;
//     mProjection.ImageProjection(mBody, mCameras);           //compute projected 2D points into each camera image for each body part
//     float err = mImageMeasurement.ImageErrorEdge(mEdgeMaps, mProjection);       //compute cylinder edge map term
//     err += mImageMeasurement.ImageErrorInside(mFGMaps, mProjection);            //compute silhouette term
//     valid = true;
//     return -err;
// }

//Calculate the likelihood for the current observation
void TrackingModelOMPSS::LogLikelihood(const vector<float> &v, float &weight, int tid, int &valid){
    tid++;
    mPose[tid].Set(v);                                              //set pose angles and translation
    valid = 0;
    if(!mPose[tid].Valid(mPose[0].Params())){                       //test for a valid pose (reject impossible body angles)
        weight=-1e10;
        return;
    }
    mBody[tid].ComputeGeometry(mPose[tid], mBody[0].Parameters());  //compute 3D model geometry from pose (generate conic cylinders and their transforms)

    if(!mBody[tid].Valid()){                                        //test for valid geometry (reject poses with intersecting body parts)
        weight=-1e10;
        return;
    }
    mProjection[tid].ImageProjection(mBody[tid], mCameras);         //compute projected 2D points into each camera image for each body part

    float err = mImageMeasurement[tid].ImageErrorEdge(mEdgeMaps, mProjection[tid]); //compute cylinder edge map term
    err += mImageMeasurement[tid].ImageErrorInside(mFGMaps, mProjection[tid]);      //compute silhouette term
    weight= -err;
    valid = 1;
    return;
}

MultiCameraProjectedBody& TrackingModelOMPSS::EstimatedProjection(const vector<float> &estimate){
    //set pose angles and translation
    mPose[0].Set(estimate);
    //compute 3D model geometry from pose (generate conic cylinders and their transforms)
    mBody[0].ComputeGeometry(mPose[0], mBody[0].Parameters());
    //compute projected 2D points into each camera image for each body part
    mProjection[0].ImageProjection(mBody[0], mCameras);

    return mProjection[0];
}

