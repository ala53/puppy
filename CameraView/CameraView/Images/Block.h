#pragma once
#include <stdint.h>
#include "BGRColor.h"
#include "RGB565Color.h"
#include <cmath>

class Block
{
private:
	void FindDistinctColors(BGRColor* colorData, BGRColor* color1, BGRColor* color2);
	void ComputePixelBlending(BGRColor* colorData, BGRColor color1, BGRColor color2);
public:
	//Represents the blending between the two colors in the block
	enum PixelBlendFactor {
		//100% color 1
		COLOR_1 = 0,
		//66% color 1, 33% color 2
		COLOR_1_66_33 = 1,
		//66% color 2, 33% color 1
		COLOR_2_66_33 = 2,
		//100% color 2
		COLOR_2 = 3
	};

	static const int
		Width = 8, // Must be power of two -- do not change
		Height = 8, // Must be power of two -- do not change
		PixelCount = Width * Height,
		RowSizeBits = Width * 2,
		RowSizeBytes = RowSizeBits / 8,
		PixelDataLengthBits = RowSizeBits * Height,
		PixelDataLengthBytes = PixelDataLengthBits / 8,
		SizeBits = (Width * Height * 2) + (RGB565Color::ColorDepthBits * 2), // W*H*2bpp + 2*colors
		SizeBytes = SizeBits / 8;

	//The two colors to blend between
	RGB565Color LowColor, HighColor;

	//The raw pixel data -- initialized as zeroes
	uint8_t PixelData[PixelDataLengthBytes] = {};

	Block(BGRColor* colorData);
	Block() {}
	~Block();

	//Keep this at the end so writing <SizeBits> bits to a stream does not accidentally write the PixelValues
private:
	//For behind the scenes comparison: it gets the total RGB value of all 16 pixels
	int PixelValues = 0;
public:
	//Returns the added-together values of the R,G, and B values of all 16 pixels. Used for internal comparisons.
	inline int GetTotalPixelValue() { return PixelValues; }

	//Gets the blend factor for a specific pixel in the block
	inline PixelBlendFactor GetBlendFactor(int x, int y) {
		int i = y * Width + x;
		int byteOffset = i / 4;
		int shiftAmount = (i % 4) * 2;
		uint8_t byte = PixelData[byteOffset];
		byte >>= shiftAmount;
		byte &= 0b00000011;
		return (PixelBlendFactor)byte;
	}

	//Decodes a pixel's data.
	inline BGRColor Decode(int x, int y) {
		//Get the 4 possible RGB blends
		RGB565Color blends[4];
		blends[0] = LowColor;
		blends[3] = HighColor;
		blends[1] = RGB565Color::Blend(blends[0], blends[3], 0.33f);
		blends[2] = RGB565Color::Blend(blends[0], blends[3], 0.66f);

		return BGRColor::From565(blends[(int)GetBlendFactor(x, y)]);
	}

	//Compares 2 blocks and returns whether they fall within the similarity threshold.
	inline bool SimilarTo(Block& other, int similarityThreshold) {
		//Kept in header file for performance (better inlining heuristics)
		//Not a perfect or even fair comparison
		return abs(other.PixelValues - PixelValues) < similarityThreshold;
	}
	//Gets the difference factor between the two blocks
	inline int GetSimilarity(Block& other) {
		return abs(other.PixelValues - PixelValues);
	}
};

