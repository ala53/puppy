#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <iostream>
#include <string>
#include "Images\CompressedImage.h"
#include <ctime>
#include "Images\Decoder.h"
#include "Images\ImageDiff.h"
#include <fstream>

int ErrorAndExit(std::string str)
{
	std::cout << str << "\n";
	std::cout << "Press ENTER to exit";
	std::string abc;
	std::getline(std::cin, abc);
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
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	capture.set(CV_CAP_PROP_FPS, 30);
	int width = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH),
		height = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT);

	CompressedImage* img = new CompressedImage(width, height);
	CompressedImage* prev = new CompressedImage(width, height);

	double durationSecs = 10;
	bool temporalDeduplication = true;

	//std::ofstream file;
	//file.open("test.csv");
	//file << "Orig Size" << "," << "Size" << "," << "W/o dedup" << "," << "Spatial dedup" << "," << "Temporal dedup" << "," << "Total dedup" << "," << "Total blocks" << "," << "Temporal regions" << "," << "Total regions" << "," << "\n";



	while (true) {
		//Flip the two images so we actually have a "prev"
		auto tmp = img;
		img = prev;
		prev = tmp;

		std::clock_t start = clock();

		//Capture the frame
		cv::Mat frame;
		capture >> frame;
		if (frame.empty())
			return ErrorAndExit("Empty frame read!");

		if (!frame.isContinuous())
			return ErrorAndExit("Frame storage not contiguous!");

		//Set the data as a simple gradient
		img->SetData((BGRColor*)frame.data);

		//Run a comparison
		ImageDiff diff(*prev, *img, temporalDeduplication ? 768 : 0);
		//And copy all the blocks from the old image
		for (int y = 0; y < diff.RegionsTall(); y++)
			for (int x = 0; x < diff.RegionsWide(); x++)
				if (diff.AreSimilar(x, y)) {
					img->GetRegion(x, y) = prev->GetRegion(x, y);
				}

		double fps = 1 / durationSecs;
		int dedupBlockCount, totalBlockCount, sizeBytes, sizeBytesNoDedup, totalRegions, deduplicatedRegions;

		img->GetStatistics(diff, &sizeBytes, &sizeBytesNoDedup, &dedupBlockCount, &totalBlockCount, &deduplicatedRegions, &totalRegions);

		//file << img->Width() * img->Height() * 3 << "," << sizeBytes << "," << sizeBytesNoDedup << "," << dedupBlockCount << "," << deduplicatedRegions *Region::BlockCount << "," << dedupBlockCount + deduplicatedRegions *Region::BlockCount << "," << totalBlockCount << "," << deduplicatedRegions << "," << totalRegions << "," << "\n";

		std::ostringstream status;
		status << "FPS: " << (int)fps << "\n";
		status << dedupBlockCount + deduplicatedRegions * Region::BlockCount << "/" << totalBlockCount << " blocks deduplicated\n";
		status << "     (" << deduplicatedRegions * Region::BlockCount << " temporal, " << dedupBlockCount << " spatial)\n";
		status << deduplicatedRegions << "/" << totalRegions << " regions deduplicated (temporal)\n";
		status << "Before: " << (img->Width() * img->Height() * 3 * fps) / 1024.0 / 1024.0 << "mb/s\n";
		status << "After: " << (sizeBytes * fps) / 1024.0 / 1024.0 << "mb/s | ";
		status << "W/o dedup: " << (sizeBytesNoDedup * fps) / 1024.0 / 1024.0 << "mb/s\n";
		status << "Image Format: " << type2str(frame.type()) << "\n";
		status << "Temporal Deduplication: " << (temporalDeduplication ? "on" : "off") << "\n";

		Decoder::DecodeImageToBGRArray(*img, (BGRColor*)frame.data, width, height);
		Print(status.str(), frame);
		cv::imshow(windowName, frame);

		int key = cv::waitKey(30);

		durationSecs = (std::clock() - start) / (double)CLOCKS_PER_SEC;

	}
	return 0;
}
