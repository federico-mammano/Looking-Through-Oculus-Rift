/************************************************************************************
Filename    :   Hand.h
Content     :   Hand class for Oculus Rift test application
Created     :   December 20th, 2014
Authors     :   Federico Mammano

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************************/

#ifndef HAND_H_
#define HAND_H_

#define HAND_HISTORY_SIZE				10			// Number of chronological hand positions stored in memory. Useful for gesture detection, stabilization, etc ...
#define HAND_MIN_FINGER_DEPTH			10.0f		// Finger depth threshold value. Used in the finger detection algorithm.
#define HAND_GESTURE_NONE				0x00000000
#define HAND_GESTURE_SWIPE_RIGHT		0x00000001
#define HAND_GESTURE_SWIPE_LEFT			0x00000002
#define HAND_GESTURE_SWIPE_UP			0x00000004
#define HAND_GESTURE_SWIPE_DOWN			0x00000008

cv::Scalar MinYCrCb = cv::Scalar(100, 160, 70);		// Minimum YCrCb threshold values to extract the Skin Mask. Use the SkinMaskTool.cpp file to detect your best values.
cv::Scalar MaxYCrCb = cv::Scalar(255, 190, 100);	// Maximum YCrCb threshold values to extract the Skin Mask. Use the SkinMaskTool.cpp file to detect your best values.
													// NOTE: As far as hand is not detected through a depth map but through a color map, light condition could dramatically change 
													// these values. Consider to develop an algorithm to identify these thresholds automatically (e.g.: through a ROI Histogram).

cv::vector<cv::Point> PalmCenters;					// Array of hand center positions. Useful for gesture detection, stabilization, etc ...


// ==================================================================================//
//  Hand Class
// ==================================================================================//
class Hand
{
public:

	cv::RotatedRect				BoundingRect;
	cv::Point					RoughPalmCenter;
	float						fMeanSize;
	cv::vector<cv::Point>		FingerTips;

private:

	cv::vector<cv::Point>		Contour;
	cv::vector<cv::Vec4i>		Defect;

public:

	// Constructor
	Hand() : fMeanSize(1.0f) {}

	// Destructor
	~Hand() {}

