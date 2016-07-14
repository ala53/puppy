#pragma once
#include <stdint.h>
//Represents an RGB color in 16 bits
class RGB565Color
{
private:
	union {
		uint16_t _backing;
		uint8_t _Low, _High; // Little endian is weird
	};
public:
	inline uint8_t R() { return (uint8_t)((_backing & 0b1111100000000000) >> 8); }
	inline uint8_t G() { return (uint8_t)((_backing & 0b0000011111100000) >> 3); }
	inline uint8_t B() { return (uint8_t)((_backing & 0b0000000000011111) << 3); }
	inline uint16_t Backing() { return _backing; }
	inline uint8_t BackingHigh() { return _High; }
	inline uint8_t BackingLow() { return _Low; }

	static const int
		ColorDepthBits = 16, // Must be power of two -- do not change
		SizeBits = ColorDepthBits,
		SizeBytes = SizeBits / 8,
		RBits = 5,
		GBits = 6,
		BBits = 5;

	RGB565Color()
	{
	}

	RGB565Color(uint8_t r, uint8_t g, uint8_t b) {
		//Truncate to significant bits
		r &= 0b11111000;
		g &= 0b11111100;
		b &= 0b11111000;
		//And compress
		_backing = (uint16_t)((r << 8) | (g << 3) | (b >> 3));
	}

	~RGB565Color()
	{
	}

	inline static RGB565Color CreateFromHighLow(uint8_t high, uint8_t low) {
		RGB565Color color;
		color._High = high;
		color._Low = low;
		return color;
	}

	inline static RGB565Color CreateFromBacking(uint16_t backing) {
		RGB565Color color;
		color._backing = backing;
		return color;
	}

	inline static RGB565Color Blend(RGB565Color color1, RGB565Color color2, float distance) {
		auto r = (uint8_t)((color1.R() * (1 - distance) + color2.R() * distance));
		auto g = (uint8_t)((color1.G() * (1 - distance) + color2.G() * distance));
		auto b = (uint8_t)((color1.B() * (1 - distance) + color2.B() * distance));

		return RGB565Color(r, g, b);
	}

	//Computes the distance between 2 colors -- lower is more similar
	inline static int DistanceAbs(RGB565Color color1, RGB565Color color2) {
		int dist = 0;
		dist += abs(color1.R() - color2.R());
		dist += abs(color1.G() - color2.G());
		dist += abs(color1.B() - color2.B());

		return dist;
	}

	//Computes the distance between 2 colors -- 0 is the same, positive means color 2 is darker than color 1, negative means color 2 is lighter than color 1
	inline static int Distance(RGB565Color color1, RGB565Color color2) {
		int dist = 0;
		dist += color1.R() - color2.R();
		dist += color1.G() - color2.G();
		dist += color1.B() - color2.B();

		return dist;
	}

	std::string ToString() {
		std::ostringstream stream;
		stream << "{R:" << (int)R() << ",G:" << (int)G() << ",B:" << (int)B() << "}";
		return stream.str();
	}
};

