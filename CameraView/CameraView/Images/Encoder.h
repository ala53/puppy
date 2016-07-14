#pragma once
#include <vector>
#include <stdint.h>
#include "CompressedImage.h"
#include "Block.h"
#include "Region.h"
class Encoder
{
private:
	Encoder();
	~Encoder();
	void WriteByte(std::vector<uint8_t> chars, uint8_t byte);
	void EncodeRegion(std::vector<uint8_t> chars, Region& r);
public:
	std::vector<uint8_t> EncodeImage(CompressedImage& image);
	template <typename T> T SwapEndian(T u);
};