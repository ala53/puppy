#pragma once
#include <assert.h>
//Represents an array of 2-dimensional space. Bounds checks are defined at compile time
template<typename T> class Array2D
{
private:
	int _Width;
	int _Height;
	int _Count;
	//Index "0" in the array
	T* _First;
	//Whether we own the array we're using
	bool _CanDelete;
public:
	inline int Width() { return _Width; }
	inline int Height() { return _Height; }
	inline int Count() { return _Count; }
	//Gets index 0 in the array
	inline T* First() { return _First; }

#if _DEBUG
	static const bool DoBoundsChecks = true;
#else 
	static const bool DoBoundsChecks = false;
#endif
	Array2D(int width, int height) {
		assert(width > 0);
		assert(height > 0);

		_Width = width;
		_Height = height;
		_Count = _Width * _Height;
		_First = new T[_Width * _Height];
		_CanDelete = true;
	}
	//Creates an array from an existing backing memory store
	Array2D(int width, int height, T* backing) {
		assert(width > 0);
		assert(height > 0);

		_Width = width;
		_Height = height;
		_Count = _Width * _Height;
		_First = backing;
		_CanDelete = false;
	}

	~Array2D() {
		if (_CanDelete)
			delete[] _First;
	}

	//Takes a 2D offset in the array and converts it to the equivalent 1D offset
	inline int To1DOffset(int x, int y) { return (y * XSize) + x; }
	//Takes a 1D offset in the array and finds the equivalent 2D offset
	inline void To2DOffset(int oneDOffset, int& outX, int& outY)
	{
		outY = oneDOffset / _Width;
		outX = oneDOffset - (y * _Width);
	}

	//Checks that a value is within the bounds of the array
	inline bool InBounds(int x, int y)
	{
		//Inlined from To1DOffset(x, y) >= 0 && To1DOffset(x, y) < _backing.Length;
		int off = (y * _Width) + x;
		return off >= 0 && off < _Count;
	}

	//Gets the reference to a value in the array
	inline T& Get(int xIndex, int yIndex) {
		if (DoBoundsChecks) assert(InBounds(xIndex, yIndex));
		return _First[(yIndex * _Width) + xIndex];
	}

	//Sets a value in an aray
	inline void Set(int xIndex, int yIndex, T value) {
		if (DoBoundsChecks) assert(InBounds(xIndex, yIndex));
		_First[(yIndex * _Width) + xIndex] = value;
	}

	inline T& operator[](int oneDIndex) {
		if (DoBoundsChecks) {
			int x, y;
			To2DOffset(oneDIndex, &x, &y);
			assert(InBounds(x, y));
		}
		return _First[oneDIndex];
	}
};

