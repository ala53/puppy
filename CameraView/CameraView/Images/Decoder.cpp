#include "Decoder.h"



Decoder::Decoder()
{
}


Decoder::~Decoder()
{
}

void Decoder::DecodeImageToBGRArray(CompressedImage & image, BGRColor * arr, int arrWidth, int arrHeight)
{
	//Make sure the RGB array is big enough
	assert(arrWidth >= image.Width());
	assert(arrHeight >= image.Height());

	for (int regionY = 0; regionY < image.RegionsTall(); regionY++) {
		for (int blockY = 0; blockY < Region::BlocksPerColumn; blockY++) {
			for (int pixelY = 0; pixelY < Block::Height; pixelY++) {
				for (int regionX = 0; regionX < image.RegionsWide(); regionX++) {
					//Cache the region for perf
					Region& region = image.GetRegion(regionX, regionY);

					for (int blockX = 0; blockX < Region::BlocksPerRow; blockX++)
					{
						int blockTopLeftX = regionX * Region::Width + blockX * Block::Width;
						int blockTopLeftY = regionY * Region::Height + blockY * Block::Height;
						//Get the block
						Block& block = region.GetBlock(blockX, blockY);
						//Get the 4 possible RGB blend colors
						BGRColor blends[4];
						blends[0] = BGRColor::From565(block.LowColor);
						blends[3] = BGRColor::From565(block.HighColor);
						blends[1] = BGRColor::Blend(blends[0], blends[3], 0.33f);
						blends[2] = BGRColor::Blend(blends[0], blends[3], 0.66f);

						//Go over all the pixels in the row and write them into the output vector
						for (int pixelX = 0; pixelX < Block::Width; pixelX++) {
							*arr++ = blends[(int)block.GetBlendFactor(pixelX, pixelY)];
						}
					}
				}
				//Because we might not match up with width (we may downscale to the nearest region bound),
				//we have to adjust the pointer every row
				arr += arrWidth - image.Width();
			}
		}
	}
}

BGRColor * Decoder::DecodeImageToBGRArray(CompressedImage & image)
{
	BGRColor* out = new BGRColor[image.Width() * image.Height()];
	DecodeImageToBGRArray(image, out, image.Width(), image.Height());
	return out;
}

CompressedImage& Decoder::DeserializeImage(uint8_t* serializedData)
{
	int regionsWide = serializedData[0];
	int regionsTall = serializedData[1];
	auto img = new CompressedImage(regionsWide * Region::Width, regionsTall * Region::Height);

	DeserializeImage(*img, serializedData);

	return *img;
}

void Decoder::DeserializeImage(CompressedImage & image, uint8_t* serializedData)
{
	int regionsWide = serializedData[0];
	int regionsTall = serializedData[1];

	assert(regionsWide == image.RegionsWide() /*Image width wrong*/);
	assert(regionsTall == image.RegionsTall() /*Image height wrong*/);

	//Read the regions
	for (int y = 0; y < regionsTall; y++) {
		for (int x = 0; x < regionsWide; x++) {
			DecodeRegion(&serializedData, image.GetRegion(x, y));
		}
	}

	//And that's all she wrote -- it is "decoded" now
}

void Decoder::DecodeRegion(uint8_t** ptr, Region& r)
{
	auto data = *ptr;

	//Read the block table
	uint8_t blockTable[Region::BlockTableSizeBytes];

	for (int i = 0; i < Region::BlockTableSizeBytes; i++)
		blockTable[i] = *data++;

	//Read all the present blocks
	for (int y = 0; y < Region::BlocksPerColumn; y++) {
		for (int x = 0; x < Region::BlocksPerRow; x++) {
			if (r.IsBlockPresent(x, y)) {
				//Decode the block
				uint8_t
					low_high = *data++, low_low = *data++,
					high_high = *data++, high_low = *data++;

				Block b = Block();
				//Read the color data
				b.LowColor = RGB565Color::CreateFromHighLow(low_high, low_low);
				b.HighColor = RGB565Color::CreateFromHighLow(high_high, high_low);
				//And the blend factors
				for (int i = 0; i < Block::PixelDataLengthBytes; i++)
					b.PixelData[i] = *data++;
			}
		}
	}

	//And update the data pointer
	*ptr = data;
}