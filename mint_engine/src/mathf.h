#pragma once
#define MATHF_H_
#include <math.h>

const float PI = 3.141592f;
const float DEG2RAD = PI / 180.0f;
const float RAD2DEG = 180.0f / PI;
const float EPSILON = 0x8p-26F;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef float f32;
typedef double f64;

struct mat4;
struct vec2;
struct vec3;

#define FLOAT_MAX 3.402823e+038f
#define FLOAT_MIN 1.175494e-038f

static inline float signf(float num){return (num >= 0.0f) ? 1.0f : -1.0f;}
static inline float clamp(float value, float min, float max){return fminf(max, fmaxf(min, value));}
static inline float clamp01(float value){return fminf(1.0f, fmaxf(0.0f, value));}
static inline vec3 clamp(vec3 value, vec3 min, vec3 max);

void rand_set_seed(u32 seed);
float rand_get();
float rand_range(float min, float max);
int rand_range(int min, int max);
vec2 rand_in_circle(float radius=1.0f);
vec3 rand_in_sphere(float radius=1.0f);
vec3 rand_cone(const vec3 &dir, float cone_half_angle);


struct vec2
{
	float x, y;

	vec2() : x(0.0f), y(0.0f){}
	vec2(float x, float y):x(x), y(y){}

	inline vec2 operator+(const vec2 &rhs) const {return vec2(x+rhs.x, y+rhs.y);}
	inline vec2 operator-(const vec2 &rhs) const {return vec2(x-rhs.x, y-rhs.y);}
	inline vec2 operator-() const {return vec2(-x, -y);}
	inline vec2 operator*(float rhs) const {return vec2(x*rhs, y*rhs);}
	inline vec2& operator+=(const vec2 &rhs){x+=rhs.x; y+=rhs.y; return *this;}

	inline float len() const {return sqrtf(x * x + y * y);}
	inline float sqrlen() const {return x * x + y * y;} // squared length
};

struct vec3
{
	union{
		struct{float x, y, z;};
		struct{float r, g, b;};
	};

	vec3() : x(0.0f), y(0.0f), z(0.0f){}
	vec3(float x, float y, float z){
		this->x = x; this->y = y; this->z = z;
	}

	inline vec3 operator+(const vec3 &rhs) const {return vec3(x+rhs.x, y+rhs.y, z+rhs.z);}
	inline vec3 operator-(const vec3 &rhs) const {return vec3(x-rhs.x, y-rhs.y, z-rhs.z);}
	inline vec3 operator-() const {return vec3(-x, -y, -z);}
	inline vec3 operator*(vec3 rhs) const {return vec3(x * rhs.x, y * rhs.y, z * rhs.z);}
	inline vec3 operator*(float rhs) const {return vec3(x*rhs, y*rhs, z*rhs);}
	inline vec3 operator/(float rhs) const {return vec3(x/rhs, y/rhs, z/rhs);}
	inline vec3& operator+=(const vec3 &rhs){x+=rhs.x; y+=rhs.y; z+=rhs.z; return *this;}
	inline vec3& operator-=(const vec3 &rhs){x-=rhs.x; y-=rhs.y; z-=rhs.z; return *this;}
	inline vec3& operator*=(float rhs){x*=rhs; y*=rhs; z*=rhs; return *this;}

	inline float len() const {return sqrtf(x * x + y * y + z * z);}
	inline float sqrlen() const {return x * x + y * y + z * z;} // squared length

	inline vec3 normalized() const
	{
		float len = this->len();
		if(len > (1e-6))
		{
			return vec3(x/len, y/len, z/len);
		}
		return vec3();
	}

	static vec3 min(const vec3 &lhs, const vec3 &rhs)
	{
		vec3 v;
		v.x = lhs.x < rhs.x ? lhs.x : rhs.x;
		v.y = lhs.y < rhs.y ? lhs.y : rhs.y;
		v.z = lhs.z < rhs.z ? lhs.z : rhs.z;
		return v;
	}

	static vec3 max(const vec3 &lhs, const vec3 &rhs)
	{
		vec3 v;
		v.x = lhs.x > rhs.x ? lhs.x : rhs.x;
		v.y = lhs.y > rhs.y ? lhs.y : rhs.y;
		v.z = lhs.z > rhs.z ? lhs.z : rhs.z;
		return v;
	}

