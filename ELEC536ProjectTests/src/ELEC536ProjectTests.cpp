/****************************************************
* Name        : ELEC536Assignment2.cpp
* Author      : Aras Balali Moghaddam
* Version     : 0.1
*****************************************************/
#include <iostream>

//OpenCV include files.
#include "highgui.h"
#include "cv.h"
#include "cxcore.h"
#include "ml.h"
#include "cxtypes.h"

//Flycapture SDK
#include "FlyCapture2.h"

#include <cmath>

using namespace std;
using namespace cv;
using namespace FlyCapture2;

void process(Mat* img);
void opencvConnectedComponent(Mat* src, Mat* dst);
void depthFromDiffusion(Mat* src, Mat* dst, int size);
void initiateCamera();
void startVideo();
Mat grabImage();
IplImage* convertImageToOpenCV(Image* pImage);

//Global Settings
int lower_threshold = 20; //global threshold which can be changed using up and down arrow keys
int upper_threshold = 200;
bool save_video = false;
bool display_video = true;
bool subtract_background = false;
bool use_pgr_camera = true;

//PGR variables
Camera pgrCam;
PGRGuid guid;

//OpenCV variables
CvPoint mouseLocation;

int main(int argc, char* argv[]) {
	cout << "Starting ELEC536 Grab and Release Project" << endl;
	cvNamedWindow( "Source", CV_WINDOW_AUTOSIZE ); 		//monochrome source
	cvNamedWindow( "Processed", CV_WINDOW_AUTOSIZE ); 	//monochrome image after pre-processing
	cvNamedWindow( "Tracked", CV_WINDOW_AUTOSIZE ); 	//Colour with pretty drawings showing tracking results

	//Mat result; //working image for most parts
	//Mat tmp;
	Mat colorImg; //color image for connected component labeling

	startVideo();

	cvDestroyWindow( "Source" );
	cvDestroyWindow( "Processed" );
	cvDestroyWindow( "Tracked" );
	cout << "Exiting ELEC536 Grab and Release Project." << endl;
	return 0;
}

/**
 * An implementation of connected components labeling using
 * opencv findContour function
 * */
