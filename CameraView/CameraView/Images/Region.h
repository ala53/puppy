#pragma once
#include "Block.h"
#include <stdint.h>
#include <cmath>
#include "BGRColor.h"
class Region
{
private:
	//Pixel values of all the blocks in this region
	int PixelValues;
	//Find blocks which are similar to one another and marks them as identical
	void MatchSimilarBlocks();
public:
	//Defines whether a block is present in the stream, and if not, what block represents it
	enum BlockPresence {
		//The block is present in the data stream and represents itself
		BLOCK_PRESENT,
		//The block spatially to the left represents this block 
		BLOCK_LEFT_REPRESENTS,
		//The block spatially above represents this block
		BLOCK_ABOVE_REPRESENTS,
		//The block spatially above and to the left represents this block 
		BLOCK_ABOVE_LEFT_REPRESENTS
	};

	static const int
		Width = Block::Width * 4,
		Height = Block::Height * 4,
		BlocksPerRow = Width / Block::Width,
		BlocksPerColumn = Height / Block::Height,
		BlockCount = BlocksPerRow * BlocksPerColumn,
		BlockTableSizeBits = BlockCount * 2, //2 bits per block
		BlockTableSizeBytes = BlockTableSizeBits / 8,
		SizeBits = BlockTableSizeBits + Block::SizeBits * BlockCount, //128 bit block table + blocks
		SizeBytes = SizeBits / 8,
		SimilarBlockPixelThreshold = 24,
		SimilarBlockTotalThreshold = 128; //To be tuned as needed

	//The block presence table. Explains whether any block can be represented by its neighbors via BlockPresence enum
	uint8_t BlockTable[BlockTableSizeBytes] = {};
	Block Blocks[BlockCount] = {};

	Region() {}
	//Creates a region from a set of colors. It is expected they are 
	//aligned as 4x4 blocks written in row order -- aka as such:
	//So: (0,0) (1,0) (2,0) (3,0) (0,1) (1,1) (2,1) (3,1)...
	Region(BGRColor* blockColors);
	~Region();

	inline BlockPresence BlockPresenceStatus(int x, int y) {
		int i = y * BlocksPerRow + x;
		int byteOffset = i / 4;
		int shiftAmount = (i % 4) * 2;

		uint8_t byte = BlockTable[byteOffset];
		byte >>= shiftAmount;
		byte &= 0b00000011;
		return (BlockPresence)byte;
	}
	//Returns whether a block is represented by one of its neighbors
	inline bool IsBlockPresent(int x, int y) { return BlockPresenceStatus(x, y) == BLOCK_PRESENT; }

	inline Block& GetBlock(int x, int y) { return Blocks[y * BlocksPerRow + x]; }

	//Compares this region with another to tell if the two are similar
	bool SimilarTo(Region& region, int similarityThresholdTotal, int similarityThresholdPerBlock);
};