	static vec3 abs(const vec3 &v)
	{
		return vec3(fabsf(v.x), fabsf(v.y), fabsf(v.z));
	}

	static float dot(const vec3 &lhs, const vec3 &rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	static vec3 cross(const vec3 &lhs, const vec3 &rhs)
	{
		return vec3(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
	}

	static vec3 project(const vec3 &v, const vec3 &normal)
	{
		float sqrlen = normal.sqrlen();
		if(sqrlen < EPSILON){return vec3();}

		float dot = vec3::dot(v, normal);
		return vec3(normal.x * dot / sqrlen,
					normal.y * dot / sqrlen,
					normal.z * dot /sqrlen);
	}

	static vec3 project_on_plane(const vec3 &v, const vec3 &normal)
	{
		float sqrlen = normal.sqrlen();
		if(sqrlen < EPSILON){return v;}

		float dot = vec3::dot(v, normal);
		return vec3(v.x - normal.x * dot / sqrlen,
					v.y - normal.y * dot / sqrlen,
					v.z - normal.z * dot /sqrlen);
	}

	static vec3 reflect(const vec3 &v, const vec3 &normal)
	{
		float factor = -2.0f * vec3::dot(v, normal);
		return vec3(normal.x * factor + v.x,
					normal.y * factor + v.y,
					normal.z * factor + v.z);
	}

	static float angle(const vec3 &lhs, const vec3 &rhs)
	{
		vec3 cross = vec3::cross(lhs, rhs);
		float len = cross.len();
		float dot = vec3::dot(lhs, rhs);
		return atan2f(len, dot) * RAD2DEG;
	}
};

static inline vec3 clamp(vec3 value, vec3 min, vec3 max)
{
	return vec3(fmaxf(min.x, fminf(max.x, value.x)),
				fmaxf(min.y, fminf(max.y, value.y)),
				fmaxf(min.z, fminf(max.z, value.z)));
}

struct vec4
{
	union{
		struct{float x, y, z, w;};
		struct{float r, g, b, a;};
	};
	
	vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f){}
	vec4(float x) : x(x), y(x), z(x), w(x) {}
	vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	inline float len() const {return sqrtf(x * x + y * y + z * z + w * w);}
	inline float sqrlen() const {return x * x + y * y + z * z + w * w;} // squared length
};

struct rect_t
{
	float x, y, w, h;
	rect_t() : x(0.0f), y(0.0f), w(0.0f), h(0.0f){}
	rect_t(float x, float y, float w, float h) : x(x), y(y), w(w), h(h){}
};

struct quat
{
	float x, y, z, w;
	quat() : x(0), y(0), z(0), w(1){}
	quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w){}
	quat(float x, float y, float z){*this = quat::euler(x, y, z);}
	quat(const vec3 &euler){*this = quat::euler(euler);}
	quat(const quat &q) : x(q.x), y(q.y), z(q.z), w(q.w){}
	inline mat4 to_mat4() const;
	inline vec3 to_euler()
	{
		vec3 e = {};

		// Roll (x-axis rotation)
		float x0 = 2.0f*(w*x + y*z);
		float x1 = 1.0f - 2.0f*(x*x + y*y);
		e.x = atan2f(x0, x1);

		// Pitch (y-axis rotation)
		float y0 = 2.0f*(w*y - z*x);
		y0 = y0 > 1.0f ? 1.0f : y0;
		y0 = y0 < -1.0f ? -1.0f : y0;
		e.y = asinf(y0);

		// Yaw (z-axis rotation)
		float z0 = 2.0f*(w*z + x*y);
		float z1 = 1.0f - 2.0f*(y*y + z*z);
		e.z = atan2f(z0, z1);

		return e;
	}

	inline vec3 operator*(const vec3 &rhs) const
	{
		return vec3(
			rhs.x*(x*x + w*w - y*y - z*z) + rhs.y*(2.0f*x*y - 2.0f*w*z) + rhs.z*(2.0f*x*z + 2.0f*w*y),
			rhs.x*(2.0f*w*z + 2.0f*x*y) + rhs.y*(w*w - x*x + y*y - z*z) + rhs.z*(-2.0f*w*x + 2.0f*y*z),
			rhs.x*(-2.0f*w*y + 2.0f*x*z) + rhs.y*(2.0f*w*x + 2.0f*y*z)+ rhs.z*(w*w - x*x - y*y + z*z)
			);
	}

