#include "Region.h"


Region::Region(BGRColor* blockColors)
{
	//Construct the blocks
	for (int i = 0; i < BlockCount; i++) {
		int arrayOffset = i * Block::PixelCount;
		Blocks[i] = Block(blockColors + arrayOffset);
		PixelValues += Blocks[i].GetTotalPixelValue();
	}
	//And do similarity matching
	MatchSimilarBlocks();
}

void Region::MatchSimilarBlocks()
{
	//The first block must always be present
	for (int i = 1; i < BlockCount; i++) {
		BlockPresence presence = BLOCK_PRESENT;
		//Compare with block to left
		if (Block::SimilarTo(Blocks[i], Blocks[i - 1], SimilarBlockThreshold)) {
			presence = BLOCK_LEFT_REPRESENTS;
			Blocks[i] = Blocks[i - 1];
		}
		//Compare with block above -- assuming this isn't in the first row
		else if (i - BlocksPerRow > 0 && Block::SimilarTo(Blocks[i], Blocks[i - BlocksPerRow], SimilarBlockThreshold)) {
			presence = BLOCK_ABOVE_REPRESENTS;
			Blocks[i] = Blocks[i - BlocksPerRow];
		}
		//Compare with block above and to the left -- assuming this isn't in the first row
		else if (i - 1 - BlocksPerRow > 0 && Block::SimilarTo(Blocks[i],Blocks[i - 1 - BlocksPerRow], SimilarBlockThreshold)) {
			presence = BLOCK_ABOVE_LEFT_REPRESENTS;
			Blocks[i] = Blocks[i - 1 - BlocksPerRow];
		}

		//And write the result to block presence table
		int byteOffset = i / 4;
		int shiftAmount = (i % 4) * 2;
		BlockTable[byteOffset] |= presence << shiftAmount;
	}
}

Region::~Region()
{
}

bool Region::SimilarTo(Region & region, int similarityThresholdTotal, int similarityThresholdPerBlock)
{
	//Check if the overall difference is too high
	if (abs(PixelValues - region.PixelValues) > similarityThresholdTotal)
		return false;
	//Or if the block level differences are too large
	for (int i = 0; i < BlockCount; i++) {
		if (Block::DifferenceFactor(Blocks[i], region.Blocks[i]) > similarityThresholdPerBlock)
			return false;
	}
	//The two regions are close enough
	return true;
}
