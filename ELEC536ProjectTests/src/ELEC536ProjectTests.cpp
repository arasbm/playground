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

void automaticThreshold(Mat* result);
void opencvConnectedComponent(Mat* src, Mat* dst);
void depthFromDiffusion(Mat* src, Mat* dst, int size);
void initiateCamera();
void startVideo();
Mat grabImage();
IplImage* convertImageToOpenCV(Image* pImage);

//Global Settings
int lower_threshold = 40; //global threshold which can be changed using up and down arrow keys
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
	cout << "Starting ELEC536 Assignment 2 Application" << endl;
	cvNamedWindow( "ELEC536_Project", CV_WINDOW_AUTOSIZE );
	//Mat result; //working image for most parts
	//Mat tmp;
	Mat colorImg; //color image for connected component labeling

	startVideo();

	cvDestroyWindow( "ELEC536_Project" );
	cout << "Done." << endl;
	return 0;
}

/**
 * My own implementation of iterative optimal threshold selection
 * an automatic thresholding algorithm
 * */
void automaticThreshold(Mat* result) {
	cout << "Automatic threshold selection ..." << endl;
	int estimated_threshold = 100; //Initial estimate
	int selected_threshold = 0;
	int iter_count = 1;
	while (selected_threshold != estimated_threshold) {
		int sum1 = 0, count1 = 0, sum2 = 0, count2 = 0;
		for(int i = 0; i < result->rows; i++) {
			uchar* rowPtr = result->ptr<uchar>(i);
			for(int j=0; j < result->cols; j++) {
				if (rowPtr[j] < estimated_threshold) {
					sum1 += rowPtr[j];
					count1++;
				} else {
					sum2 += rowPtr[j];
					count2++;
				}
			}
		}
		int mu1 = sum1 / count1; //Average of group 1
		int mu2 = sum2 / count2; //Average of group 2
		selected_threshold = estimated_threshold;
		estimated_threshold = 0.5 * (mu1 + mu2);
		cout << iter_count << " - Previous T: [" << selected_threshold << "] Next T: [";
		cout << estimated_threshold << "]" << endl;
		iter_count++;
	}
	cout << "Selected threshold: [[ " << selected_threshold << " ]]" << endl;

	lower_threshold = selected_threshold;

	//convert the image to binary using this selected threshold value
	//manualThreshold(result, selected_threshold);
	threshold(*result, *result, lower_threshold, 255, THRESH_BINARY );
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

		//Move the window one pixel at a time through the image and set pixel value
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
					//window is eithr blury or very smooth
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

void startVideo(){
	VideoCapture video("/home/zooby/Desktop/grab_and_release_data/Grab_Release_Mar15.avi");
	double fps = 30;
	int totalFrames = 0;

	//goodFeaturesToTrack values
	vector<Point2f> corners;
	int maxCorners = 10;
	double qualityLevel = 0.01;
	double minDistance = 24;
	int blockSize = 30;
	bool useHarrisDetector = false; //its either harris or cornerMinEigenVal

	//Try the camera
	initiateCamera();

	//If camera is disconnected fall back on video data
	if(!use_pgr_camera){
		if(!video.isOpened()) { // check if we succeeded
			cout << "Failed to open video file" << endl;
			return;
		}

		//*** just testing TODO
//		Mat edges;
//		namedWindow("edges",1);
//		for(;;)
//		{
//			Mat frame;
//			video >> frame; // get a new frame from camera
//			cvtColor(frame, edges, CV_BGR2GRAY);
//			GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
//			Canny(edges, edges, 0, 30, 3);
//			imshow("edges", edges);
//			if(waitKey(30) >= 0) break;
//		}
		//*** end of testing
	}

	if(save_video){
		//TODO: save video
	}

	int key;
	Mat previousFrame;
	Mat currentFrame;
	Mat tmp;

	for(int frame = 0; frame < 100 || use_pgr_camera; frame++) {
		if(use_pgr_camera){
			currentFrame = grabImage();
		} else{
			video >> currentFrame;
		}

		key = cvWaitKey(10);

		switch(key) {
			case '1':
				break;
			case '2':
				break;
			default:
				break;
		}

		// previousFrame = currentFrame.clone();
        // cvtColor(currentFrame, currentFrame, CV_RGB2GRAY);
		medianBlur(currentFrame, currentFrame, 5);
		GaussianBlur(currentFrame, currentFrame, Size(7,7), 1.5, 1.5);
		threshold(currentFrame, currentFrame, lower_threshold, 255, THRESH_TOZERO);
//		tmp = currentFrame.clone();
//		//depthFromDiffusion(&tmp, &currentFrame, 3);
		goodFeaturesToTrack( currentFrame, corners, maxCorners, qualityLevel, minDistance, currentFrame, blockSize, useHarrisDetector);
//
//		//Draw squares where features are detected
//		for(int i = 0; i < maxCorners; i++) {
//			rectangle(currentFrame, Point(corners[i].x - blockSize/2, corners[i].y - blockSize/2),
//					Point(corners[i].x + blockSize/2, corners[i].y + blockSize/2),Scalar(200,200,200));
//		}

		if (display_video) {
			imshow("ELEC536_Project", currentFrame);
		}
	}

	previousFrame.release();
	currentFrame.release();
	tmp.release();
	if(use_pgr_camera){
		pgrCam.StopCapture();
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