	inline quat operator*(const quat &rhs) const
	{
		return quat{x*rhs.w + w*rhs.x + y*rhs.z - z*rhs.y,
			y*rhs.w + w*rhs.y + z*rhs.x - x*rhs.z,
			z*rhs.w + w*rhs.z + x*rhs.y - y*rhs.x,
			w*rhs.w - x*rhs.x - y*rhs.y - z*rhs.z
		};
	}

	inline quat operator*(float rhs) const
	{
		return quat{x*rhs, y*rhs, z*rhs, w*rhs};
	}

	inline quat& operator*=(const quat &rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	inline void normalize()
	{
		float len = sqrtf(x*x + y*y + z*z + w*w);
		if(len == 0.0f) len = 1.0f;
		float ilen = 1.0f/len;
		x *= ilen;
		y *= ilen;
		z *= ilen;
		w *= ilen;
	}

	inline quat normalized() const
	{
		float len = sqrtf(x*x + y*y + z*z + w*w);
		if(len == 0.0f) len = 1.0f;
		float ilen = 1.0f/len;
		return quat{x*ilen, y*ilen, z*ilen, w*ilen};
	}

	inline float len() const
	{
		return sqrtf(x*x + y*y + z*z + w*w);
	}

	inline float sqrlen() const
	{
		return x*x + y*y + z*z + w*w;
	}

	inline void invert()
	{
		float t = 1.0f / sqrlen();
		x = -x * t;
		y = -y * t;
		z = -z * t;
	}

	inline quat inverse() const
	{
		quat q(*this);
		q.invert();
		return q;
	}

	inline static quat identity(){return quat{0.0f, 0.0f, 0.0f, 1.0f};}
	inline static quat euler(float x, float y, float z)
	{
		quat result;
		float xr = x * DEG2RAD;
		float yr = y * DEG2RAD;
		float zr = z * DEG2RAD;

		float x0 = cosf(xr * 0.5f);
		float x1 = sinf(xr * 0.5f);
		float y0 = cosf(yr * 0.5f);
		float y1 = sinf(yr * 0.5f);
		float z0 = cosf(zr * 0.5f);
		float z1 = sinf(zr * 0.5f);

		result.x = x1 * y0 * z0 - x0 * y1 * z1;
		result.y = x0 * y1 * z0 + x1 * y0 * z1;
		result.z = x0 * y0 * z1 - x1 * y1 * z0;
		result.w = x0 * y0 * z0 + x1 * y1 * z1;

		return result;
	}
	inline static quat euler(const vec3 &v){return euler(v.x, v.y, v.z);}

	inline static quat look(const vec3 &dir, const vec3 &up);

	inline static quat from_to(const vec3 &from, const vec3 &to)
	{
		float theta = vec3::dot(from, to);
		if(theta == 0.0f){return quat::identity();}
		vec3 cross = vec3::cross(from, to);

		quat res = {cross.x, cross.y, cross.z, 1.0f + theta};
		return res.normalized();
	}


	inline static quat axis_angle(const vec3 &axis, float angle)
	{
		quat q;
		float axis_len = axis.len();

		if(axis_len == 0.0f) return q;

		angle = angle * DEG2RAD;
		angle *= 0.5f;

		vec3 a = axis.normalized();
		float sinres = sinf(angle);
		float cosres = cosf(angle);

		q.x = a.x * sinres;
		q.y = a.y * sinres;
		q.z = a.z * sinres;
		q.w = cosres;

		return q.normalized();
	}

	inline static quat lerp(const quat &from, const quat &to, float t)
	{
		t = t < 1.0f ? t : 1.0f;
		quat q = {
			from.x + (to.x - from.x) * t,
			from.y + (to.y - from.y) * t,
			from.z + (to.z - from.z) * t,
			from.w + (to.w - from.w) * t
		};
		return q;
	}

	inline static quat nlerp(const quat &from, const quat &to, float t)
	{
		t = t < 1.0f ? t : 1.0f;
		quat q = lerp(from, to, t);
		float len = q.len();
		if(len == 0.0f) len = 1.0f;
		float ilen = 1.0f / len;
		q.x = q.x * ilen;
		q.y = q.y * ilen;
		q.z = q.z * ilen;
		q.w = q.w * ilen;
		return q;
	}

	inline static quat slerp(const quat &from, const quat &to, float t)
	{
		t = t < 1.0f ? t : 1.0f;
		quat q2 = to;
		quat q;

		float cosHalfTheta = from.x*q2.x + from.y*q2.y + from.z*q2.z + from.w*q2.w;

		if (cosHalfTheta < 0)
		{
			q2.x = -q2.x; q2.y = -q2.y; q2.z = -q2.z; q2.w = -q2.w;
			cosHalfTheta = -cosHalfTheta;
		}

		if (fabsf(cosHalfTheta) >= 1.0f) q = from;
		else if (cosHalfTheta > 0.95f) q = nlerp(from, q2, t);
		else
		{
			float halfTheta = acosf(cosHalfTheta);
			float sinHalfTheta = sqrtf(1.0f - cosHalfTheta*cosHalfTheta);

			if (fabsf(sinHalfTheta) < 0.001f)
			{
				q.x = (from.x*0.5f + q2.x*0.5f);
				q.y = (from.y*0.5f + q2.y*0.5f);
				q.z = (from.z*0.5f + q2.z*0.5f);
				q.w = (from.w*0.5f + q2.w*0.5f);
			}
			else
			{
				float ratioa = sinf((1 - t)*halfTheta)/sinHalfTheta;
				float ratiob = sinf(t*halfTheta)/sinHalfTheta;

				q.x = (from.x*ratioa + q2.x*ratiob);
				q.y = (from.y*ratioa + q2.y*ratiob);
				q.z = (from.z*ratioa + q2.z*ratiob);
				q.w = (from.w*ratioa + q2.w*ratiob);
			}
		}

		return q;
	}

	inline static float angle(const quat &q1, const quat &q2)
	{
		float dot = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
		dot = fminf(fabsf(dot), 1.0F);
		return (dot > 1.0f - EPSILON) ? 0.0f : acosf(dot) * 2.0f * RAD2DEG;
	}

	// ref https://twitter.com/FreyaHolmer/status/1596955953446137856
	inline vec3 right() const
	{
		return -vec3(
			x * x - y * y - z * z + w * w,
			2 * (x * y + z * w),
			2 * (x * z - y * w)
		);
	}

	inline vec3 up() const
	{
		return vec3(
			2 * (x * y - z * w),
			-x * x + y * y - z * z + w * w,
			2 * (x * w + y * z)
		);
	}

	inline vec3 forward() const
	{
		return vec3(
			2 * (x * z + y * w),
			2 * (y * z - x * w),
			-x * x - y * y + z * z + w * w
		);
	}
};

struct mat4
{
	union
	{
		float m[16];
		struct{float m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44;};
	};

