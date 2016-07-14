#pragma once
#include "Region.h"
#include "..\Array2D.h"
#include "BGRColor.h"
#include <stdint.h>
#include "Block.h"
#include "..\ThreadPool.h"

/*
* Each image is made up of a series of regions - large blocks of pixel data (32x32 -- 1024 pixels)
* Those regions are made up, in turn, of small blocks: 4x4 pixels (16 pixels)
*
* Each region is structured as such:
*      There are 8 blocks along the x axis and 8 blocks along the y axis for a total of 64 chunks
*          Neighboring blocks often share similar data and can be represented by their neighbor block's data
*          A region begins with a simple structure: a table representing the data state of each block
*          2 bits are used per block, for a total of 128 bits of data for the so-called "block table":
*              0 = block has its own data
*              2 = block shares data with the block to its left
*              2 = block shares data with block above
*              3 = block shares data with block above and to the left
*          The blocks below and to the right are ignored because it would require look ahead scanning and resolving loops
*		   (e.g. when the block to the left says it is represented by the block to the right but the one to the right says it is
*		    represented by the block to the left -- in effect, the data is lost)
*          The block table is immediately followed by raw block data in a scan order from top left to bottom right, row wise. Any
*          blocks marked as sharing data are not written to the stream.
*      Thus, the region uses the space of up to 66 blocks to represent 64 blocks, but in practice much less as neighboring blocks
*      can be highly similar (e.g. in a flat color image, a region is represented by only 3 blocks of data)
*
* Each of those blocks are made up of 2 color samples stored in 8-4-4 format:
*      8 bit Y (luminosity), 4 bit U and 4 bit V (chroma).
*      and 2 bits per pixel: 0-3 describe how to blend between color 1 and color 2:
*          0 = 100% color 1
*          1 = 66% color 1, 33% color 2
*          2 = 33% color 1, 66% color 2
*          3 = 100% color 2
*
*
* That means each "block" takes up:
*      32 bits = 16 bits * 2 for color samples
*      32 bits = 2 pixels * 16 bits for pixel blending
*      --------
*      64 bits per block (compared to 384 bits uncompressed -- 16 px * 24bpp)
*
* And each region:
*      128 bits = 2 bits * 64 blocks for block table
*      64 bits - 4096 bits = 64 bits * (1 to 64 blocks) for color samples
*      ----------
*      192 bits - 4224 bits per region
*
* The image is then structured as such:
*	2 bytes Width
*	2 bytes Height
*   [Width * Height] bit region table. Any region marked as a "1" is present. Any region marked as a zero is not
*		encoded in the current image and should be copied from the previous frame (the two are identical)
*   Then, the raw regions are written into the stream, in top-left to bottom-right order.
*/

//Represents an image compressed to use less bandwidth in transit
class CompressedImage
{
private:
	//Threadpool for processing
	static ThreadPool* _Pool;
	//The size of the input data to the image. Ergo, the original size
	int _InternalWidth;
	int _InternalHeight;
	//The number of regions wide and tall the image is. The actual encoded size
	int _RegionsWidth;
	int _RegionsHeight;
	//A temporary array initialized on the first SetData() call
	BGRColor* _TemporaryArray = nullptr;
public:
	Array2D<Region>* Regions = nullptr;

	inline int Width() { return _RegionsWidth * Region::Width; }
	inline int Height() { return _RegionsHeight * Region::Height; }
	inline int RegionsWide() { return _RegionsWidth; }
	inline int RegionsTall() { return _RegionsHeight; }

	inline Region& GetRegion(int x, int y) { return Regions->Get(x, y); }

	CompressedImage(int width, int height);
	~CompressedImage();

	//Sets the image's data from a row-ordered RGB array
	void SetData(BGRColor* colorData);

	//Computes some useful statistics on the image. Expensive! Iterates over the entire image.
	void GetStatistics(int* sizeBytes, int* sizeBytesWithoutDeduplication, int* deduplicatedBlockCount, int* totalBlockCount);
private:
	//Reorders the pixel data from openCV into a series of "chunks" in memory
	void RearrangeRGBData(BGRColor* input, BGRColor* output);
	//Builds the region objects in the array
	void BuildRegions(BGRColor* blockArranged);
};

