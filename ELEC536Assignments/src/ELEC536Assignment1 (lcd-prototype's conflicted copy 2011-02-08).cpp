//============================================================================
// Name        : ELEC536Assignment1.cpp
// Author      : Aras Balali Moghaddam
// Version     : 0.1
//============================================================================

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

void applyMyHistogramEq(IplImage* img);
void applyOpenCVHistEq(IplImage* img);
IplImage* createGradientMagImg(IplImage* img);
IplImage* createGradientOrient(IplImage* img);
IplImage* applyLaplacianZeroX(IplImage* img);
void applyLaplacFilter(IplImage* img);

int gradMagThreshold = 30;
double PI = 3.1415;

int main(int argc, char* argv[]) {
	cout << "Starting ELEC536 Assignment 1 Application" << endl;
	cvNamedWindow( "ELEC536_Assignment1", CV_WINDOW_AUTOSIZE );

	char key = 'a';
	IplImage* img;

	while(key != 'q') {
		switch(key) {
			case 'a':
				//Display raising moon image
				img = cvLoadImage("raising_moon_gray_small.jpg", CV_LOAD_IMAGE_GRAYSCALE);
				cvShowImage("ELEC536_Assignment1", img);
				break;

			case 'b':
				cout << "Applying my own histogram algorithm" << endl;
				//reload image as grayscale
				//img = cvLoadImage("raising_moon_gray_small.jpg", CV_LOAD_IMAGE_GRAYSCALE);
				applyMyHistogramEq(img);
				cvShowImage("ELEC536_Assignment1", img);
				break;

			case 'c':
				cout << "Applying OpenCV histogram algorithm" << endl;
				img = cvLoadImage("raising_moon_gray_small.jpg", CV_LOAD_IMAGE_GRAYSCALE);
				//applyOpenCVHistEq(img);
				cvEqualizeHist(img, img);
				cvShowImage("ELEC536_Assignment1", img);
				break;

			case 'd':
				//load the blocks image file
				img = cvLoadImage("blocks.jpg", CV_LOAD_IMAGE_GRAYSCALE);
				cvShowImage("ELEC536_Assignment1", img);
				break;

			case 'e':
				cout << "Compute the gradient magnitude image" << endl;
				cvShowImage("ELEC536_Assignment1", createGradientMagImg(img));
				break;

			case 'f':
				cout << "Compute the gradient orientation image" << endl;
				cvShowImage("ELEC536_Assignment1", createGradientOrient(img));
				break;

			case 'g':
				//GaussianBlur
				cvSmooth(img, img, CV_GAUSSIAN, 7, 7, 0, 0);
				cvShowImage("ELEC536_Assignment1", img);
				break;

			case 'h':
				//Laplacian filter for sharpening the image
				applyLaplacFilter(img);
				cvShowImage("ELEC536_Assignment1", img);
				break;

			case 'i':
				//Applying laplacian kernel to find zero crossings
				cvShowImage("ELEC536_Assignment1", applyLaplacianZeroX(img));
				break;

			case 82:
				//UP Arrow: Increase threshold
				gradMagThreshold++;
				break;

			case 84:
				//Down Arrow: Decrease threshold
				gradMagThreshold--;
				break;

			default:
				cout << "The following keys are recognized:" << endl;
				cout << "a -> Display raising moon image" << endl;
				cout << "b -> Apply my own histogram equalization algorithm" << endl;
				cout << "c -> Applying OpenCV histogram equalization algorithm" << endl;
				cout << "d -> Load the blocks image file" << endl;
				cout << "e -> Compute the gradient magnitude image" << endl;
				cout << "f -> Compute the gradient orientation image" << endl;
				cout << "g -> Apply GaussianBlur" << endl;
				cout << "h -> Apply Laplacian filter for sharpening the image" << endl;
				cout << "i -> Applying laplacian kernel to find zero crossings (not working yet)" << endl;
				cout << "UP-Arrow -> increase threshold" << endl;
				cout << "Down-Arrow -> decrease threshold" << endl;
				break;
		}

		key = cvWaitKey(0);
		cout << "KEY CODE: " << (int)key << endl;
	}

	cvReleaseImage( &img );
	cvDestroyWindow( "ELEC536_Assignment1" );
	return 0;
}

/**
 * applyMyHistogramEq: This is my own implementation of a histogram equalization.
 * @Precondition: img must be an 8bit grayscale image.
 * */
void applyMyHistogramEq(IplImage* img) {
	cout << "Image depth is: " << img->depth << endl;
	cout << "Number of channels: " << img->nChannels << endl;

	//initialize the histogram
	int hist[256] = {0};

	//iterate through all the pixels of the image to count values of the pixel and make the initial histogram
	for(int i = 0; i < img->height; i++) {
		uchar* ptr = (uchar*) (img->imageData + i * img->widthStep);
		for(int j = 0; j < img->width; j++) {
			//Increment the counter for this grayscale value
			hist[(int)ptr[j]] ++;
		}
	}

	//histogram of cumulative sum
	int cumulative_hist[256] = {0};
	int last_sum = 0;
	for(int i = 0; i < 256; i++) {
		cumulative_hist[i] = last_sum + hist[i];
		last_sum = cumulative_hist[i];
	}

	//Mapping gray levels of initial image to target image
	int target_gray[256] = {0};
	int num_of_pixels = img->width * img->height;

	for(int i = 0; i < 256; i++) {
		target_gray[i] = round(cumulative_hist[i]*255/num_of_pixels);
	}

	//iterate through all the pixels and set new grayscale value
	for(int i = 0; i < img->height; i++) {
		uchar* ptr = (uchar*) (img->imageData + i * img->widthStep);
		for(int j = 0; j < img->width; j++) {
			ptr[j] = target_gray[(int)(ptr[j])];
		}
	}
}

/**
 * applyOpenCVHistEq: this function applies OpenCV builtin histogram equalization
 * @precondition: img must be an 8 bit greyscale image
 * */
void applyOpenCVHistEq(IplImage* img) {
	//TODO: write this
}

/**
 * This function creates a gradient magnitude image based on the gradMagThreshold
 * @precondition: image is 8bit grayscale
 * */
IplImage* createGradientMagImg(IplImage* img) {
	int xMag, yMag, totalMag;
	int a,b,c,d; //parameters for calculating xMag and yMag

	CvMat stubImg, *imgMat = (CvMat*) img;
	imgMat = cvGetMat(imgMat, &stubImg, 0, 1);
	CvMat *targetMat = cvCreateMat(img->height, img->width, CV_8UC1);

	//Iterate through and calculate a simple maximum gradient magnitude for each pixel
	//Filter by gradMagThreshold to decide if pixel belongs to an edge or not
	//ignore the pixels on the very edge of the image
	for(int i = 1; i < imgMat->rows - 1; i++) {
	    for(int j = 1; j < imgMat->cols - 1; j++) {
	    	//magnitude along x:
	        a = (int)CV_MAT_ELEM(*imgMat, uchar, i,j-1);
	        b = (int)CV_MAT_ELEM(*imgMat, uchar, i,j+1);
	        xMag = -0.5 * a + 0.5 * b;

	        //magnitude along y:
	        c = (int)CV_MAT_ELEM(*imgMat, uchar, i-1,j);
	        d = (int)CV_MAT_ELEM(*imgMat, uchar, i+1,j);
	        yMag = -0.5*c + 0.5 * d;

	        totalMag = sqrt(xMag*xMag + yMag*yMag);
	        //compare to threshold to decide if this is an edge
	        if(totalMag < gradMagThreshold) {
	        	*((int*)CV_MAT_ELEM_PTR(*targetMat, i,j)) = 200;
	        } else {
	        	*((int*)CV_MAT_ELEM_PTR(*targetMat, i,j)) = 10;
	        }
	    }
	}
	IplImage* tmpImg = (IplImage*) targetMat;
	return tmpImg;
}

/**
 * This function creates a representation of gradient orientation based on intensity
 * Vertical orientation is represented by light grey while horizontal is darker.
 * The formula to calculate the intensity is: (((orientation + PI) / PI) * 200)
 * */
IplImage* createGradientOrient(IplImage* img) {
	int xMag, yMag, totalMag, orientation;
	int a,b,c,d; //parameters for calculating xMag and yMag

	CvMat stubImg, *imgMat = (CvMat*) img;
	imgMat = cvGetMat(imgMat, &stubImg, 0, 1);
	CvMat *targetMat = cvCreateMat(img->height, img->width, CV_8UC1);

	//Iterate through and calculate a simple maximum gradient magnitude for each pixel
	//Filter by gradMagThreshold to decide if pixel belongs to an edge or not
	//ignore the pixels on the very edge of the image
	for(int i = 1; i < imgMat->rows - 1; i++) {
	    for(int j = 1; j < imgMat->cols - 1; j++) {
	    	//magnitude along x:
	        a = (int)CV_MAT_ELEM(*imgMat, uchar, i,j-1);
	        b = (int)CV_MAT_ELEM(*imgMat, uchar, i,j+1);
	        xMag = -0.5 * a + 0.5 * b;

	        //magnitude along y:
	        c = (int)CV_MAT_ELEM(*imgMat, uchar, i-1,j);
	        d = (int)CV_MAT_ELEM(*imgMat, uchar, i+1,j);
	        yMag = -0.5*c + 0.5 * d;

	        orientation = atan2(yMag, xMag);
//	        if (orientation < 0) {
//	        	orientation += PI;
//	        }

	        totalMag = sqrt(xMag*xMag + yMag*yMag);
	        //compare to threshold to decide if this is an edge
	        if(totalMag < gradMagThreshold) {
	        	*((int*)CV_MAT_ELEM_PTR(*targetMat, i,j)) = 255;
	        } else {
	        	*((int*)CV_MAT_ELEM_PTR(*targetMat, i,j)) = orientation * 90;
	        	//*((int*)CV_MAT_ELEM_PTR(*targetMat, i,j)) = (int)((orientation / PI) * 200);
	        }
	    }
	}
	IplImage* tmpImg = (IplImage*) targetMat;
	return tmpImg;
}

/**
 * This method applies a 3x3 laplacian matrix and attempts to find zero crossings on the image
 * */
IplImage* applyLaplacianZeroX(IplImage* img) {
	double laplac[] = { 0, 1, 0,
						1, -4, 1,
						0, 1, 0	};
	Mat lapMat = Mat(3, 3, CV_32FC1, laplac);

	Mat imgMat = (Mat) img;
//	CvMat* targetMat = (CvMat*) img; //cvCreateMat(img->height, img->width, CV_8SC1);

//	for(int i = 1; i < imgMat.rows - 1; i++) {
//		for(int j = 1; j < imgMat.cols - 1; j++) {
//			//Find the matrix at this position and multiply it by laplacian matrix
//			Mat tmp = Mat(imgMat, Range(i+1,j+1), Range(i-1,j-1));
//			Mat productMat = lapMat*tmp;
//			//get sum of the values in productMat. If sum is zero then draw a circle at this position
//			int sum = 0;
//			for(int k = 0; k < productMat.cols; k++) {
//				for(int l = 0; l < productMat.rows; l++) {
//					sum = productMat.at<int>(k,l);
//				}
//			}
//			if(sum == 0) {
//				cvCircle(targetMat, cvPoint(i,j), 2,cvScalar(220,200,200,200),2,2);
//			}
//		}
//	}
	return imgMat.;
}

/**
 * This method traversus and image with a 2x2 matrix. If there is a change in the sign inside the matrix the
 * the top left corner pixel is marked as an edge, otherwise it is set to background.
 * @precondition: the input image should contain gradiant magnitude
 * */
void computeZeroX() {

}

void applyLaplacFilter(IplImage* img) {
	double laplac[] = { 0, 1, 0,
						1, -4, 1,
						0, 1, 0	};
	CvMat lapMat = cvMat(3, 3, CV_32FC1, laplac);
	cvFilter2D(img, img, &lapMat, CvPoint());
}
