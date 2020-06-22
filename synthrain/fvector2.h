#pragma once
#include <cmath>

struct fvector2{
	float x, y; 

	float len() { 
		return std::sqrt(x*x+y*y); 
	}

	float lenSq() {
		return x * x + y * y;
	}

	float dot(fvector2 const &other) const
	{
		return x*other.x + y*other.y;
	}

	fvector2 operator+(const fvector2& other) const
	{
		return fvector2{ x + other.x, y + other.y };
	}

	fvector2& operator+=(const fvector2& other)
	{
		this->x += other.x;
		this->y += other.y;
		return *this;
	}

	fvector2 operator-(const fvector2& other) const
	{
		return fvector2{ x - other.x, y - other.y };
	}

	fvector2 operator/(const float div) const
	{
		return fvector2{ x/div, y/div };
	}

	fvector2& operator/=(const float div)
	{
		this->x /= div;
		this->y /= div;
		return *this;
	}

	fvector2& normalize()
	{
		*this /= len();
		return *this;
	}
};

