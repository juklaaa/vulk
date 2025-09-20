#pragma once

#include <stdexcept>

struct V4
{
	V4() = default;
	V4(float x_, float y_, float z_)
		:x(x_), y(y_), z(z_), w(0) {}
	V4(float x_, float y_, float z_, float w_)
		:x(x_), y(y_), z(z_), w(w_) {}
	static V4 zero() { return V4{ 0.0f, 0.0f, 0.0f, 0.0f }; }

	float& operator[] (int index)
	{
		if (index == 0) return x;
		if (index == 1) return y;
		if (index == 2) return z;
		if (index == 3) return w;
		throw std::runtime_error("Wrong index");
	}

	const float& operator[] (int index) const
	{
		if (index == 0) return x;
		if (index == 1) return y;
		if (index == 2) return z;
		if (index == 3) return w;
		throw std::runtime_error("Wrong index");
	}

	float dot(const V4& v) const
	{
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	float length2() const { return x * x + y * y + z * z + w * w; }
	float length() const { return sqrtf(length2()); }
	float dist2(const V4& v) { return (*this - v).length2(); }
	float dist(const V4& v) { return (*this - v).length(); }

	V4 normalize() 
	{
		float l = length(); 
		if(l!=0)
			return{ x / l,y / l,z / l,w / l }; 
		return { 0.0f,0.0f,0.0f,0.0f };
	}

	V4 operator * (float v) const { return { x * v, y * v, z * v, w * v }; }
	V4 operator / (float v) const { return { x / v, y / v, z / v, w / v }; }
	V4 operator + (const V4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	V4 operator - (const V4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
	V4 operator += (const V4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	bool operator == (const V4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }


	float x;
	float y;
	float z;
	float w;
};

struct Mtx
{
	Mtx() = default;

	static Mtx indentity()
	{
		Mtx m;
		memset(&m, 0, sizeof(m));
		m.rows[0][0] = 1.0f;
		m.rows[1][1] = 1.0f;
		m.rows[2][2] = 1.0f;
		m.rows[3][3] = 1.0f;
		return m;
	}

	static Mtx rotate(V4 euler)
	{
		Mtx m;
		memset(&m, 0, sizeof(m));
		float ca = cosf(euler.x); float sa = sinf(euler.x);
		float cb = cosf(euler.y); float sb = sinf(euler.y);
		float cg = cosf(euler.z); float sg = sinf(euler.z);
		m.rows[0] = { cb * cg, sa * sb * cg - ca * sg, ca * sb * cg + sa * sg };
		m.rows[1] = { cb * sg, sa * sb * sg + ca * cg, ca * sb * sg - sa * cg };
		m.rows[2] = { -sb, sa * cb, ca * cb };
		m.rows[3][3] = 1.0f;
		return m;
	}

	static Mtx translate(V4 vec)
	{
		Mtx m;
		memset(&m, 0, sizeof(m));
		m.rows[0][0] = 1.0f;
		m.rows[1][1] = 1.0f;
		m.rows[2][2] = 1.0f;
		m.rows[3] = vec;
		m.rows[3][3] = 1.0f;
		return m;
	}

	V4 getColumn(int i) const
	{
		return V4(rows[0][i], rows[1][i], rows[2][i], rows[3][i]);
	}

	Mtx operator * (const Mtx& mtx) const
	{
		Mtx m;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m.rows[i][j] = rows[i].dot(mtx.getColumn(j));

		return m;
	}

	V4 operator * (const V4& v) const
	{
		V4 res;
		for (int i = 0; i < 4; ++i)
			res[i] = rows[i].dot(v);

		return res;
	}

	const V4& operator[] (int index) const
	{
		return rows[index];
	}

	V4 getPosition() const
	{
		return rows[3];
	}

	V4 rows[4];
};