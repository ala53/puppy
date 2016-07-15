#pragma once
#include "Region.h"
#include "CompressedImage.h"
//Represents the difference between two compressed images
class ImageDiff
{
private:
	int _SimilarityThreshold;
	int _RegionsWide, _RegionsTall;
	Array2D<int> _RegionDiffs;
public:
	inline int& SimilarityThreshold() { return _SimilarityThreshold; }
	inline int Width() { return _RegionsWide * Region::Width; }
	inline int Height() { return _RegionsTall * Region::Height; }
	inline int RegionsWide() { return _RegionsWide; }
	inline int RegionsTall() { return _RegionsTall; }

	ImageDiff(CompressedImage& prev, CompressedImage& curr, int similarityThreshold = 768) : _RegionDiffs(prev.Width() / Region::Width, prev.Height() / Region::Height)
	{
		assert(prev.Width() == curr.Width());
		assert(prev.Height() == curr.Height());

		_RegionsWide = prev.RegionsWide();
		_RegionsTall = prev.RegionsTall();
		_SimilarityThreshold = similarityThreshold;

		for (int y = 0; y < prev.RegionsTall(); y++)
			for (int x = 0; x < prev.RegionsWide(); x++)
			{
				Region& pRegion = prev.GetRegion(x, y);
				Region& cRegion = curr.GetRegion(x, y);

				int largestRegionDiff = 0;
				for (int blockY = 0; blockY < Region::BlocksPerColumn; blockY++) {
					for (int blockX = 0; blockX < Region::BlocksPerRow; blockX++)
					{
						//Compare the blocks
						int diff = Block::DifferenceFactor(pRegion.GetBlock(blockX, blockY), cRegion.GetBlock(blockX, blockY));
						if (diff > largestRegionDiff)
							largestRegionDiff = diff;
					}
				}
				RegionDifference(x, y) = largestRegionDiff;
			}
	}

	//Gets the largest per-block difference between the regions
	inline int& RegionDifference(int x, int y) { return _RegionDiffs.Get(x, y); }

	inline bool AreSimilar(int x, int y) { return RegionDifference(x, y) < _SimilarityThreshold; }

	~ImageDiff()
	{
	}
};

