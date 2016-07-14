#include "Encoder.h"
#include <climits>

void Encoder::WriteByte(std::vector<uint8_t> chars, uint8_t u)
{
	chars.push_back(u);
}


Encoder::Encoder()
{
}


Encoder::~Encoder()
{
}

void Encoder::EncodeRegion(std::vector<uint8_t> chars, Region& region) {
	//Write the block table
	for (int i = 0; i < Region::BlockTableSizeBytes; i++) {
		WriteByte(chars, region.BlockTable[i]);
	}
	//And write the blocks
	for (int blockY = 0; blockY < Region::BlocksPerColumn; blockY++) {
		for (int blockX = 0; blockX < Region::BlocksPerRow; blockX++) {
			//...but only if they're present
			if (region.IsBlockPresent(blockX, blockY)) {
				Block& block = region.GetBlock(blockY, blockY);
				//First the colors
				WriteByte(chars, block.LowColor.BackingHigh());
				WriteByte(chars, block.LowColor.BackingLow());
				WriteByte(chars, block.HighColor.BackingHigh());
				WriteByte(chars, block.HighColor.BackingLow());
				//Then the blend factors
				for (int i = 0; i < Block::PixelDataLengthBytes; i++) {
					WriteByte(chars, block.PixelData[i]);
				}
			}
		}
	}
}

std::vector<uint8_t> Encoder::EncodeImage(CompressedImage & image)
{
	std::vector<uint8_t> chars;
	//Broken, TODO
	//Write the image size (but to minimize space usage, write the number of regions instead) 
	WriteByte(chars, (uint8_t)image.RegionsWide());
	WriteByte(chars, (uint8_t)image.RegionsTall());

	for (int y = 0; y < image.RegionsTall(); y++) {
		for (int x = 0; y < image.RegionsWide(); x++) {
			EncodeRegion(chars, image.GetRegion(x, y));
		}
	}
	return chars;
}