	int ExtractContourAndHull(cv::Mat &SkinMask)
	{
		static int iThresh = 100;
		static double dMinArea = 5000;																					// Ignore all small insignificant areas
		static unsigned int uiMinContourSize = 300;
		
		cv::RNG rng;
		cv::Mat OutThreshold;
		cv::vector<cv::vector<cv::Point>> Contours;
		cv::vector<cv::Vec4i> Hierarchy;

		// Detect edges using Threshold
		cv::threshold(SkinMask, OutThreshold, iThresh, 255, cv::THRESH_BINARY);

		// Find Contours
		cv::findContours(OutThreshold, Contours, Hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

		// Find the convex Hull object and the Defects points for each contour
		cv::vector<cv::vector<int>> Hulls(Contours.size());
		cv::vector<cv::vector<cv::Vec4i>> Defects(Contours.size());
		for (unsigned int i = 0; i < Contours.size(); i++)
		{
			if (cv::contourArea(Contours[i]) < dMinArea) { continue; }

			cv::convexHull(cv::Mat(Contours[i]), Hulls[i], false);
			if (Hulls[i].size() > 3) { cv::convexityDefects(Contours[i], Hulls[i], Defects[i]); }
		}

		// Find the biggest contour
		int iBiggestContourID = FindBiggestContour(Contours);
		if (iBiggestContourID == -1) return 0;
		if (Contours[iBiggestContourID].size() < uiMinContourSize ||
			Defects[iBiggestContourID].size() < 3) return 0;

		Contour = Contours[iBiggestContourID];
		Defect = Defects[iBiggestContourID];
		return 1;
	}

	int IdentifyProperties()																							// Identify Hand: bounding rect, palm center and fingers.
	{
		BoundingRect = cv::minAreaRect(cv::Mat(Contour));
		RoughPalmCenter = cv::Point2f(BoundingRect.center.x, BoundingRect.center.y);
		fMeanSize = (BoundingRect.size.width + BoundingRect.size.height)*0.5f;

		// Getting the rough palm center as the average of all defect points
		for (unsigned int j = 0; j < Defect.size(); j++)
		{
			cv::Point Start(Contour[Defect[j][0]]);
			cv::Point End(Contour[Defect[j][1]]);
			cv::Point Far(Contour[Defect[j][2]]);
			RoughPalmCenter += Far + Start + End;
		}
		RoughPalmCenter.x /= (int)Defect.size() * 3;
		RoughPalmCenter.y /= (int)Defect.size() * 3;

		// Detect fingers points
		// NOTE: Play with these thresholds, improve the very basic algorithm and develop your own. 
		//       For instance, look at fold points that form an almost isosceles triangle with certain angles 
		//		(e.g.: angleBetweenFingers >= 60), etc...
		for (unsigned int j = 0; j < Defect.size(); j++)
		{
			float fDepth = (float)Defect[j][3] / 256.0f;
			if (fDepth < HAND_MIN_FINGER_DEPTH) { continue; }

			cv::Point Start(Contour[Defect[j][0]]);
			cv::Point End(Contour[Defect[j][1]]);
			cv::Point Far(Contour[Defect[j][2]]);
			double dDistA	= sqrt(Distance(RoughPalmCenter, Far));
			double dDistB	= sqrt(Distance(RoughPalmCenter, Start));
			double dDistC	= sqrt(Distance(Far, Start));
			double dDistD	= sqrt(Distance(End, Far));
			if (dDistC >= HAND_MIN_FINGER_DEPTH && dDistD >= HAND_MIN_FINGER_DEPTH &&
				std::max(dDistC, dDistD) / std::min(dDistC, dDistD) >= 0.8 && 
				std::min(dDistA, dDistB) / std::max(dDistA, dDistB) <= 0.8)
			{
				FingerTips.push_back(Start);
			}
		}

		//if (FingerTips.size() > 5) return 0;																			// Add your own finger tips robust control algorithm

		PalmCenters.push_back(RoughPalmCenter);																			// Add this center position to the history
		if (PalmCenters.size() > HAND_HISTORY_SIZE) { PalmCenters.erase(PalmCenters.begin()); }

		return 1;
	}

	void BGR2SkinMask(cv::Mat &BGRFrame, cv::Mat &SkinMask)																// Convert the video frame into a YCrCb color space and obtain a Black & White mask to extract the skin
	{
		cv::Mat FrameYCrCb;
		cv::cvtColor(BGRFrame, FrameYCrCb, CV_BGR2YCrCb);

		int y, cr, cb;
		int rows = BGRFrame.rows;
		int cols = BGRFrame.cols;
		int channels = FrameYCrCb.channels();
		unsigned char *pYCrCbData = FrameYCrCb.data;
		unsigned char *pGrayMaskData = SkinMask.data;
		for (int i = 0; i < rows; i++)
		{
			for (int j = 0; j < cols; j++)
			{
				y = pYCrCbData[(i*cols + j)*channels + 0];
				cr = pYCrCbData[(i*cols + j)*channels + 1];
				cb = pYCrCbData[(i*cols + j)*channels + 2];

				y = (y >= MinYCrCb[0] && y <= MaxYCrCb[0]) ? (unsigned char)255 : (unsigned char)0;
				cr = (cr >= MinYCrCb[1] && cr <= MaxYCrCb[1]) ? (unsigned char)255 : (unsigned char)0;
				cb = (cb >= MinYCrCb[2] && cb <= MaxYCrCb[2]) ? (unsigned char)255 : (unsigned char)0;

				pGrayMaskData[i*cols + j] = ((y + cr + cb) > 255 ? (unsigned char)255 : (unsigned char)0);
			}
		}

		int size = 3;
		cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * size, 2 * size), cv::Point(size, size));
		cv::erode(SkinMask, SkinMask, element);
		cv::dilate(SkinMask, SkinMask, element);
	}

	DWORD GestureDetection()																							// Analyze the parameters and identify gestures
	{
		DWORD dwGesture = HAND_GESTURE_NONE;
		int deltaX = PalmCenters[PalmCenters.size()-1].x - PalmCenters[0].x;
		int deltaY = PalmCenters[PalmCenters.size()-1].y - PalmCenters[0].y;

		if (deltaX > (int)fMeanSize)		{ dwGesture |= HAND_GESTURE_SWIPE_RIGHT; }
		else if (deltaX < -(int)fMeanSize)	{ dwGesture |= HAND_GESTURE_SWIPE_LEFT; }

		if (deltaY > (int)fMeanSize)		{ dwGesture |= HAND_GESTURE_SWIPE_DOWN; }
		else if (deltaY < -(int)fMeanSize)	{ dwGesture |= HAND_GESTURE_SWIPE_UP; }

		return dwGesture;
	}

	void Draw(cv::Mat &Frame)
	{
		// Draw the palm center position history
		for (unsigned int i = 0; i<PalmCenters.size(); i++)
		{
			if (PalmCenters.size() >= 2 && i < PalmCenters.size() - 2)
			{
				cv::line(Frame, PalmCenters[i], PalmCenters[i + 1], cv::Scalar(128, 100, 0), 2);
			}
		}

		// Draw the contour
		cv::vector<cv::vector<cv::Point>> ContoursPoly(1);
		approxPolyDP(cv::Mat(Contour), ContoursPoly[0], 3, true);
		drawContours(Frame, ContoursPoly, 0, cv::Scalar(128, 128, 128), 2, 8, cv::vector<cv::Vec4i>(), 0, cv::Point());

		// Draw the palm center and fingers
		float fRelativeUnity = fMeanSize*0.007f;
		cv::circle(Frame, RoughPalmCenter, (int)(12 * fRelativeUnity), cv::Scalar(255, 200, 0), 6);
		for (unsigned int i = 0; i<FingerTips.size(); i++)
		{
			cv::circle(Frame, FingerTips[i], (int)(5 * fRelativeUnity), cv::Scalar(255, 200, 0), (int)(3 * fRelativeUnity));
			cv::circle(Frame, FingerTips[i], (int)(10 * fRelativeUnity), cv::Scalar(255, 200, 0), (int)fRelativeUnity);
			cv::line(Frame, FingerTips[i], RoughPalmCenter, cv::Scalar(255, 200, 0), 2);
		}
	}

	int FindBiggestContour(cv::vector<cv::vector<cv::Point>> Contours)
	{
		int iBiggestContourID = -1;
		double dAreaOfBiggestContour = 0;

		for (unsigned int i = 0; i<Contours.size(); i++)
		{
			double areaOfContour = contourArea(Contours[i]);
			if (areaOfContour > dAreaOfBiggestContour)
			{
				dAreaOfBiggestContour = areaOfContour;
				iBiggestContourID = i;
			}
		}
		return iBiggestContourID;
	}

	double Distance(cv::Point x, cv::Point y)
	{
		return (x.x - y.x)*(x.x - y.x) + (x.y - y.y)*(x.y - y.y);
	}
};

#endif // HAND_H_