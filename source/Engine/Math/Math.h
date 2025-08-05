#pragma once

#include <exception>

struct V4
{
	V4() = default;

	float& operator[] (int index)
	{
		if (index == 0) return x;
		if (index == 1) return y;
		if (index == 2) return z;
		if (index == 3) return w;
		throw std::runtime_error("Wrong index");
	}

	float x;
	float y;
	float z;
	float w;
};

struct Mtx
{
	Mtx() = default;

	static Mtx Indentity() 
	{ 
		Mtx m;
		memset(&m, 0, sizeof(m));
		m.rows[0][0] = 1.0f;
		m.rows[1][1] = 1.0f;
		m.rows[2][2] = 1.0f;
		m.rows[3][3] = 1.0f;
		return m;
	}

	V4 rows[4];
};