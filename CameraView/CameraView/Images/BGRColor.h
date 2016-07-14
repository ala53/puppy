#pragma once
#include <stdint.h>
#include <cmath>
#include <sstream>
#include <string>
#include "RGB565Color.h"

class BGRColor
{
private:
	//Stored as BGR to match OpenCV's internal format
	uint8_t _B, _G, _R;
public:
	inline uint8_t R() { return _R; }
	inline uint8_t G() { return _G; }
	inline uint8_t B() { return _B; }

	BGRColor(uint8_t r, uint8_t g, uint8_t b)
	{
		_R = r;
		_G = g;
		_B = b;
	}

	BGRColor() {}

	~BGRColor()
	{
	}

	inline static BGRColor Blend(BGRColor color1, BGRColor color2, float distance) {
		auto r = (uint8_t)((color1._R * (1 - distance) + color2._R * distance));
		auto g = (uint8_t)((color1._G * (1 - distance) + color2._G * distance));
		auto b = (uint8_t)((color1._B * (1 - distance) + color2._B * distance));

		return BGRColor(r, g, b);
	}

	//Computes the distance between 2 colors -- lower is more similar
	inline static int DistanceAbs(BGRColor color1, BGRColor color2) {
		int dist = 0;
		dist += abs(color1._R - color2._R);
		dist += abs(color1._G - color2._G);
		dist += abs(color1._B - color2._B);

		return dist;
	}

	//Computes the distance between 2 colors -- 0 is the same, positive means color 2 is darker than color 1, negative means color 2 is lighter than color 1
	inline static int Distance(BGRColor color1, BGRColor color2) {
		int dist = 0;
		dist += color1._R - color2._R;
		dist += color1._G - color2._G;
		dist += color1._B - color2._B;

		return dist;
	}

	std::string ToString() {
		std::ostringstream stream;
		stream << "{R:" << (int)R() << ",G:" << (int)G() << ",B:" << (int)B() << "}";
		return stream.str();
	}

	//Creates an RGB 565 compressed color from a 24 bit RGB color
	inline RGB565Color To565() { return RGB565Color(R(), G(), B()); }
	//Creates an RGB color from a 565 formatted color
	inline static BGRColor From565(RGB565Color color) { return BGRColor(color.R(), color.G(), color.B()); }
};

