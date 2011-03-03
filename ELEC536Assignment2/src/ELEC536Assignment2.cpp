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

#include <cmath>

using namespace std;
using namespace cv;

void findMissingObject(Mat* img1, Mat* img2, Mat* result);
void manualThreshold(Mat* result, int threshold);
void automaticThreshold(Mat* result);

//Define some colors for drawing and writing on image
CvScalar GREEN = cvScalar(20,150,20);
CvScalar RED = cvScalar(150,20,20);

int main(int argc, char* argv[]) {
	cout << "Starting ELEC536 Assignment 2 Application" << endl;
	cvNamedWindow( "ELEC536_Assignment2", CV_WINDOW_AUTOSIZE );

	Mat img1 = imread("img/stuff.pgm", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img2 = imread("img/stufmin1.pgm", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img3 = imread("img/testimage.pgm", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img4 = imread("img/testimagemin1.pgm", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img5 = imread("img/things.pgm", CV_LOAD_IMAGE_GRAYSCALE);
	Mat result;

	int threshold_value = 30; //global threshold which can be changed using up and down arrow keys

	CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0, 1);

	char key = 'a';
	while (key != 'q') {
		switch (key) {
			case 'a':
				//findMissingObject(&img1, &img2, &result);
				result = (img1 - img2) + (img2 - img1);
				putText(result, "Differencing the two images.", cvPoint(30,30), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
				imshow("ELEC536_Assignment2", result);
				cvWaitKey();
				manualThreshold(&result, threshold_value);
				imshow("ELEC536_Assignment2", result);
				cvWaitKey();
				break;

			case 'b':
				break;

			case 82:
				//Up arrow key
				threshold_value++;
				cout << "Increase threshold to: " << threshold_value << endl;
				break;

			case 84:
				//Down arrow key
				threshold_value--;
				cout << "Decrease threshold to: " << threshold_value << endl;
				break;

			default:
				cout << "The following keys are recognised:" << endl;
		}
		key = cvWaitKey(0);
		cout << "KEY CODE: " << (int)key << endl;
	}

	//cleanup and leave
	//releaseMat( &result );
	//cvReleaseMat( &img1 );
	//cvReleaseMat( &img2 );
	cvDestroyWindow( "ELEC536_Assignment2" );
	cout << "Done." << endl;
	return 0;
}

/**
 * This function expects two similar images with one foreground object missing from one of the images.
 * the function attempts to isolate the missing object by subtracting the two images from each other
 * */
void findMissingObject(Mat* img1, Mat* img2, Mat* result) {
	//result = (img1->Mat() - img2->Mat()) + (img2->Mat() - img1->Mat());
}

/**
 * This function iterates through the image and convert it to a
 * binary image based on the value of the threshold
 * */
void manualThreshold(Mat* result, int threshold) {
	for(int i = 0; i < result->rows; i++) {
		uchar* rowPtr = result->ptr<uchar>(i);
		for(int j=0; j < result->cols; j++) {
			rowPtr[j] = (rowPtr[j] < threshold) ? 0 : 255;
		}
	}
}

/**
 * My own implementation of an automatic thresholding algorithm
 * */
void automaticThreshold(Mat* result) {

}
