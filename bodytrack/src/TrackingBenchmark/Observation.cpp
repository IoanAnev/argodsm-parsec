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
#include <vector>
#include <iomanip>
#include <sys/types.h>
#include <sys/time.h>
#include "Observation.h"
#include "CovarianceMatrix.h"
#include "FlexImageLib/FlexLib.h"
#include "system.h"

using namespace std;

//templated conversion to string with field width
template<class T>
inline string str(T n, int width = 0, char pad = '0'){
    stringstream ss;
    ss << setw(width) << setfill(pad) << n;
    return ss.str();
}

Observation::Observation(string path, int camera){
    mPath = path;
    mCameraID = camera+1;
};

//------------------------ Observation processing -----------------------------

//Separable 7x7 gaussian filter
inline void GaussianBlur(FlexImage8u &src, FlexImage8u &dst)
{
    float k[] = {0.12149085090552f, 0.14203719483447f, 0.15599734045770f, 0.16094922760463f, 0.15599734045770f, 0.14203719483447f, 0.12149085090552f};
    FlexImage8u tmp;
    FlexFilterRowV(src, tmp, k, 7);											//separable gaussian convolution using kernel k
    FlexFilterColumnV(tmp, dst, k, 7);
}

//Calculate gradient magnitude and threshold to binarize
inline FlexImage8u GradientMagThreshold(const FlexImage8u &src, float threshold){
    FlexImage8u r(src.Size());
    ZeroBorder(r);
    //for each pixel
    for(int y = 1; y < src.Height() - 1; y++) {
        Im8u *p = &src(1,y), *ph = &src(1,y - 1), *pl = &src(1,y + 1), *pr = &r(1,y);
        for(int x = 1; x < src.Width() - 1; x++) {
            float xg = -0.125f * ph[-1] + 0.125f * ph[1] - 0.250f * p[-1] + 0.250f * p[1] - 0.125f * pl[-1] + 0.125f * pl[1];	//calc x and y gradients
            float yg = -0.125f * ph[-1] - 0.250f * ph[0] - 0.125f * ph[1] + 0.125f * pl[-1] + 0.250f * pl[0] + 0.125f * pl[1];
            float mag = xg * xg + yg * yg;					//calc magnitude and threshold
            *pr = (mag < threshold) ? 0 : 255;
            p++; ph++; pl++; pr++;
        }
    }
    return r;
}

//Generate an edge map from the original camera image
void Observation::CreateEdgeMap(const FlexImage8u &src, FlexImage8u &dst)
{
    FlexImage8u gr = GradientMagThreshold(src, 16.0f);		//calc gradient magnitude and threshold
    GaussianBlur(gr, dst);									//Blur to create distance error map
}

//Can execute in a slow processor, wont make any difference
#pragma omp task priority(1) in(*obs) out(*outFGMap) 
void getObservationFGTaskWrapper(Observation* obs, int timeval, BinaryImage *outFGMap){
    if (!obs->GetObservationFG((float) timeval, *outFGMap)){
      cout << "Error loading observation data" << endl;
      exit(-1);
    }
}

#pragma omp task priority(0) in(*obs) out(*outEdgeMap) 
void getObservationEdgeTaskWrapper(Observation* obs, int timeval, FlexImage8u *outEdgeMap){
    if (!obs->GetObservationEdge((float) timeval, *outEdgeMap)){
      cout << "Error loading observation data" << endl;
      exit(-1);
    }
}

void getObservationTaskWrapper(vector<Observation> *obsv, int timeval, vector<BinaryImage> *outFGMap, vector<FlexImage8u> *outEdgeMap, int *cameras){
    for (int j=0; j<*cameras; j++){
      getObservationFGTaskWrapper(&(*obsv)[j], timeval, &(*outFGMap)[j]);
      getObservationEdgeTaskWrapper(&(*obsv)[j], timeval, &(*outEdgeMap)[j]);
    }
    #pragma omp taskwait
}

//load and process all images for new observation at a given time(frame)
bool Observation::GetObservationFG(float timeval, BinaryImage &outFGMap)
{
    int frame = (float) timeval;								//generate image filenames
    string FGfile = mPath + "FG" + str(mCameraID) + DIR_SEPARATOR + "image" + str(frame, 4) + ".bmp";
    string ImageFile = mPath + "CAM" + str(mCameraID) + DIR_SEPARATOR + "image" + str(frame, 4) + ".bmp";

    FlexImage8u im;

    //Load foreground maps and raw images
    if(!FlexLoadBMP(FGfile.c_str(), im)){
        cout << "Unable to load image: " << FGfile.c_str() << endl;
        return false;
    }

    //binarize foreground maps to 0 and 1
    mFGMap.ConvertToBinary(im);
    outFGMap= mFGMap;

    return true;
}

//load and process all images for new observation at a given time(frame)
bool Observation::GetObservationEdge(float timeval, FlexImage8u &outEdgeMap)
{
    int frame = (float) timeval;								//generate image filenames
    string FGfile = mPath + "FG" + str(mCameraID) + DIR_SEPARATOR + "image" + str(frame, 4) + ".bmp";
    string ImageFile = mPath + "CAM" + str(mCameraID) + DIR_SEPARATOR + "image" + str(frame, 4) + ".bmp";

    FlexImage8u im;

    if(!FlexLoadBMP(ImageFile.c_str(), im)) {
        cout << "Unable to load image: " << ImageFile.c_str() << endl;
        return false;
    }
    //Create edge maps
    CreateEdgeMap(im, mEdgeMap);
    outEdgeMap = mEdgeMap;

    return true;
}