	mat4():m11(0), m12(0), m13(0), m14(0), m21(0), m22(0), m23(0), m24(0), m31(0), m32(0), m33(0), m34(0), m41(0), m42(0), m43(0), m44(0){}
	mat4(float *f) : m11(f[0]), m12(f[1]), m13(f[2]), m14(f[3]), m21(f[4]), m22(f[5]), m23(f[6]), m24(f[7]), m31(f[8]), m32(f[9]), m33(f[10]), m34(f[11]), m41(f[12]), m42(f[13]), m43(f[14]), m44(f[15]){}

	mat4(const vec3 &pos, const quat &rot, const vec3 scale)
	{
		float qx = rot.x;
		float qy = rot.y;
		float qz = rot.z;
		float qw = rot.w;

		m[0] = (1 - 2 * qy*qy - 2 * qz*qz) * scale.x;
		m[1] = (2 * qx*qy + 2 * qz*qw) * scale.x;
		m[2] = (2 * qx*qz - 2 * qy*qw) * scale.x;
		m[3] = 0.0f;

		m[4] = (2 * qx*qy - 2 * qz*qw) * scale.y;
		m[5] = (1 - 2 * qx*qx - 2 * qz*qz) * scale.y;
		m[6] = (2 * qy*qz + 2 * qx*qw) * scale.y;
		m[7] = 0.0f;

		m[8] = (2 * qx*qz + 2 * qy*qw) * scale.z;
		m[9] = (2 * qy*qz - 2 * qx*qw) * scale.z;
		m[10] = (1 - 2 * qx*qx - 2 * qy*qy) * scale.z;
		m[11] = 0.0f,

		m[12] = pos.x;
		m[13] = pos.y;
		m[14] = pos.z;
		m[15] = 1.0f;
	}

