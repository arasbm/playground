/****************************************************
* Name        : ELEC536Project.cpp
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

//Standard includes
#include <cmath>
//#include <queue>
//#include <list>

using namespace std;
using namespace cv;
using namespace FlyCapture2;

void process(Mat img);
void processKey(char key);
void addHand(bool left, Point2f center, float radius);
void addFeatureMeanStdDev(bool left, Point2f mean, float stdDev);
void findHands(vector<vector<cv::Point> > contours);
void opencvConnectedComponent(Mat* src, Mat* dst);
void depthFromDiffusion(Mat img, int size);
void initiateCamera();
void startVideo();
void checkGrab();
void checkRelease();
void meanAndStdDevExtract();
void findGoodFeatures(Mat frame1, Mat frame2);
void assignFeaturesToHands();
void drawFeatures(Mat img);
void drawMeanAndStdDev(Mat img);
void featureDepthExtract(const Mat img);
float getDistance(const Point2f a, const Point2f b);
int numberOfHands();
Mat grabImage();
IplImage* convertImageToOpenCV(Image* pImage);

/*** Global settings and modes ***/
int lower_threshold = 10; //global threshold which can be changed using up and down arrow keys
int upper_threshold = 200;
bool save_input_video = false;
bool save_output_video = false;
bool display_video = true;
bool subtract_background = false;
bool use_pgr_camera = true;

bool left_grab_mode = false;
bool right_grab_mode = false;
int grab_std_dev_factor = 14; // the rate at which stdDev is expected to change during grab and release gesture

/** PGR variables **/
Camera pgrCam;
PGRGuid guid;

/** OpenCV variables **/
CvPoint mouseLocation;
const Size imageSize = Size(580, 380);

/** Define some nice RGB colors for dark background **/
Scalar YELLOW = CV_RGB(255, 255, 51);
Scalar ORANGE = CV_RGB(255, 153, 51); //use for left hand
Scalar RED = CV_RGB(255, 51, 51);
Scalar PINK = CV_RGB(255, 51, 153);
Scalar GREEN = CV_RGB(153, 255, 51);
Scalar BLUE = CV_RGB(51, 153, 255); // use for right hand
Scalar OLIVE = CV_RGB(184, 184, 0);
Scalar RANDOM_COLOR = CV_RGB( rand()&255, rand()&255, rand()&255 ); //a random color

/** Hand tracking structures (temporal tracking window) **/
uint feature_temp_window = 3; //Number of frames to look at including current frame to track features
uint hand_temp_window = 5; //Number of frames to keep track of hand
vector<Point2f> leftHandCenter;
vector<Point2f> rightHandCenter;
vector<int> leftHandRadius;
vector<int> rightHandRadius;
vector<Point2f> leftHandFeatureMean;
vector<Point2f> rightHandFeatureMean;
vector<float> leftHandFeatureStdDev;
vector<float> rightHandFeatureStdDev;

/** goodFeaturesToTrack structure and settings **/
vector<Point2f> previousCorners;
vector<Point2f> currentCorners; //Centre point of feature or corner rectangles
vector<uchar> flowStatus;
vector<float> featureDepth;
vector<uchar> leftRightStatus; // 0=None, 1=Left, 2=Right
vector<float> flowError;
TermCriteria termCriteria = TermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 );
double derivLambda = 0.5; //proportion for impact of "image intensity" as opposed to "derivatives"
int maxCorners = 16;
double qualityLevel = 0.01;
double minDistance = 10;
int blockSize = 16;
bool useHarrisDetector = false; //its either harris or cornerMinEigenVal

int main(int argc, char* argv[]) {
	cout << "Starting ELEC536 Grab and Release Project" << endl;
//	cvNamedWindow( "Source", CV_WINDOW_AUTOSIZE ); 		//monochrome source
//	cvNamedWindow( "Processed", CV_WINDOW_AUTOSIZE ); 	//monochrome image after pre-processing
	cvNamedWindow( "Tracked", CV_WINDOW_AUTOSIZE ); 	//Colour with pretty drawings showing tracking results

	//Mat result; //working image for most parts
	//Mat tmp;
	Mat colorImg; //color image for connected component labeling

	startVideo();

//	cvDestroyWindow( "Source" );
//	cvDestroyWindow( "Processed" );
	cvDestroyWindow( "Tracked" );
	cout << "Exiting ELEC536 Grab and Release Project." << endl;
	return 0;
}

