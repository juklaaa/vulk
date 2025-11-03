#pragma once

#include <stdexcept>
#include <cassert>

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
	friend V4 operator * (float v, const V4& vec) { return vec * v; }
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

	Mtx(V4 r0, V4 r1, V4 r2, V4 r3)
	{
		rows[0] = r0;
		rows[1] = r1;
		rows[2] = r2;
		rows[3] = r3;
	}

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

	static Mtx scale(V4 vec)
	{
		Mtx m;
		memset(&m, 0, sizeof(m));
		m.rows[0][0] = vec.x;
		m.rows[1][1] = vec.y;
		m.rows[2][2] = vec.z;
		m.rows[3][3] = 1.0f;
		return m;
	}

	V4 getColumn(int i) const
	{
		return V4(rows[0][i], rows[1][i], rows[2][i], rows[3][i]);
	}

	V4 getRow(int i) const
	{
		return rows[i];
	}

	Mtx operator * (const Mtx& mtx) const
	{
		Mtx m;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m.rows[i][j] = rows[i].dot(mtx.getColumn(j));

		return m;
	}

	friend V4 operator * (const V4& v, const Mtx& m)
	{
		V4 res;
		for (int i = 0; i < 4; ++i)
			res[i] = v.dot(m.getColumn(i));

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

	bool operator == (const Mtx& other) const
	{
		for (int i = 0; i < 4; ++i)
			if (rows[i] != other.rows[i])
				return false;
		return true;
	}

	Mtx inversedTransform() const
	{
		V4 row0 = getRow(0), row1 = getRow(1), row2 = getRow(2), translation = getRow(3);

		float a = row0[0], b = row0[1], c = row0[2],
			d = row1[0], e = row1[1], f = row1[2],
			g = row2[0], h = row2[1], i = row2[2];

		float W = a * (e * i - f * h) + b * (f * g - d * i) + c * (d * h - e * g);

		float w[3][3];
		w[0][0] = e * i - f * g;
		w[0][1] = f * g - d * i;
		w[0][2] = d * h - e * g;

		w[1][0] = c * h - b * i;
		w[1][1] = a * i - c * g;
		w[1][2] = b * g - a * h;
		
		w[2][0] = b * f - c * e;
		w[2][1] = c * d - a * f;
		w[2][2] = a * e - b * d;

		V4 rs[3];
		rs[0][3] = 0;
		rs[1][3] = 0;
		rs[2][3] = 0;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				rs[i][j] = w[j][i] / W;
			}
		}

		V4 newTransform;
		newTransform[0] = -(rs[0][0] * translation[0] + rs[0][1] * translation[1] + rs[0][2] * translation[2]);
		newTransform[1] = -(rs[1][0] * translation[0] + rs[1][1] * translation[1] + rs[1][2] * translation[2]);
		newTransform[2] = -(rs[2][0] * translation[0] + rs[2][1] * translation[1] + rs[2][2] * translation[2]);
		newTransform[3] = 1;
		
		Mtx inv = {rs[0],rs[1],rs[2], newTransform};
		return inv;
	}

	V4 rows[4];
};

inline void testInverseAlgorithm()
{
	assert(
		Mtx({ 1,0,0,0 },
			{ 0,1,0,0 },
			{ 0,0,1,0 },
			{ 1,2,3,1 }).inversedTransform()
	==
		Mtx({ 1,0,0,0 },
			{ 0,1,0,0 },
			{ 0,0,1,0 },
			{ -1,-2,-3,1 }));

	assert(
		Mtx({ 0,-1,0,0 },
			{ 1,0,0,0 },
			{ 0,0,1,0 },
			{ 1,2,3,1 }).inversedTransform()
	==
		Mtx({ 0,1,0,0 },
			{ -1,0,0,0 },
			{ 0,0,1,0 },
			{ -2,1,-3,1 }));
}

struct Quat
{
	Quat() = default;

	Quat(const V4& v, float angle)
	{
		w = cosf(angle / 2.0f);
		float sn = sinf(angle / 2.0f);
		V4 sinv = v * sn;
		x = sinv.x;
		y = sinv.y;
		z = sinv.z;
	}

	static Quat indentity()
	{
		Quat q;
		q.x = 0.0f;
		q.y = 0.0f;
		q.z = 0.0f;
		q.w = 1.0f;
		return q;
	}

	float x;
	float y;
	float z;
	float w;
};

Quat MtxToQuat(const Mtx& m);
Mtx QuatToMtx(const Quat& q);