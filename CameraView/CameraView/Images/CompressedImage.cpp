#include "CompressedImage.h"

ThreadPool* CompressedImage::_Pool = nullptr;

CompressedImage::CompressedImage(int width, int height)
{
	_InternalWidth = width;
	_InternalHeight = height;
	_RegionsWidth = width / Region::Width;
	_RegionsHeight = height / Region::Height;
	Regions = new Array2D<Region>(RegionsWide(), RegionsTall());
}

CompressedImage::~CompressedImage()
{
	delete[] _TemporaryArray;
	delete Regions;
}

void CompressedImage::SetData(BGRColor * colorData)
{
	//Initialize our temporary data array if this is the first call
	if (_TemporaryArray == nullptr)
		_TemporaryArray = new BGRColor[Width() * Height()];
	RearrangeRGBData(colorData, _TemporaryArray);
	BuildRegions(_TemporaryArray);
}

void CompressedImage::RearrangeRGBData(BGRColor * input, BGRColor * output)
{
	BGRColor* inputCopy = input;
	int i = 0;
	for (int y = 0; y < _InternalHeight; y++)
		for (int x = 0; x < _InternalWidth; x++) {
			inputCopy[y*_InternalWidth + x] = inputCopy[i];
			i++;
		}

	//We need to re-order the data into a chunk format.

	//We need to reorder the data from:
	//(0,0) (1,0)...(99,0) (0,1)...(99,99) to
	//4x4 pixel grids stored in 32 by 32 segments, all row order

	//The code's a bit hard to follow, so at a high level, what it does is:
	//The outer 2 loops go over regions of the image in row order
	//Then the next 2 loops go over each block in each region, again in row order
	//In that loop, we compute the pixel position of the top left of the block
	//Then, the next loop goes over each row of the block, again in row order
	//Here, we compute the memory offset of the top left of the row.
	//Then, the final loop actually copies the RGB data (the optimizer should do its job here)

	//Despite...well...6 nested for loops, the actual algorithm is O(N), I swear

	//The for loop hell is real
	for (int regionY = 0; regionY < RegionsTall(); regionY++) {
		for (int regionX = 0; regionX < RegionsWide(); regionX++)
			for (int blockY = 0; blockY < Region::BlocksPerColumn; blockY++)
				for (int blockX = 0; blockX < Region::BlocksPerRow; blockX++)
				{
					/*
					int blockT = blockY * Block::Height + regionY * Region::Height;
					int blockL = blockX * Block::Width + regionX * Region::Width;

					for (int pixelY = 0; pixelY < Block::Height; pixelY++) {
						for (int pixelX = 0; pixelX < Block::Width; pixelX++) {
							arr->set((pixelX + blockL), (pixelY + blockT), inputCopy[(pixelY + blockT)*_InternalWidth + pixelX + blockL]);
						}
					}
					*/
					int pixelX = regionX * Region::Width + blockX * Block::Width;
					int pixelY = regionY * Region::Height + blockY * Block::Height;
					for (int row = 0; row < Block::Height; row++) {
						uint8_t* inp = (uint8_t*)&input[(pixelY + row) * _InternalWidth + pixelX];
						//Original code:
						//Update: the optimizer sucks :(
						for (int offset = 0; offset < Block::Width * 3; offset++) {
							*(uint8_t*)output = *inp++; //Let us be silent in prayer the optimizer unrolls this into a single movqd
							output = (BGRColor*)(((uint8_t*)output) + 1); //And increment the pointer
						}

						//Optimized code:
						//static_assert(Block::Width == 4 && Block::Height == 4, "Optimized copy is only on 4x4 blocks");
						//Optimized to 2 copies instead of 12 byte copies (4 * 3 bytes)
						// 1 8-byte copy and 1 4-byte copy
						  //*((uint64_t*)output) = *((uint64_t*)inp); //Turn into a 64 bit copy (instead of 24 bit)
						  //inp += 8; //And increment
						  //output = (RGBColor*)(((uint8_t*)output) + 8); //...the pointers

						  //*((uint32_t*)output) = *((uint32_t*)inp);
						  //output = (RGBColor*)(((uint8_t*)output) + 4); //increment output pointer
					}
				}
	}

}

void CompressedImage::BuildRegions(BGRColor* blockArranged) {
	//Instantiate the static thread pool if it's null
	if (_Pool == nullptr)
		_Pool = new ThreadPool(4);
	//Concurrent version of:
	//for (int y = 0; y < RegionsTall(); y++)
	// for (int x = 0; x < RegionsWide(); x++)
	//  image.GetRegion(x, y) = Region(&blockArranged[(y * RegionsWide() * bytesPerRegion) + x * bytesPerRegion]);

	int bytesPerRegion = Region::BlockCount * Block::PixelCount;
	std::vector<std::future<void>> futures;
	for (int y = 0; y < RegionsTall(); y++) {
		futures.push_back(
			_Pool->enqueue(
				[](int y, CompressedImage* image, BGRColor* blockArrangedColors, int bytesPerReg) {
					for (int x = 0; x < image->RegionsWide(); x++) {
						image->GetRegion(x, y) = Region(&blockArrangedColors[(y * image->RegionsWide() * bytesPerReg) + x * bytesPerReg]);
						//And increment for the next block
						//blockArranged += Region::BlockCount * Block::PixelCount;
			}
		}, y, this, blockArranged, bytesPerRegion));
	}

	//And wait for all the enqueued objects
	for (int i = 0; i < futures.size(); i++)
		futures[i].wait();
}

void CompressedImage::GetStatistics(int* sizeBytes, int* sizeBytesWithoutDeduplication, int* deduplicatedBlockCount, int* totalBlockCount)
{
	*sizeBytes = 0;
	*sizeBytesWithoutDeduplication =
		RegionsTall() * RegionsWide() * Region::BlockTableSizeBytes +
		RegionsTall() * RegionsWide() * Region::BlockCount * Block::SizeBytes;

	*deduplicatedBlockCount = 0;
	*totalBlockCount = RegionsTall() * RegionsWide() * Region::BlockCount;

	for (int regionY = 0; regionY < RegionsTall(); regionY++) {
		for (int regionX = 0; regionX < RegionsWide(); regionX++) {
			//Add the region overhead
			*sizeBytes += Region::BlockTableSizeBytes;
			//Iterate over the region
			Region& region = GetRegion(regionX, regionY);
			for (int blockY = 0; blockY < Region::BlocksPerColumn; blockY++) {
				for (int blockX = 0; blockX < Region::BlocksPerRow; blockX++)
				{
					//And, for each block, add its size if it's not deduplicated
					if (region.IsBlockPresent(blockX, blockY))
						*sizeBytes += Block::SizeBytes;
					else
						*deduplicatedBlockCount += 1;
				}
			}
		}
	}
}