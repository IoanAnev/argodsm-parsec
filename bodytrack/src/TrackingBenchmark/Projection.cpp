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

#include "config.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sys/types.h>
// #include <sys/time.h>
#include "system.h"
#include "Projection.h"

//templated conversion to string with field width
template<class T>
inline string str(T n, int width = 0, char pad = '0'){
	stringstream ss;
	ss << setw(width) << setfill(pad) << n;
	return ss.str();
}

Projection::Projection(string path, int cameras) :
	ImageFiles(cameras)
{
	mPath = path;
	mNCameras = cameras;

	ImageFiles[0] = path + "CAM" + str(1) + DIR_SEPARATOR + "image" + str(0, 4) + ".bmp";

	FlexImage<Im8u,1> im;
	FlexLoadBMP(ImageFiles[0].c_str(), im);
	w = im.Width() / 2;
	h = im.Height() / 2;
	levels = (cameras - 1) / 2 + 1;
	cout<< "width " << w << ", height " << h << ", levels " << levels<<endl;
}

#pragma omp task priority(1) in(*pj, *projection, *result) 
void createBMPPartTaskWrapper(Projection *pj, int camera, MultiCameraProjectedBody *projection, FlexImage<Im8u,3> *result){
    pj->createBMPPart( camera, *projection, *result);
}

void Projection::createBMPPart(int camera, MultiCameraProjectedBody &mProjection, FlexImage<Im8u,3> &result){
    Im8u yellow[3] = {0, 255, 255}, cyan[3] = {255, 255, 0}, magenta[3] = {255, 0, 255};
    //create new image for each camera view
      //load raw image from file
    FlexLoadBMP(ImageFiles[camera].c_str(), srcImage[camera]);
    //downsample image by factor of 2
    FlexDownSample2(srcImage[camera], imds[camera]);
    //create sub-image in result image
    dstImage[camera] = result((camera % 2) * w , int(camera / 2) * h, w, h);
    //convert downsampled image to RGB into sub-image of result
    FlexGrayToRGB(imds[camera], dstImage[camera], false);
    ProjectedBody &pb = mProjection(camera);
    //draw projected cylinders colored by body part
    for(int i = 0; i < pb.Size(); i++) {
        ProjectedCylinder &c = pb(i);
        Im8u *color = cyan;
        if(i == 7 || i == 8 || i == 3 || i == 4)
          color = yellow;
        else if(i == 9 || i == 0)
          color = magenta;

        FlexLine(dstImage[camera], (int)c.mPts[0].x / 2, (int)c.mPts[0].y / 2, (int)c.mPts[1].x / 2, (int)c.mPts[1].y / 2, color);
        FlexLine(dstImage[camera], (int)c.mPts[1].x / 2, (int)c.mPts[1].y / 2, (int)c.mPts[2].x / 2, (int)c.mPts[2].y / 2, color);
        FlexLine(dstImage[camera], (int)c.mPts[2].x / 2, (int)c.mPts[2].y / 2, (int)c.mPts[3].x / 2, (int)c.mPts[3].y / 2, color);
        FlexLine(dstImage[camera], (int)c.mPts[3].x / 2, (int)c.mPts[3].y / 2, (int)c.mPts[0].x / 2, (int)c.mPts[0].y / 2, color);
    }
}

void createBMPTaskWrapper(Projection *pj, MultiCameraProjectedBody *mProjection, int frame){
    pj->createBMP(*mProjection, frame);
}

void Projection::createBMP(MultiCameraProjectedBody &mProjection, int frame){
	//create result image and sub-image object
	FlexImage<Im8u,3> result(w * 2, h * levels);

	for(int i = 0; i < mNCameras; i++)
		ImageFiles[i] = mPath + "CAM" + str(i + 1) + DIR_SEPARATOR + "image" + str(frame, 4) + ".bmp";

	//create new image for each camera view
	for(int camera = 0; camera < (int)ImageFiles.size(); camera++){
      createBMPPartTaskWrapper(this, camera, &mProjection, &result);
	}
   #pragma omp taskwait
	string outFname = mPath + "Result" + str(frame, 4) + ".bmp";
	FlexSaveBMP(outFname.c_str(), result);
}