	mat4 inversed() const
	{
		// Cache the matrix values (speed optimization)
		float a00 = m11, a01 = m12, a02 = m13, a03 = m14;
		float a10 = m21, a11 = m22, a12 = m23, a13 = m24;
		float a20 = m31, a21 = m32, a22 = m33, a23 = m34;
		float a30 = m41, a31 = m42, a32 = m43, a33 = m44;

		float b00 = a00*a11 - a01*a10;
		float b01 = a00*a12 - a02*a10;
		float b02 = a00*a13 - a03*a10;
		float b03 = a01*a12 - a02*a11;
		float b04 = a01*a13 - a03*a11;
		float b05 = a02*a13 - a03*a12;
		float b06 = a20*a31 - a21*a30;
		float b07 = a20*a32 - a22*a30;
		float b08 = a20*a33 - a23*a30;
		float b09 = a21*a32 - a22*a31;
		float b10 = a21*a33 - a23*a31;
		float b11 = a22*a33 - a23*a32;

		// Calculate the invert determinant (inlined to avoid double-caching)
		float invDet = 1.0f/(b00*b11 - b01*b10 + b02*b09 + b03*b08 - b04*b07 + b05*b06);

		mat4 res;
		res.m[0] = (a11*b11 - a12*b10 + a13*b09)*invDet;
		res.m[1] = (-a01*b11 + a02*b10 - a03*b09)*invDet;
		res.m[2] = (a31*b05 - a32*b04 + a33*b03)*invDet;
		res.m[3] = (-a21*b05 + a22*b04 - a23*b03)*invDet;
		res.m[4] = (-a10*b11 + a12*b08 - a13*b07)*invDet;
		res.m[5] = (a00*b11 - a02*b08 + a03*b07)*invDet;
		res.m[6] = (-a30*b05 + a32*b02 - a33*b01)*invDet;
		res.m[7] = (a20*b05 - a22*b02 + a23*b01)*invDet;
		res.m[8] = (a10*b10 - a11*b08 + a13*b06)*invDet;
		res.m[9] = (-a00*b10 + a01*b08 - a03*b06)*invDet;
		res.m[10] = (a30*b04 - a31*b02 + a33*b00)*invDet;
		res.m[11] = (-a20*b04 + a21*b02 - a23*b00)*invDet;
		res.m[12] = (-a10*b09 + a11*b07 - a12*b06)*invDet;
		res.m[13] = (a00*b09 - a01*b07 + a02*b06)*invDet;
		res.m[14] = (-a30*b03 + a31*b01 - a32*b00)*invDet;
		res.m[15] = (a20*b03 - a21*b01 + a22*b00)*invDet;

		return res;
	}

	// code from raylib
	quat to_quat() const
	{
		quat res;

		float fourWSquaredMinus1 = m[0] + m[5] + m[10];
		float fourXSquaredMinus1 = m[0] - m[5] - m[10];
		float fourYSquaredMinus1 = m[5] - m[0] - m[10];
		float fourZSquaredMinus1 = m[10] - m[0] - m[5];

		int biggestIndex = 0;
		float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
		if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourXSquaredMinus1;
			biggestIndex = 1;
		}

		if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourYSquaredMinus1;
			biggestIndex = 2;
		}