/**
 * Add location and radius of a hand to the temporal tracking window
 *  if the vector grow larger than hand_temp_window delete the last element
 * */
void addHand(bool left, Point2f center, float radius) {
	if(left) {
		leftHandCenter.push_back(center);
		leftHandRadius.push_back(radius);
		if(leftHandRadius.size() > hand_temp_window) {
			leftHandCenter.erase(leftHandCenter.begin() + 0);
			leftHandRadius.erase(leftHandRadius.begin() + 0);
		}
	} else {
		rightHandCenter.push_back(center);
		rightHandRadius.push_back(radius);
		if(rightHandRadius.size() > hand_temp_window) {
			rightHandCenter.erase(rightHandCenter.begin() + 0);
			rightHandRadius.erase(rightHandRadius.begin() + 0);
		}
	}
}

/**
 * Add a feature to history and if the vector grow larger than
 * feature_temp_window delete the last element
 * */
void addFeatureMeanStdDev(bool left, Point2f mean, float stdDev) {
	if(left) {
		leftHandFeatureMean.push_back(mean);
		leftHandFeatureStdDev.push_back(stdDev);
		if(leftHandFeatureMean.size() > feature_temp_window) {
			leftHandFeatureMean.erase(leftHandFeatureMean.begin() + 0);
			leftHandFeatureStdDev.erase(leftHandFeatureStdDev.begin() + 0);
		}
	} else {
		rightHandFeatureMean.push_back(mean);
		rightHandFeatureStdDev.push_back(stdDev);
		if(rightHandFeatureMean.size() > feature_temp_window) {
			rightHandFeatureMean.erase(rightHandFeatureMean.begin() + 0);
			rightHandFeatureStdDev.erase(rightHandFeatureStdDev.begin() + 0);
		}
	}
}

/**
 * Check for grab gesture
 * */
void checkGrab() {
	int leftWindow = leftHandFeatureStdDev.size();
	int rightWindow = rightHandFeatureStdDev.size();
	if (leftWindow > 2) {
		//check that standard deviation has been decreasing:
		if (leftHandFeatureStdDev.at(leftWindow - 2) - leftHandFeatureStdDev.at(leftWindow - 1) > grab_std_dev_factor ) {
			if (leftHandFeatureStdDev.at(leftWindow - 3) - leftHandFeatureStdDev.at(leftWindow - 2) > grab_std_dev_factor ) {
				//two consecutive decrease
				left_grab_mode = true;
			}
		}
	}
	if (rightWindow > 2) {
		//check that standard deviation has been decreasing:
		if (rightHandFeatureStdDev.at(rightWindow - 2) - rightHandFeatureStdDev.at(rightWindow - 1) > grab_std_dev_factor ) {
			if (rightHandFeatureStdDev.at(rightWindow - 3) - rightHandFeatureStdDev.at(rightWindow - 2) > grab_std_dev_factor ) {
				//two consecutive decrease
				right_grab_mode = true;
			}
		}
	}
}

/**
 * Check for release gesture
 * */
void checkRelease() {
	int leftWindow = leftHandFeatureStdDev.size();
	int rightWindow = rightHandFeatureStdDev.size();
	if (leftWindow > 2) {
		//check that standard deviation has been decreasing:
		if (leftHandFeatureStdDev.at(leftWindow - 1) - leftHandFeatureStdDev.at(leftWindow - 2) > grab_std_dev_factor ) {
			if (leftHandFeatureStdDev.at(leftWindow - 2) - leftHandFeatureStdDev.at(leftWindow - 3) > grab_std_dev_factor ) {
				//two consecutive decrease
				left_grab_mode = false;
			}
		}
	}
	if (rightWindow > 2) {
		//check that standard deviation has been decreasing:
		if (rightHandFeatureStdDev.at(rightWindow - 1) - rightHandFeatureStdDev.at(rightWindow - 2) > grab_std_dev_factor ) {
			if (rightHandFeatureStdDev.at(rightWindow - 2) - rightHandFeatureStdDev.at(rightWindow - 3) > grab_std_dev_factor ) {
				//two consecutive decrease
				right_grab_mode = false;
			}
		}
	}
}

