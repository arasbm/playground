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
void connectedComponentLabeling(Mat* src, Mat* dst);
void opencvConnectedComponent(Mat* src, Mat* dst);
void depthFromDiffusion(Mat* src, Mat* dst, int size);


//Define some colors for drawing and writing on image
CvScalar GREEN = cvScalar(20,150,20);
CvScalar RED = cvScalar(150,20,20);

int lower_threshold = 40; //global threshold which can be changed using up and down arrow keys
int upper_threshold = 200;

int main(int argc, char* argv[]) {
	cout << "Starting ELEC536 Assignment 2 Application" << endl;
	cvNamedWindow( "ELEC536_Project", CV_WINDOW_AUTOSIZE );

	Mat img1 = imread("img/1.pgm", CV_LOAD_IMAGE_GRAYSCALE);
		Mat img2 = imread("img/2.pgm", CV_LOAD_IMAGE_GRAYSCALE);
		Mat img3 = imread("img/3.pgm", CV_LOAD_IMAGE_GRAYSCALE);
		Mat img4 = imread("img/4.pgm", CV_LOAD_IMAGE_GRAYSCALE);
		Mat img5 = imread("img/5.pgm", CV_LOAD_IMAGE_GRAYSCALE);
	Mat result; //working image for most parts
	Mat tmp;
	Mat colorTmp; //color image for connected component labeling



	char key = '0';
	while (key != 'q') {
		switch (key) {
			case '0':
				cout << "subtract background for Question 1 part a and b" << endl;
				result = (img1 - img2) + (img2 - img1);
				imshow("ELEC536_Project", result);
				break;

			case 'a':
				cout << "using manual threshold" <<endl;
				//putText(result, "Differencing the two images.", cvPoint(30,30), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
				manualThreshold(&result, lower_threshold);
				imshow("ELEC536_Project", result);
				//cvWaitKey();
				break;

			case 'b':
				cout << "Question 1 part b: Automatic threshold selection" <<endl;
				//result = (img1 - img2) + (img2 - img1);
				automaticThreshold(&result);
				imshow("ELEC536_Project", result);
				break;

			case 'c':
				cout << "Median Blur filtering" << endl;
				//tmp = result.clone();
				medianBlur(result, result, 5);
				imshow("ELEC536_Project", result);
				break;

			case 'd':
				cout << "Question 2 part a: My Connected Component Labeling" << endl;
				tmp = cvCreateMat(result.rows, result.cols, CV_8UC3 ); //initialize a colour image
				connectedComponentLabeling(&result, &tmp);
				imshow("ELEC536_Project", tmp);
				break;

			case 'e':
				cout << "Question 2 part a implemented with openCV contour function" << endl;
				tmp = cvCreateMat(result.rows, result.cols, CV_8UC3 ); //initialize a colour image
				opencvConnectedComponent(&result, &tmp);
				imshow("ELEC536_Project", tmp);
				break;

			case 'f':
				cout << "Calculating depth map based on blur" << endl;
				//tmp = cvCreateMat(result.rows, result.cols, CV_8UC3 ); //color image
				tmp = result.clone();
				depthFromDiffusion(&result, &tmp, 5);
				imshow("ELEC536_Project", tmp);
				break;

			case '1':
				result = img1.clone();
				imshow("ELEC536_Project", result);
				break;

			case '2':
				result = img2.clone();
				imshow("ELEC536_Project", result);
				break;

			case '3':
				result = img3.clone();
				imshow("ELEC536_Project", result);
				break;

			case '4':
				result = img4.clone();
				imshow("ELEC536_Project", result);
				break;

			case '5':
				result = img5.clone();
				imshow("ELEC536_Project", result);
				break;

			case 82:
				//Up arrow key
				lower_threshold++;
				cout << "Increase threshold to: " << lower_threshold << endl;
				break;

			case 84:
				//Down arrow key
				lower_threshold--;
				cout << "Decrease threshold to: " << lower_threshold << endl;
				break;

			default:
				cout << "The following keys are recognized:" << endl;
				cout << " __________________ " << endl;
				cout << "| Key\t | Question" << endl;
				cout << "| a  \t | 1a  " << endl;
				cout << "| b  \t | 1b  " << endl;
				cout << "| c  \t | 1c  " << endl;
				cout << "| d  \t | 2a my own implementation of connected component " << endl;
				cout << "| e  \t | 2a implementation using OpenCV findContour " << endl;


				cout << "* Key 1 to 5 is used to load images 1 to 5" << endl;
		}
		key = cvWaitKey(0);
		cout << "KEY CODE: " << (int)key << endl;
	}

	//cleanup and leave
	//releaseMat( &result );
	//cvReleaseMat( &img1 );
	//cvReleaseMat( &img2 );
	cvDestroyWindow( "ELEC536_Project" );
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
	cout << "Selected threshold: [[ " << selected_threshold << " ]]" << endl;

	lower_threshold = selected_threshold;

	//convert the image to binary using this selected threshold value
	//manualThreshold(result, selected_threshold);
	threshold(*result, *result, lower_threshold, 255, THRESH_BINARY );
}


/**
 * Q2a. Connected Component Labeling
 * This function
 * I use 4-connectivity and a two pass algorithm.
 * Precondition: Input is a binary image.
 * src is a binary image. dst is a color image
 * */
void connectedComponentLabeling(Mat* src, Mat* dst) {
	//Equivalence class

	//First Pass
//	for(int i = 0; i < src->rows; i++) {
//			uchar* srcRowPtr = src->ptr<uchar>(i);
//			uchar* dstRowPtr =
//			for(int j=0; j < result->cols; j++) {
//				rowPtr[j] =
//			}
//		}
	//Second Pass

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
	int _levels = 0;
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
	cout << "Calculating depth from diffusion with window of size " << size << endl;
		int border = round(size / 2);
		Rect rect = Rect(0, 0, size, size);
		Mat window;
		//Mat sorted_window;

		//smooth the image to get sid of salt and pepper noise

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
					dst->at<uchar>(i,j) = (stdDev.val[0]/(pow(size, 2))) * 255;
					//+ src->at<uchar>(i,j) / 4;
				}

				rect.x += 1; //shift window to right
			}
			//Move window down one row and back to position 0
			rect.y += 1;
			rect.x = 0;
		}
}
