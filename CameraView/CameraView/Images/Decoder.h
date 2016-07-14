#pragma once
#include "BGRColor.h"
#include "CompressedImage.h"
#include <assert.h>
class Decoder
{
private:
	Decoder();
	~Decoder();
	static void DecodeRegion(uint8_t** ptr, Region& r);
public:
	//Decodes the image data to a user provided RGB array
	static void DecodeImageToBGRArray(CompressedImage& image, BGRColor* arr, int arrWidth, int arrHeight);
	//Decodes an image to an RGB array (which is created for the image data)
	static BGRColor* DecodeImageToBGRArray(CompressedImage& image);
	//Deserializes an image object from its binary representation
	static CompressedImage& DeserializeImage(uint8_t* serializedData);
	//Deserializes an image object from its binary representation
	static void DeserializeImage(CompressedImage& image, uint8_t* serializedData);
};

