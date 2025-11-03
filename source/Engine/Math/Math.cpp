#include "Math.h"

Quat MtxToQuat(const Mtx& m)
{	
	Quat q = Quat::indentity();

	// TODO: julka zrobi :)

	return q;
}

Mtx QuatToMtx(const Quat& q)
{
	Mtx m = Mtx::indentity();
	m.rows[0] = {q.x*q.x-q.y*q.y-q.z*q.z+q.w*q.w, 2*q.x*q.y-2*q.z*q.w, 2*q.x*q.z+2*q.y*q.w};
	m.rows[1] = {2*q.x*q.y+2*q.z*q.w, -q.x*q.x+q.y*q.y-q.z*q.z+q.w*q.w, 2*q.y*q.z-2*q.x*q.w};
	m.rows[2] = {2*q.x*q.z-2*q.y*q.w, 2*q.y*q.z+2*q.x*q.w, -q.x*q.x-q.y*q.y+q.z*q.z+q.w*q.w};
	return m;
}