void opencvConnectedComponent(Mat* src, Mat* dst) {
	vector<vector<cv::Point> > contours;
	vector<Vec4i> hiearchy;

	//threshold(*src, *src, lower_threshold, 255, THRESH_BINARY );
	findContours(*src, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	int index = 0;
	for (; index >= 0; index = hiearchy[index][0]) {
		Scalar color = CV_RGB(rand()&255, rand()&255, rand()&255 );
		drawContours(*dst, contours, index, color, 3, CV_FILLED, hiearchy, std::abs(index));
		//drawContours(*dst, contours, index, color, CV_FILLED, 8, hiearchy);
	}
}

/**
 * Precondition: size > 1 and it is an odd number
 */
void depthFromDiffusion(Mat* src, Mat* dst, int size) {
		int border = round(size / 2);
		Rect rect = Rect(0, 0, size, size);
		Mat window;
		//Mat sorted_window;

		// Move the window one pixel at a time through the image and set pixel value
		// in dst to the standard deviation of values in current window
		for(int i = border; i < src->rows - border; i++) {
			for(int j = border; j < src->cols - border; j++) {
				window = Mat(*src, rect);
				Scalar mean;
				Scalar stdDev;
				cv::meanStdDev(window, mean, stdDev);
				if (src->at<uchar>(i,j) < lower_threshold) {
					dst->at<uchar>(i,j) = 0;
				} else {
					//window is either blury or very smooth
					if (src->at<uchar>(i,j) > upper_threshold) {
						dst->at<uchar>(i,j) = src->at<uchar>(i,j);
					} else {
						dst->at<uchar>(i,j) = (stdDev.val[0] / pow(size, 2)) * 191 + src->at<uchar>(i,j) / 4;
					}
				}

				rect.x += 1; //shift window to right
			}
			//Move window down one row and back to position 0
			rect.y += 1;
			rect.x = 0;
		}

}

void initiateCamera(){
	//Starts the camera.
	BusManager busManager;
	unsigned int totalCameras;
	busManager.GetNumOfCameras(&totalCameras);
	printf("Found %d cameras on the bus.\n",totalCameras);
	busManager.GetCameraFromIndex(0, &guid);
	Error pgError;
	pgError = pgrCam.Connect(&guid);
	if (pgError != PGRERROR_OK){
		printf("Error in starting the camera.\n");
		use_pgr_camera = false;
		return;
	}
	pgrCam.StartCapture();
	if (pgError != PGRERROR_OK){
		printf("Error in starting the camera capture.\n");
		use_pgr_camera = false;
		return;
	}
	cout << "pgr camera initialized." << endl;
}

/**
 * Do all the pre-processing before tracking the image
 * */
void process(Mat img) {
	// cvtColor(currentFrame, currentFrame, CV_RGB2GRAY);
	medianBlur(img, img, 3);
	GaussianBlur(img, img, Size(3,3), 1.5, 1.5);
	//
	//threshold(img, img, lower_threshold, 255, THRESH_TOZERO);
	//depthFromDiffusion(&tmp, &previousFrame, 5);
}

void startVideo(){
	VideoCapture video("/home/zooby/Desktop/grab_and_release_data/Grab_Release_Mar15.avi");
	double fps = 30;
	int totalFrames = 0;

	//goodFeaturesToTrack structure and settings
	vector<Point2f> previousCorners;
	vector<Point2f> currentCorners;
	vector<uchar> flowStatus;
	vector<float> flowError;
	TermCriteria termCriteria = TermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 );
	double derivLambda = 0.1; //proportion for impact of "image intensity" as opposed to "derivatives"
	int maxCorners = 16;
	double qualityLevel = 0.01;
	double minDistance = 20;
	int blockSize = 30;
	bool useHarrisDetector = false; //its either harris or cornerMinEigenVal

	//Contour detection structures
	vector<vector<cv::Point> > contours;
	vector<Vec4i> hiearchy;

	//Try the camera
	initiateCamera();

	//If camera is disconnected fall back on video data
	if(!use_pgr_camera){
		if(!video.isOpened()) { // check if we succeeded
			cout << "Failed to open video file" << endl;
			return;
		}
	}

	if(save_video){
		//TODO: save video
	}

	//Preparing for main video loop
	Mat previousFrame;
	Mat currentFrame;
	Mat trackingResults;
	Mat binaryImg; //binary image for finding contours of the hand
	Mat tmpMono;
	Mat tmpColor;

	//Define some nice BGR colors for dark background
	Scalar YELLOW = CV_RGB(255, 255, 51);
	Scalar ORANGE = CV_RGB(255, 153, 51); //use for left hand
	Scalar RED = CV_RGB(255, 51, 51);
	Scalar PINK = CV_RGB(255, 51, 153);
	Scalar GREEN = CV_RGB(153, 255, 51);
	Scalar BLUE = CV_RGB(51, 153, 255); // use for right hand
	Scalar OLIVE = CV_RGB(184, 184, 0);
	Scalar RANDOM_COLOR = CV_RGB(rand()&255, rand()&255, rand()&255 ); //a random color

	char key = 'a';
	int frame_count = 0;
	while(key != 'q') {
		if(use_pgr_camera){
			currentFrame = grabImage();
		} else{
			video >> tmpColor;
			cvtColor(tmpColor, currentFrame, CV_RGB2GRAY);
		}
		//imshow("Source", currentFrame);

		//do all the pre-processing
		process(currentFrame);
		//imshow("Processed", currentFrame);

		//need at least one previous frame to process
		if(frame_count == 0) {
			previousFrame = currentFrame;
		}
		frame_count++;

		key = cvWaitKey(10);
		switch(key) {
			case '1':
				break;
			case '2':
				break;
			case 'a':
				break;
			default:
				break;
		}

		// previousFrame = currentFrame.clone();

		//Find Two Largest Contours TODO: sort and filter out small contours

		binaryImg = currentFrame.clone();
		threshold(binaryImg, binaryImg, lower_threshold, 255, THRESH_BINARY);
		findContours(binaryImg, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		//findContours( binaryImg, contours, RETR_TREE, CV_CHAIN_APPROX_SIMPLE );

		//Canny(previousFrame, previousFrame, 0, 30, 3);
		trackingResults = cvCreateMat(currentFrame.rows, currentFrame.cols, CV_8UC3 );
		cvtColor(currentFrame, trackingResults, CV_GRAY2BGR);

		goodFeaturesToTrack(previousFrame, previousCorners, maxCorners, qualityLevel, minDistance, previousFrame, blockSize, useHarrisDetector);
//
		calcOpticalFlowPyrLK(previousFrame, currentFrame, previousCorners, currentCorners, flowStatus, flowError, Size(blockSize, blockSize), 4, termCriteria, derivLambda, OPTFLOW_FARNEBACK_GAUSSIAN);
		//Draw squares where features are detected
		for(int i = 0; i < maxCorners; i++) {
			rectangle(trackingResults, Point(previousCorners[i].x - blockSize/2, previousCorners[i].y - blockSize/2), Point(previousCorners[i].x + blockSize/2, previousCorners[i].y + blockSize/2), PINK);
			if((uchar)flowStatus[i] == 1) {
				line(trackingResults, previousCorners[i], currentCorners[i], GREEN, 2, 8, 0);
			}
		}

		//Draw contours if there are any
		//if (hiearchy.size() > 0) {
			double max1Index = 0; //biggest blob
			double max1Area = 0;
			double max2Index = 0; //second biggest blob
			double max2Area = 0;
			//for (int i = 0; i < contours.size(); i++) {
//				Moment moment = moment(contour[i]);
//				if(contourArea(contours[i]) > max2Area) {
//					max2Area = contourArea(contours[i]);
//					max2Index = i;
//				} else if (contourArea(contours[i]) > max1Area) {
//					max1Area = contourArea();
//					max1Index = i;
//				}
//			if (contours.size() > 2) {
//				if (contours[0].size() > 1) {
//					drawContours(trackingResults, contours, 0, ORANGE, CV_FILLED);
//				}
//			}

//			}
//			if (contours.size() > 0) {
//				int index = 0;
//				for (; index >= 0; index = hiearchy[index][0]) {
//					drawContours(trackingResults, contours, index, ORANGE, 1, 4, hiearchy, 0);
//				}
//			}


			//Find two largest enclosing circles (hopefully the two hands)
			Point2f tmpCenter, max1Center, max2Center;
			float tmpRadius = 0, max1Radius = 0, max2Radius = 0;
			for (uint i = 0; i < contours.size(); i++) {
				if(contours[i].size() > 0) {
					minEnclosingCircle(Mat(contours[i]), tmpCenter, tmpRadius);
					if (tmpRadius > max1Radius) {
						if (max1Radius > max2Radius) {
							max2Radius = max1Radius;
							max2Center = max1Center;
						}
						max1Radius = tmpRadius;
						max1Center = tmpCenter;
					} else if (tmpRadius > max2Radius) {
						//max1Radius is bigger than max2Radius
						max2Radius = tmpRadius;
						max2Center = tmpCenter;
					}
				}
			}

			//draw the two largest circles if it exit
			if(max1Radius > 0) {
				if(max1Center.x > max2Center.x) {
					//max1 is on the right
					circle(trackingResults, max1Center, max1Radius, BLUE, 2, 4);
					circle(trackingResults, max1Center, 4, BLUE, 2, 4);
					if(max2Radius > 0) {
						circle(trackingResults, max2Center, max2Radius, ORANGE, 2, 4);
						circle(trackingResults, max2Center, 4, ORANGE, 2, 4);
					}
				} else {
					//max1 is on the left
					circle(trackingResults, max1Center, max1Radius, ORANGE, 2, 4);
					circle(trackingResults, max1Center, 4, ORANGE, 2, 4);
					if(max2Radius > 0) {
						circle(trackingResults, max2Center, max2Radius, BLUE, 2, 4);
						circle(trackingResults, max2Center, 4, BLUE, 2, 4);

					}
				}
			}

		//}

		imshow("Tracked", trackingResults);

		previousFrame = currentFrame;
		currentCorners = previousCorners;

	}

	//Clean up before leaving
	previousFrame.release();
	currentFrame.release();
	trackingResults.release();
	tmpColor.release();
	tmpMono.release();
	if(use_pgr_camera){
		pgrCam.StopCapture();
		pgrCam.Disconnect();
	}
}

Mat grabImage(){
	Error pgError;
	Image rawImage;
	pgError = pgrCam.RetrieveBuffer(&rawImage);
	if (pgError != PGRERROR_OK){
		cout << "Error in grabbing frame." << endl;
	}
	Mat image(convertImageToOpenCV(&rawImage));
	return image;
}

IplImage* convertImageToOpenCV(Image* pImage){
	IplImage* cvImage = NULL;
	bool bColor = true;
	CvSize mySize;
	mySize.height = pImage->GetRows();
	mySize.width = pImage->GetCols();
	//cout << "Pixel format is " << pImage->GetPixelFormat() << endl;

	switch ( pImage->GetPixelFormat() )
	{
		case PIXEL_FORMAT_MONO8:
			cvImage = cvCreateImageHeader(mySize, 8, 1 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 1;
			bColor = false;
			break;
		case PIXEL_FORMAT_411YUV8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_422YUV8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			//printf("Inside switch.  Depth is %d\n",cvImage->depth);
			break;
		case PIXEL_FORMAT_444YUV8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_RGB8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_MONO16:
			cvImage = cvCreateImageHeader(mySize, 16, 1 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 1;
			bColor = false;
			break;
		case PIXEL_FORMAT_RGB16:
			cvImage = cvCreateImageHeader(mySize, 16, 3 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_S_MONO16:
			cvImage = cvCreateImageHeader(mySize, 16, 1 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 1;
			bColor = false;
			break;
		case PIXEL_FORMAT_S_RGB16:
			cvImage = cvCreateImageHeader(mySize, 16, 3 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_RAW8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_RAW16:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_MONO12:
			//"Image format is not supported by OpenCV"
			bColor = false;
			break;
		case PIXEL_FORMAT_RAW12:
			//"Image format is not supported by OpenCV
			break;
		case PIXEL_FORMAT_BGR:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_BGRU:
			cvImage = cvCreateImageHeader(mySize, 8, 4 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 4;
			break;
		case PIXEL_FORMAT_RGBU:
			cvImage = cvCreateImageHeader(mySize, 8, 4 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 4;
			break;
		default:
			cout << "ERROR in detecting image format" << endl;
			break;
	}

	if(bColor)
	{
		Image colorImage; //new image to be referenced by cvImage
		colorImage.SetData(new unsigned char[pImage->GetCols() * pImage->GetRows()*3], pImage->GetCols() * pImage->GetRows() * 3);
		pImage->Convert(PIXEL_FORMAT_BGR, &colorImage); //needs to be as BGR to be saved
		cvImage->width = colorImage.GetCols();
		cvImage->height = colorImage.GetRows();
		cvImage->widthStep = colorImage.GetStride();
		cvImage->origin = 0; //interleaved color channels
		cvImage->imageDataOrigin = (char*)colorImage.GetData(); //DataOrigin and Data same pointer, no ROI
		cvImage->imageData         = (char*)(colorImage.GetData());
		cvImage->widthStep              = colorImage.GetStride();
		cvImage->nSize = sizeof (IplImage);
		cvImage->imageSize = cvImage->height * cvImage->widthStep;
		//printf("Inside if.  width is %d, height is %d\n",cvImage->width,cvImage->height);
	}
	else
	{
		cvImage->imageDataOrigin = (char*)(pImage->GetData());
		cvImage->imageData         = (char*)(pImage->GetData());
		cvImage->widthStep         = pImage->GetStride();
		cvImage->nSize             = sizeof (IplImage);
		cvImage->imageSize         = cvImage->height * cvImage->widthStep;
		//at this point cvImage contains a valid IplImage
	}

	return cvImage;
}
