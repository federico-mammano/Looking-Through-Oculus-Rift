#include "stdafx.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>

using namespace cv;
using namespace std;

int iMinY = 100;
int iMaxY = 255;
int iMinCr = 160;
int iMaxCr = 190;
int iMinCb = 70;
int iMaxCb = 100;

void Skin2GrayMask(Mat &BGRFrame, Mat &SkinMask)
{
	Mat FrameYCrCb;
	cvtColor(BGRFrame, FrameYCrCb, CV_BGR2YCrCb);

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

			y = (y >= iMinY && y <= iMaxY) ? (unsigned char)255 : (unsigned char)0;
			cr = (cr >= iMinCr && cr <= iMaxCr) ? (unsigned char)255 : (unsigned char)0;
			cb = (cb >= iMinCb && cb <= iMaxCb) ? (unsigned char)255 : (unsigned char)0;

			pGrayMaskData[i*cols + j] = ((y + cr + cb) > 255 ? (unsigned char)255 : (unsigned char)0);
		}
	}

	int size = 3;
	Mat element = getStructuringElement(MORPH_RECT, Size(2 * size, 2 * size), Point(size, size));
	erode(SkinMask, SkinMask, element);
	dilate(SkinMask, SkinMask, element);
}

int main(int argc, char* argv[])
{
	VideoCapture cap(0);																				// Open the video camera number 0
	if (!cap.isOpened())
	{
		cout << "Cannot open the video file" << endl;
		return -1;
	}

	double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double dHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	namedWindow("MyVideo", CV_WINDOW_AUTOSIZE);
	namedWindow("SkinMask", CV_WINDOW_AUTOSIZE);
	createTrackbar("Min Y", "SkinMask", &iMinY, 255);
	createTrackbar("Max Y", "SkinMask", &iMaxY, 255);
	createTrackbar("Min Cr", "SkinMask", &iMinCr, 255);
	createTrackbar("Max Cr", "SkinMask", &iMaxCr, 255);
	createTrackbar("Min Cb", "SkinMask", &iMinCb, 255);
	createTrackbar("Max Cb", "SkinMask", &iMaxCb, 255);

	while (1)
	{
		Mat Frame;
		bool bSuccess = cap.read(Frame);
		if (!bSuccess)
		{
			cout << "Cannot read a Frame from video file" << endl;
			break;
		}

		Mat GrayMask((int)dHeight, (int)dWidth, CV_8UC1);
		Skin2GrayMask(Frame, GrayMask);

		imshow("SkinMask", GrayMask);
		imshow("MyVideo", Frame);

		if (waitKey(30) == 27) { break; }
	}
	return 0;
}



