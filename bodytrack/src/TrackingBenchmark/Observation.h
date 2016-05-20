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

#ifndef OBSERVATION_H
#define OBSERVATION_H

#include "config.h"
#include <vector>
#include <string>
#include "FlexImageLib/FlexImage.h"
#include "ImageMeasurements.h"
#include "ImageProjection.h"
#include "CameraModel.h"
#include "BodyGeometry.h"
#include "FlexImageLib/BinaryImage.h"

#define FlexImage8u  FlexImage<Im8u,1>
#define FlexImage32f FlexImage<Im32f,1>

using namespace std;

class Observation{

protected:
	BinaryImage		mFGMap;		// Background segmented images from each camera
	FlexImage8u		mEdgeMap;	// edge processed images from each camera
	int				mCameraID;
	string			mPath;

	//Generate an edge map from the original camera image
	void CreateEdgeMap(const FlexImage8u &src, FlexImage8u &dst);

public:

	Observation(string path, int camera);
	~Observation() {};

	//Load and process new observation data from image files for a given time(frame).  Generates edge maps from the raw image files.
	bool GetObservationEdge(float timeval, FlexImage8u &outEdgeMap);

	//Load and process new observation data from image files for a given time(frame).  Generates binary maps from the raw image files.
	bool GetObservationFG(float timeval, BinaryImage &outFGMap);
};

#pragma omp task priority(1) in(*obsv) out(*outFGMap, *outEdgeMap) inout (*cameras) 
void getObservationTaskWrapper(vector<Observation> *obsv, int timeval, vector<BinaryImage> *outFGMap, vector<FlexImage8u> *outEdgeMap, int *cameras);

#endif