		if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourZSquaredMinus1;
			biggestIndex = 3;
		}

		float biggestVal = sqrtf(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
		float mult = 0.25f / biggestVal;

		switch (biggestIndex)
		{
		case 0:
			res.w = biggestVal;
			res.x = (m[6] - m[9]) * mult;
			res.y = (m[8] - m[2]) * mult;
			res.z = (m[1] - m[4]) * mult;
			break;
		case 1:
			res.x = biggestVal;
			res.w = (m[6] - m[9]) * mult;
			res.y = (m[1] + m[4]) * mult;
			res.z = (m[8] + m[2]) * mult;
			break;
		case 2:
			res.y = biggestVal;
			res.w = (m[8] - m[2]) * mult;
			res.x = (m[1] + m[4]) * mult;
			res.z = (m[6] + m[9]) * mult;
			break;
		case 3:
			res.z = biggestVal;
			res.w = (m[1] - m[4]) * mult;
			res.x = (m[8] + m[2]) * mult;
			res.y = (m[6] + m[9]) * mult;
			break;
		}

		return res;
	}

	vec3 multiply_point_3x4(const vec3 &point) const
	{
		vec3 res;
		res.x = m11 * point.x + m21 * point.y + m31 * point.z + m41;
		res.y = m12 * point.x + m22 * point.y + m32 * point.z + m42;
		res.z = m13 * point.x + m23 * point.y + m33 * point.z + m43;
		return res;
	}

	vec3 multiply_point(const vec3 &point) const
	{
		vec3 res;
		float w = 0.0f;
		res.x = m11 * point.x + m21 * point.y + m31 * point.z + m41;
		res.y = m12 * point.x + m22 * point.y + m32 * point.z + m42;
		res.z = m13 * point.x + m23 * point.y + m33 * point.z + m43;
		w = m14 * point.x + m24 * point.y + m34 * point.z + m44;
		w = 1.0f / w;
		res.x *= w;
		res.y *= w;
		res.z *= w;
		return res;
	}

	// transform direction only
	vec3 multiply_vector(const vec3 &vector) const
	{
		vec3 res;
		res.x = m11 * vector.x + m21 * vector.y + m31 * vector.z;
		res.y = m12 * vector.x + m22 * vector.y + m32 * vector.z;
		res.z = m13 * vector.x + m23 * vector.y + m33 * vector.z;
		return res;
	}

	static mat4 translate(vec3 pos)
	{
		mat4 m = mat4::identity();
		m.m41 = pos.x; m.m42 = pos.y; m.m43 = pos.z; m.m44 = 1.0f;
		return m;
	}

	static mat4 identity()
	{
		mat4 m;
		m.m11 = 1.0f; m.m22 = 1.0f; m.m33 = 1.0f; m.m44 = 1.0f;
		return m;
	}

	static mat4 perspective(float fovy, float aspect, float near, float far)
	{
		mat4 m;

		float top = near * tanf(fovy * 0.5f);
		float bottom = -top;
		float right = top * aspect;
		float left = -right;

		// MatrixFrustum(-right, right, -top, top, near, far);
		float rl = (float)(right - left);
		float tb = (float)(top - bottom);
		float fn = (float)(far - near);

		m.m[0] = ((float)near * 2.0f) / rl;
		m.m[5] = ((float)near * 2.0f) / tb;
		m.m[8] = ((float)right + (float)left) / rl;
		m.m[9] = ((float)top + (float)bottom) / tb;
		m.m[10]= -((float)far + (float)near) / fn;
		m.m[11] = -1.0f;
		m.m[14] = ((float)far * (float)near * -2.0f) / fn;

		return m;
	};

	static mat4 ortho(float left, float right, float bottom, float top, float near, float far)
	{
		mat4 m;
		m.m11 = 2.0f / (right - left);
		m.m22 = 2.0f / (top - bottom);
		m.m33 = -2.0f / (far - near);
		m.m44 = 1.0f;

		m.m41 = -(left + right) / (right - left);
		m.m42 = -(top + bottom) / (top - bottom);
		m.m43 = -(far + near) / (far - near);
		return m;
	};

	static mat4 lookat(const vec3 &pos, const vec3 &target, const vec3 &up)
	{
		vec3 f = (target - pos).normalized() * -1.0f;
		vec3 r =  vec3::cross(up, f).normalized();
		if(r.x == 0.0f && r.y == 0.0f && r.z == 0.0f){return lookat(pos, pos-vec3(0,0,-1), vec3(0,1,0));}// Error
		vec3 u = vec3::cross(f, r).normalized();
		mat4 mat;
		mat.m[0] = r.x;
		mat.m[1] = u.x;
		mat.m[2] = f.x;
		mat.m[3] = 0.0f;

		mat.m[4] = r.y;
		mat.m[5] = u.y;
		mat.m[6] = f.y;
		mat.m[7] = 0.0f;

		mat.m[8] = r.z;
		mat.m[9] = u.z;
		mat.m[10] = f.z;
		mat.m[11] = 0.0f;

		mat.m[12] = -vec3::dot(r, pos);
		mat.m[13] = -vec3::dot(u, pos);
		mat.m[14] = -vec3::dot(f, pos);
		mat.m[15] = 1.0f;
		return mat;
	}

	inline vec3 get_position() const
	{
		return vec3(m41, m42, m43);
	}
};



