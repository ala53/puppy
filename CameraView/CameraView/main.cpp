#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <iostream>
#include <string>
#include "Images\CompressedImage.h"
#include <ctime>
#include "Images\Decoder.h"

int ErrorAndExit(std::string str)
{
	std::cout << str;
	std::cout << "Press any key to exit";
	int a = 0;
	std::cin >> a;
	exit(-1);
	return -1;
}
std::string type2str(int type) {
	std::string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}

void PrintLine(std::string line, int lineNumber, cv::Mat frame) {
	lineNumber += 1;
	//Draw border line
	cv::putText(frame, line, cv::Point2i(20 + 1, 20 * lineNumber + 1), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 0, 0), 1, CV_AA);
	cv::putText(frame, line, cv::Point2i(20 - 1, 20 * lineNumber - 1), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 0, 0), 1, CV_AA);
	cv::putText(frame, line, cv::Point2i(20, 20 * lineNumber), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255, 255, 255), 1, CV_AA);
}

void Print(std::string data, cv::Mat frame) {
	std::istringstream stream(data);
	int lineNum = 0;
	while (!stream.eof()) {
		std::string line;
		std::getline(stream, line);
		PrintLine(line, lineNum, frame);
		lineNum++;
	}
}

int main()
{
	auto windowName = "Camera";
	cvNamedWindow(windowName, CV_WINDOW_AUTOSIZE);
	cv::VideoCapture capture;
	if (!capture.open(0))
		return ErrorAndExit("Could not open camera");
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	capture.set(CV_CAP_PROP_FPS, 30);
	int width = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH),
		height = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	CompressedImage img(width, height);

	double durationSecs = 10;

	while (true) {
		std::clock_t start = clock();

		//Capture the frame
		cv::Mat frame;
		capture >> frame;
		if (frame.empty())
			return ErrorAndExit("Empty frame read!");

		if (!frame.isContinuous())
			return ErrorAndExit("Frame storage not contiguous!");

		//Set the data as a simple gradient
		img.SetData((BGRColor*)frame.data);

		Decoder::DecodeImageToBGRArray(img, (BGRColor*)frame.data, width, height);

		double fps = 1 / durationSecs;
		int dedupBlockCount = 0, totalBlockCount = 0, sizeBytes = 0, sizeBytesNoDedup;

		img.GetStatistics(&sizeBytes, &sizeBytesNoDedup, &dedupBlockCount, &totalBlockCount);

		std::ostringstream status;
		status << "FPS: " << (int)fps << "\n";
		status << dedupBlockCount << "/" << totalBlockCount << " blocks deduplicated\n";
		status << "Before: " << (img.Width() * img.Height() * 3 * fps) / 1024.0 / 1024.0 << "mb/s | ";
		status << "After: " << (sizeBytes * fps) / 1024.0 / 1024.0 << "mb/s | ";
		status << "W/o dedup: " << (sizeBytesNoDedup * fps) / 1024.0 / 1024.0 << "mb/s\n";
		status << "Image Format: " << type2str(frame.type());

		Print(status.str(), frame);
		//cv::putText(frame, status.str(), cv::Point2i(20, 20), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(128, 0, 0), 1, CV_AA);
		cv::imshow(windowName, frame);
		cv::waitKey(30);

		durationSecs = (std::clock() - start) / (double)CLOCKS_PER_SEC;

	}
	return 0;
}