/**
 * Draw the circles around the hands and the trace of them moving
 * during the temporal window that they are tracked
 * */
void drawHandTrace(Mat img) {
	//left hand
	for(uint i = 1; i < leftHandCenter.size(); i++) {
		if(i == leftHandCenter.size() - 1) {
			//if last element exist
			circle(img, leftHandCenter.at(i), leftHandRadius.at(i) , ORANGE, 2, 4);
			if(left_grab_mode) {
				circle(img, leftHandCenter.at(i), 30, ORANGE, 40, 4);
			} else {
				circle(img, leftHandCenter.at(i), 4, ORANGE, 2, 4);
			}
		} else {
			line(img, leftHandCenter.at(i - 1), leftHandCenter.at(i), ORANGE, 2, 4, 0);
		}
	}

	//right hand
	for(uint i = 1; i < rightHandCenter.size(); i++) {
		if(i == rightHandCenter.size() - 1) {
			//if last element exist
			circle(img, rightHandCenter.at(i), rightHandRadius.at(i) , BLUE, 2, 4);
			if(right_grab_mode) {
				circle(img, rightHandCenter.at(i), 30, BLUE, 40, 4);
			} else {
				circle(img, rightHandCenter.at(i), 4, BLUE, 2, 4);
			}
		} else {
			line(img, rightHandCenter.at(i - 1), rightHandCenter.at(i), BLUE, 2, 4, 0);
		}
	}
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
void depthFromDiffusion(Mat img, int size) {
		int border = round(size / 2);
		Rect rect = Rect(0, 0, size, size);
		Mat window;
		Mat tmp = img.clone();
		//Mat sorted_window;

		// Move the window one pixel at a time through the image and set pixel value
		// in dst to the standard deviation of values in current window
		for(int i = border; i < tmp.rows - border; i++) {
			for(int j = border; j < tmp.cols - border; j++) {
				window = Mat(tmp, rect);
				Scalar mean;
				Scalar stdDev;
				cv::meanStdDev(window, mean, stdDev);
				if (tmp.at<uchar>(i,j) < lower_threshold) {
					img.at<uchar>(i,j) = 0;
				} else {
					//window is either blury or very smooth
					if (tmp.at<uchar>(i,j) > upper_threshold) {
						img.at<uchar>(i,j) = tmp.at<uchar>(i,j);
					} else {
						img.at<uchar>(i,j) = (stdDev.val[0] / pow(size, 2)) * 191 + tmp.at<uchar>(i,j) / 4;
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
    Format7ImageSettings fmt7ImageSettings;
    fmt7ImageSettings.mode = MODE_0;
    fmt7ImageSettings.offsetX = 92;
    fmt7ImageSettings.offsetY = 20;
    fmt7ImageSettings.width = imageSize.width;
    fmt7ImageSettings.height = imageSize.height;
    fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_MONO8;
    Format7PacketInfo fmt7PacketInfo;
    bool valid;

    busManager.GetCameraFromIndex(0, &guid);
	Error pgError;
	pgError = pgrCam.Connect(&guid);
	if (pgError != PGRERROR_OK){
		printf("Error in starting the camera.\n");
		use_pgr_camera = false;
		return;
	}

    //Validate format7 settings
    pgError = pgrCam.ValidateFormat7Settings(
        &fmt7ImageSettings,
        &valid,
        &fmt7PacketInfo );
    if (pgError != PGRERROR_OK)
    {
        cout << "ERROR! in validating format7 settings" << endl;
    }

    if ( !valid )
    {
        // Settings are not valid
		cout << "Format7 settings are not valid" << endl;
    }

    // Set the settings to the camera
    pgError = pgrCam.SetFormat7Configuration(
        &fmt7ImageSettings,
        fmt7PacketInfo.recommendedBytesPerPacket );
    if (pgError != PGRERROR_OK)
    {
        cout << "ERROR setting format7" << endl;
    } else {
		pgrCam.StartCapture();
    }

	if (pgError != PGRERROR_OK){
		cout << "Error in starting the camera capture" << endl;
		use_pgr_camera = false;
		return;
	}
	cout << "pgr camera successfully initialized." << endl;
}

/**
 * Do all the pre-processing before tracking the image
 * */
void process(Mat img) {
	// cvtColor(currentFrame, currentFrame, CV_RGB2GRAY);
	//GaussianBlur(img, img, Size(5,5), 2, 2);
	//medianBlur(img, img, 21);
	//morphologyEx(img, img, MORPH_OPEN, );
	//
	//threshold(img, img, lower_threshold, 255, THRESH_TOZERO);
	//depthFromDiffusion(img, 3);
	//img = img + (img > 30);
	//boxFilter(img, img, 2, Size(3,3), Point(-1,-1), false, BORDER_CONSTANT);
	//bilateralFilter(img, img, 3, 20, 5, BORDER_CONSTANT);
}

/**
 * This functions process the input key and set application mode accordingly
 * */
void processKey(char key) {
	switch(key) {
		case '1':
			break;
		case '2':
			break;
		case 'a':
			break;
		case 's':
			save_input_video = !save_input_video;
			break;
		case 'r':
			save_output_video = !save_output_video;
			break;
		default:
			break;
	}
}

void findGoodFeatures(Mat frame1, Mat frame2) {
	goodFeaturesToTrack(frame1, previousCorners, maxCorners, qualityLevel, minDistance, frame1, blockSize, useHarrisDetector);
	//cornerSubPix(previousFrame, previousCorners, Size(10,10), Size(-1,-1), termCriteria);
	calcOpticalFlowPyrLK(frame1, frame2, previousCorners, currentCorners, flowStatus, flowError, Size(blockSize, blockSize), 1, termCriteria, derivLambda, OPTFLOW_FARNEBACK_GAUSSIAN);
}

/**
 * This is the main loop function that loads and process images one by one
 * The function first attempts to connect to a PGR camera, if that fails it loads
 * a video file from predefined path
 * */
void startVideo(){
	double fps = 8;
	VideoCapture video("../../Desktop/grab_and_release_data/Grab_Release_Mar15.avi");

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
		fps = video.get(CV_CAP_PROP_FPS);
	}


	//Preparing for main video loop
	Mat previousFrame;
	Mat currentFrame;
	Mat trackingResults;
	Mat binaryImg; //binary image for finding contours of the hand
	Mat tmpColor;

	//Prepare for recording video
	VideoWriter sourceWriter = VideoWriter("../../../Desktop/grab_and_release_data/source.avi", CV_FOURCC('D', 'I', 'V', '5'), fps, imageSize);
	VideoWriter resultWriter = VideoWriter("../../../Desktop/grab_and_release_data/result.avi", CV_FOURCC('D', 'I', 'V', '5'), fps, imageSize);

	char key = 'a';
	int frame_count = 0;
	while(key != 'q') {
		if(use_pgr_camera){
			currentFrame = grabImage();
			if(save_input_video) {
				if (sourceWriter.isOpened()) {
					cvtColor(currentFrame, tmpColor, CV_GRAY2RGB);
					sourceWriter << tmpColor;
				}
			}
		} else{
			//This is a video file source, no need to save again
		}

		imshow("Source", currentFrame);

		//do all the pre-processing
		process(currentFrame);
		//imshow("Processed", currentFrame);

		//need at least one previous frame to process
		if(frame_count == 0) {
			previousFrame = currentFrame;
		}
		frame_count++;

		key = cvWaitKey(10);
		processKey(key);

		/**
		 * Prepare the binary image for tracking hands as the two largest blobs in the scene
		 * */
		binaryImg = currentFrame.clone();
		threshold(binaryImg, binaryImg, lower_threshold, 255, THRESH_BINARY);
		medianBlur(binaryImg, binaryImg, 21);
		//adaptiveThreshold(binaryImg, binaryImg, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 10); //adaptive thresholding not works so well here
		imshow("Binary", binaryImg);
		findContours(binaryImg, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		//findContours( binaryImg, contours, RETR_TREE, CV_CHAIN_APPROX_SIMPLE );

		//Canny(previousFrame, previousFrame, 0, 30, 3);
		trackingResults = cvCreateMat(currentFrame.rows, currentFrame.cols, CV_8UC3 );
		cvtColor(currentFrame, trackingResults, CV_GRAY2BGR);

//			if (contours.size() > 0) {
//				int index = 0;
//				for (; index >= 0; index = hiearchy[index][0]) {
//					drawContours(trackingResults, contours, index, ORANGE, 1, 4, hiearchy, 0);
//				}
//			}
		findHands(contours);

		if(numberOfHands() > 0) {
			findGoodFeatures(previousFrame, currentFrame);
			drawHandTrace(trackingResults);
			assignFeaturesToHands();
			//featureDepthExtract(trackingResults);

			//drawFeatures(trackingResults);
			//drawFeatureDepth(trackingResults);

			meanAndStdDevExtract();
			drawMeanAndStdDev(trackingResults);
			checkGrab();
			checkRelease();

		}

		if(save_output_video){
			//TODO: save video
			if (resultWriter.isOpened()) {
				resultWriter << trackingResults;
				putText(trackingResults, "Recording Results ... ", Point(40,120), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
			}
		}
		if(save_input_video){
			//actually saving is done before pre processing above
			putText(trackingResults, "Recording Source ... ", Point(40,40), FONT_HERSHEY_COMPLEX, 1, YELLOW, 3, 8, false);
		}

		imshow("Tracked", trackingResults);
		previousFrame = currentFrame;
		currentCorners = previousCorners;
	}

	//Clean up before leaving
	previousFrame.release();
	currentFrame.release();
	trackingResults.release();
	tmpColor.release();
	if(use_pgr_camera){
		pgrCam.StopCapture();
		pgrCam.Disconnect();
	}
}

/**
 * Find two largest blobs which hopefully represent the two hands
 * */
void findHands(vector<vector<cv::Point> > contours) {
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

	//Detect the two largest circles that represent hands, if they exist
	int radius_threshold = 10;
	if(max1Radius > radius_threshold) {
		if(max1Center.x > max2Center.x) {
			//max1 is on the right
			addHand(false, max1Center, max1Radius);
			if(max2Radius > radius_threshold) {
				//max2 is on the left
				addHand(true, max2Center, max2Radius);
			} else {
				//Clear left hand
				leftHandCenter.clear();
				leftHandRadius.clear();
			}
		} else {
			//max1 is on the left
			addHand(true, max1Center, max1Radius);
			if(max2Radius > radius_threshold) {
				//max2 is on the right
				addHand(false, max2Center, max2Radius);
			} else {
				rightHandCenter.clear();
				rightHandRadius.clear();
			}
		}
	} else {
		rightHandCenter.clear();
		rightHandRadius.clear();
	}
}

/**
 * Calculate and return number of hands by checking the size of HandCenter vectors
 * */
int numberOfHands() {
	int numberOfHands = 0;
	if (leftHandCenter.size() > 0) {
		numberOfHands++;
	}
	if (rightHandCenter.size() > 0) {
		numberOfHands++;
	}
	return numberOfHands;
}

/**
 * Draw features based on the hand they belong to
 * @Precondition: assignFeaturedToHand() is executed and leftRightStatus[i] is filled
 * */
void drawFeatures(Mat img) {
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1) {
			// Left hand
			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), ORANGE);
			circle(img, Point(currentCorners[i].x, currentCorners[i].y), featureDepth[i] + 1, ORANGE);
		} else if(leftRightStatus[i] == 2) {
			// Right hand
			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), BLUE);
			circle(img, Point(currentCorners[i].x, currentCorners[i].y), featureDepth[i] + 1, BLUE);
		} else {
			// None
			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), PINK);
		}

		if((uchar)flowStatus[i] == 1) {
			line(img, currentCorners[i], previousCorners[i], GREEN, 2, 8, 0);
		}
	}
}

