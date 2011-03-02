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

int main(int argc, char* argv[]) {
	cout << "Starting ELEC536 Assignment 2 Application" << endl;
	cvNamedWindow( "ELEC536_Assignment2", CV_WINDOW_AUTOSIZE );

	Mat img1, img2, result;

	char key = 'a';
	while (key != 'q') {
		switch (key) {
			case 'a':
				img1 = imread("img/stuff.pgm");
				img2 = imread("img/stufmin1.pgm");
				result = (img1 - img2) + (img2 - img1);
				imshow("ELEC536_Assignment2", result);
				break;

			case 'b':
				break;

			default:
				cout << "The following keys are recognised:" << endl;
		}
		key = cvWaitKey(0);
		cout << "KEY CODE: " << (int)key << endl;
	}
}
