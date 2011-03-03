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
void medianFilter1D(Mat* result);
void connectedComponentLabeling(Mat* result);


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

	char key = 'a';
	while (key != 'q') {
		switch (key) {
			case 'a':
				cout << "Question 1 part a: subtract background and use manual threshold" <<endl;
				result = (img1 - img2) + (img2 - img1);
				putText(result, "Differencing the two images.", cvPoint(30,30), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
				imshow("ELEC536_Assignment2", result);
				cvWaitKey();
				manualThreshold(&result, threshold_value);
				imshow("ELEC536_Assignment2", result);
				cvWaitKey();
				break;

			case 'b':
				cout << "Question 1 part b: Automatic threshold selection" <<endl;
				result = (img1 - img2) + (img2 - img1);
				automaticThreshold(&result);
				imshow("ELEC536_Assignment2", result);
				break;

			case 'c':
				cout << "Question 1 part c: median filtering" <<endl;
				//TODO
				break;

			case 'd':
				cout << "Question 2 part a: Connected Component Labeling" <<endl;
				//TODO
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
				cout << "The following keys are recognized:" << endl;
				cout << " _______________________ " << endl;
				cout << "| Key\t | question\t|" << endl;
				cout << "| a \t | 1a \t|" << endl;
				cout << "| b \t | 1b \t|" << endl;
				cout << "| c \t | 1c \t|" << endl;
				cout << "| d \t | 2a \t|" << endl;
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
	cout << "Selected threshold: [[ " << selected_threshold << " ]]" <<endl;

	//convert the image to binary using this threshold
	manualThreshold(result, selected_threshold);
}

/**
 * Q1c. My implementation of a simple 1D median filtering to remove noise
 * the window looks like this: [][x][]
 * */
void medianFilter1D(Mat* result) {

}

/**
 * Q2a. Connected Component Labeling
 * */
void connectedComponentLabeling(Mat* result) {

}
