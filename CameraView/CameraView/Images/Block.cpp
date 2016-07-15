#include "Block.h"



void Block::FindDistinctColors(BGRColor* colorData, BGRColor* color1, BGRColor* color2)
{
	//Finds the two most distinct colors in the data block
	//You'd expect averages to be better, but somehow they're not
	BGRColor low = BGRColor(255, 255, 255);
	BGRColor high = BGRColor(0, 0, 0);

	for (int i = 0; i < Block::PixelCount; i++) {
		if (BGRColor::Distance(low, colorData[i]) > 0) low = colorData[i];
		if (BGRColor::Distance(high, colorData[i]) < 0) high = colorData[i];
	}

	*color1 = low;
	*color2 = high;
}

void Block::ComputePixelBlending(BGRColor* colorData, BGRColor color1, BGRColor color2)
{
	BGRColor blendColors[4];
	blendColors[0] = color1;
	blendColors[3] = color2;
	blendColors[1] = BGRColor::Blend(color1, color2, 0.33f);
	blendColors[2] = BGRColor::Blend(color1, color2, 0.66f);

	for (int i = 0; i < Block::PixelCount; i++) {
		//Find the most similar color
		PixelBlendFactor factor = PixelBlendFactor::COLOR_1;
		int dist = BGRColor::DistanceAbs(colorData[i], blendColors[0]);
		for (int j = 0; j < 4; j++) {
			int localDist = BGRColor::DistanceAbs(colorData[i], blendColors[j]);
			if (localDist < dist) {
				factor = (Block::PixelBlendFactor)j;
				dist = localDist;
			}
		}

		//And push it to the pixel data array
		int byteOffset = i / 4;
		int shiftAmount = (i % 4) * 2;

		//And add the pixel blending
		PixelValues += blendColors[(int)factor].R();
		PixelValues += blendColors[(int)factor].G();
		PixelValues += blendColors[(int)factor].B();

		PixelData[byteOffset] |= factor << shiftAmount;
	}
}

Block::Block(BGRColor* colorData)
{
	//So, blocks are processed like so:
	//Step 1: find the two most distinct colors in the block
	//Step 2: for each pixel, find the blend that is most similar to the original color
	BGRColor color1, color2;
	FindDistinctColors(colorData, &color1, &color2);
	LowColor = color1.To565();// YUVColor::ToYUV(color1);
	HighColor = color2.To565();// YUVColor::ToYUV(color2);
	//Decode the 565 colors so we have the low precision versions
	ComputePixelBlending(colorData, BGRColor::From565(LowColor), BGRColor::From565(HighColor));
}


Block::~Block()
{
}