/**
 * Draw a circle for mean and stdDev and a trace of their changes over
 * the feature tracking temporal window
 * */
void drawMeanAndStdDev(Mat img) {
	int lastLeftIndex = leftHandFeatureMean.size() - 1;
	int lastRightIndex = rightHandFeatureMean.size() - 1;
	if(lastLeftIndex >= 0 && leftHandCenter.size() > 0) {
		circle(img, leftHandFeatureMean.at(lastLeftIndex), leftHandFeatureStdDev.at(lastLeftIndex), YELLOW, 1, 4, 0);
		line(img, leftHandFeatureMean.at(lastLeftIndex), leftHandCenter.at(leftHandCenter.size() - 1), YELLOW, 1, 4, 0);
	}
	if(lastRightIndex >= 0 && rightHandCenter.size() > 0) {
		circle(img, rightHandFeatureMean.at(lastRightIndex), rightHandFeatureStdDev.at(lastRightIndex), YELLOW, 1, 4, 0);
		line(img, rightHandFeatureMean.at(lastRightIndex), rightHandCenter.at(rightHandCenter.size() - 1), YELLOW, 1, 4, 0);
	}
}

/**
 * assign features to hand(s) assuming that at least one hand exist
 * */
void assignFeaturesToHands() {
	leftRightStatus.clear();
	for(int i = 0; i < maxCorners; i++) {
		int lastLeftIndex = leftHandCenter.size() - 1;
		int lastRightIndex = rightHandCenter.size() - 1;
		if(lastLeftIndex >= 0 && getDistance(currentCorners[i], leftHandCenter.at(lastLeftIndex)) < leftHandRadius.at(lastLeftIndex)) {
			//this feature point belongs to left hand
			leftRightStatus.push_back(1);
		} else if (lastRightIndex >= 0 && getDistance(currentCorners[i], rightHandCenter.at(lastRightIndex)) < rightHandRadius.at(lastRightIndex)) {
			//this feature point belongs to right hand
			leftRightStatus.push_back(2);
		} else {
			//this is noise or some other object
			leftRightStatus.push_back(0); //None
		}
	}
}