inline mat4 quat::to_mat4() const
{
	float a2 = x * x;
	float b2 = y * y;
	float c2 = z * z;
	float ac = x * z;
	float ab = x * y;
	float bc = y * z;
	float ad = w * x;
	float bd = w * y;
	float cd = w * z;

	mat4 res = mat4::identity();
	res.m[0] = 1.0f - 2.0f * (b2 + c2);
	res.m[1] = 2.0f * (ab + cd);
	res.m[2] = 2.0f * (ac - bd);

	res.m[4] = 2.0f * (ab - cd);
	res.m[5] = 1.0f - 2.0f * (a2 + c2);
	res.m[6] = 2.0f * (bc + ad);

	res.m[8] = 2.0f * (ac + bd);
	res.m[9] = 2.0f * (bc - ad);
	res.m[10] = 1.0f - 2.0f * (a2 + b2);

	return res;
}

inline quat quat::look(const vec3 &dir, const vec3 &up)
{
	return mat4::lookat(vec3(), -dir.normalized(), up).inversed().to_quat();
}


inline mat4 operator*(const mat4 &lhs, const mat4 &rhs)
{
	mat4 m;
	m.m11 = lhs.m11 * rhs.m11 + lhs.m12 * rhs.m21 + lhs.m13 * rhs.m31 + lhs.m14 * rhs.m41;
	m.m12 = lhs.m11 * rhs.m12 + lhs.m12 * rhs.m22 + lhs.m13 * rhs.m32 + lhs.m14 * rhs.m42;
	m.m13 = lhs.m11 * rhs.m13 + lhs.m12 * rhs.m23 + lhs.m13 * rhs.m33 + lhs.m14 * rhs.m43;
	m.m14 = lhs.m11 * rhs.m14 + lhs.m12 * rhs.m24 + lhs.m13 * rhs.m34 + lhs.m14 * rhs.m44;

	m.m21 = lhs.m21 * rhs.m11 + lhs.m22 * rhs.m21 + lhs.m23 * rhs.m31 + lhs.m24 * rhs.m41;
	m.m22 = lhs.m21 * rhs.m12 + lhs.m22 * rhs.m22 + lhs.m23 * rhs.m32 + lhs.m24 * rhs.m42;
	m.m23 = lhs.m21 * rhs.m13 + lhs.m22 * rhs.m23 + lhs.m23 * rhs.m33 + lhs.m24 * rhs.m43;
	m.m24 = lhs.m21 * rhs.m14 + lhs.m22 * rhs.m24 + lhs.m23 * rhs.m34 + lhs.m24 * rhs.m44;

	m.m31 = lhs.m31 * rhs.m11 + lhs.m32 * rhs.m21 + lhs.m33 * rhs.m31 + lhs.m34 * rhs.m41;
	m.m32 = lhs.m31 * rhs.m12 + lhs.m32 * rhs.m22 + lhs.m33 * rhs.m32 + lhs.m34 * rhs.m42;
	m.m33 = lhs.m31 * rhs.m13 + lhs.m32 * rhs.m23 + lhs.m33 * rhs.m33 + lhs.m34 * rhs.m43;
	m.m34 = lhs.m31 * rhs.m14 + lhs.m32 * rhs.m24 + lhs.m33 * rhs.m34 + lhs.m34 * rhs.m44;

	m.m41 = lhs.m41 * rhs.m11 + lhs.m42 * rhs.m21 + lhs.m43 * rhs.m31 + lhs.m44 * rhs.m41;
	m.m42 = lhs.m41 * rhs.m12 + lhs.m42 * rhs.m22 + lhs.m43 * rhs.m32 + lhs.m44 * rhs.m42;
	m.m43 = lhs.m41 * rhs.m13 + lhs.m42 * rhs.m23 + lhs.m43 * rhs.m33 + lhs.m44 * rhs.m43;
	m.m44 = lhs.m41 * rhs.m14 + lhs.m42 * rhs.m24 + lhs.m43 * rhs.m34 + lhs.m44 * rhs.m44;
	return m;
}

