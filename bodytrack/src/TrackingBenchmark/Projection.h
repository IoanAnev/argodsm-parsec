/*
  Copyright (C) 2013 Chi Ching Chi <chi.c.chi@tu-berlin.de>

  This file is part of Starbench.

  Starbench is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Starbench is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Starbench.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OutputBMP_H
#define OutputBMP_H

#include "config.h"
#include <iostream>
#include <vector>
#include <fstream>

#include "BodyGeometry.h"
#include "FlexImageLib/FlexImage.h"
#include "ImageMeasurements.h"
#include "ImageProjection.h"

using namespace std;

class Projection{
	// Body poses and displacement parameters
// 	vector<BodyPose> mPoses;
// 	// Body geometry objects
// 	vector<BodyGeometry> mBodies;
// 	// Image projections of the body on all cameras
// 	vector<MultiCameraProjectedBody> mProjections;
// 	// All cameras
// 	MultiCamera	mCameras;

	vector<string> ImageFiles;
	FlexImage<Im8u,3> dstImage[4];
   FlexImage<Im8u,1> srcImage[4], imds[4];
	int w;
	int h;
	int levels;
	int	mNCameras;
	string mPath;

public:
	Projection(string path, int cameras);
	void createBMP(MultiCameraProjectedBody &mProjection, int frame);
   void createBMPPart(int camera, MultiCameraProjectedBody &mProjection, FlexImage<Im8u,3> &result);
};
#pragma omp task priority(1) inout(*pj) in(*mProjection) 
void createBMPTaskWrapper(Projection* pj, MultiCameraProjectedBody *mProjection, int frame);

#endif