/**
 * Calculate the distance between two points
 * */
float getDistance(const Point2f a, const Point2f b) {
	return sqrt(pow((a.x - b.x), 2) + pow((a.y - b.y), 2));
}

/**
 * Find mean point and standard deviation of features for each hand
 * @Precondition: assignFeatureToHands is executed
 * */
void meanAndStdDevExtract() {
	//first calculate the mean
	Point2f leftMean, rightMean;
	uint leftCount = 0, rightCount = 0;
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1) {
			//Left hand
			leftMean += currentCorners[i];
			leftCount++;
		} else if(leftRightStatus[i] == 2) {
			//Right hand
			rightMean += currentCorners[i];
			rightCount++;
		} else {
			//well, nothing if the feature is not assigned to a hand
		}
	}

	leftMean = Point2f(leftMean.x / leftCount, leftMean.y / leftCount);
	rightMean = Point2f(rightMean.x / rightCount, rightMean.y / rightCount);

	//Now that we have the mean, calculate stdDev
	float leftStdDev = 0, rightStdDev = 0;
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1) {
			//Left hand
			leftStdDev += getDistance(leftMean, currentCorners[i]);
		} else if(leftRightStatus[i] == 2) {
			//Right hand
			rightStdDev += getDistance(rightMean, currentCorners[i]);
		} else {
			//feature is not assigned to a hand
		}
	}
	leftStdDev = leftStdDev / leftCount;
	rightStdDev = rightStdDev / rightCount;

	if(leftCount > 0) {
		addFeatureMeanStdDev(true, leftMean, leftStdDev);
	}
	if(rightCount > 0) {
		addFeatureMeanStdDev(false, rightMean, rightStdDev);
	}
}

/**
 * Calculate the depth of each feature based on the blurriness of its window
 * */
void featureDepthExtract(const Mat img) {
	Mat feature;
	Rect rect;
	Scalar stdDev;
	Scalar mean;
	featureDepth.clear();
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1 || leftRightStatus[i] == 2) {
			//left or right hand feature
			rect = Rect(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2, blockSize, blockSize);
			feature = Mat(img, rect);
			meanStdDev(feature, mean, stdDev);
			featureDepth.push_back(stdDev.val[0]);
		} else {
			//Doesnt matter if the feature is not assigned to a hand, so set to -1
			featureDepth.push_back(-1);
		}
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