inline vec4 operator*(const mat4 &lhs, const vec4 &rhs)
{
	vec4 result;
	result.x = lhs.m[0]*rhs.x + lhs.m[4]*rhs.y + lhs.m[8]*rhs.z + lhs.m[12]*rhs.w;
	result.y = lhs.m[1]*rhs.x + lhs.m[5]*rhs.y + lhs.m[9]*rhs.z + lhs.m[13]*rhs.w;
	result.z = lhs.m[2]*rhs.x + lhs.m[6]*rhs.y + lhs.m[10]*rhs.z + lhs.m[14]*rhs.w;
	result.w = lhs.m[3]*rhs.x + lhs.m[7]*rhs.y + lhs.m[11]*rhs.z + lhs.m[15]*rhs.w;
	return result;
}


struct transform_t
{
	vec3 pos;
	quat rot;
	vec3 scale;

	transform_t() : pos(vec3(0,0,0)), rot(quat::identity()), scale(vec3(1,1,1)){}
	transform_t(const vec3 &pos, const quat &rot, const vec3 &scale) : pos(pos), rot(rot), scale(scale){}

	mat4 to_mat4() const {return mat4(pos, rot, scale);}
};




struct ray_t
{
	vec3 pos;
	vec3 dir;
	ray_t(){}
	ray_t(const vec3 &pos, const vec3 &dir) : pos(pos), dir(dir){}
};


struct bounds_t
{
	vec3 center;
	vec3 extents; // half of the size of the Bounds.

	bounds_t() : center(vec3()), extents(vec3()){}
	bounds_t(const vec3 &center, const vec3 &extents) : center(center), extents(extents){}

	void set_min_max(const vec3 &min, const vec3 &max)
	{
		center = (min + max) * 0.5f;
		extents = (max - min) * 0.5f;
	}

	inline vec3 get_min()const{return center - extents;}
	inline vec3 get_max()const{return center + extents;}
	inline vec3 get_size()const{return extents * 2.0f;}
	void encapsulate(const vec3 &point)
	{
		vec3 min = center - extents;
		vec3 max = center + extents;
		if(point.x < min.x) {
			min.x = point.x;
		} else if(point.x > max.x) {
			max.x = point.x;
		}

		if(point.y < min.y) {
			min.y = point.y;
		} else if(point.y > max.y) {
			max.y = point.y;
		}

		if(point.z < min.z) {
			min.z = point.z;
		} else if(point.z > max.z) {
			max.z = point.z;
		}

		center = (min + max) * 0.5f;
		extents = (max - min) * 0.5f;
	}

	void encapsulate(const bounds_t &bounds)
	{
		encapsulate(bounds.get_min());
		encapsulate(bounds.get_max());
	}

	bool intersects(const bounds_t &other) const
	{
		vec3 a_min = get_min();
		vec3 a_max = get_max();
		vec3 b_min = other.get_min();
		vec3 b_max = other.get_max();
		return	(a_min.x <= b_max.x && a_max.x >= b_min.x &&
				 a_min.y <= b_max.y && a_max.x >= b_min.x &&
				 a_min.z <= b_max.z && a_max.z >= b_min.z);
	}

	bool containts(const vec3 &point) const
	{
		vec3 min = get_min();
		vec3 max = get_max();
		return	(point.x >= min.x && point.x <= max.x &&
				 point.y >= min.y && point.y <= max.y &&
				 point.z >= min.z && point.z <= max.z);
	}

	bool containts(const bounds_t &other) const
	{
		vec3 a_min = get_min();
		vec3 a_max = get_max();
		vec3 b_min = other.get_min();
		vec3 b_max = other.get_max();
		return	(a_min.x <= b_min.x && a_max.x >= b_max.x &&
				 a_min.y <= b_min.y && a_max.y >= b_max.y &&
				 a_min.z <= b_min.z && a_max.z >= b_max.z);
	}
};


struct plane_t
{
	plane_t(const vec3 &origin, const vec3 &normal) : origin(origin), normal(normal){}
	plane_t(const vec3 &p1, const vec3 &p2, const vec3 &p3)
	{
		origin = p1;
		normal = vec3::cross(p2-p1, p3-p1).normalized();
	}

	bool get_side(const vec3 &point)
	{
		return vec3::dot(normal, (point - origin).normalized()) >= 0;
	}

	float signed_distance_to(const vec3 &point)
	{
		return vec3::dot(point, normal) - vec3::dot(normal, origin);
	}

	vec3 origin;
	vec3 normal;
};

static inline float lerp(float from, float to, float t)
{
	return (1 - t) * from + t * to;
}

static inline vec3 lerp(const vec3 &from, const vec3 &to, float t)
{
	return (from * (1.0f - t)) + (to * t);
}