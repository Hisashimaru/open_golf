#include "mathf.h"

static u32 _rand_seed[4] = {123456789, 362436069, 521288629, 88675123};
static u32 _rand_geti()
{
	unsigned int t = (_rand_seed[0] ^ (_rand_seed[0] << 11));
	_rand_seed[0] = _rand_seed[1]; _rand_seed[1] = _rand_seed[2]; _rand_seed[2] = _rand_seed[3];
	return _rand_seed[3] = (_rand_seed[3] ^ (_rand_seed[3] >> 19)) ^ (t ^ (t >> 8));
}

void rand_set_seed(u32 seed)
{
	_rand_seed[3] = seed;
	for(int i=0; i<40; i++){_rand_geti();}
}

float rand_get()
{
	return (float)_rand_geti() / (float)0xffffffff;
}

// min(inclusive)  max(inclusive)
float rand_range(float min, float max)
{
	return (max - min) * rand_get() + min;
}

// min(inclusive)  max(exclusive)
int rand_range(int min, int max)
{
	return _rand_geti() % (max - min) + min;
}

// ref https://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly
vec2 rand_in_circle(float radius)
{
	float r = radius * sqrtf(rand_get());
	float theta = rand_get() * 2.0f * PI;
	return vec2(r * cosf(theta), r * sinf(theta));
}

// ref https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/
vec3 rand_in_sphere(float radius)
{
    float u = rand_get();
    float v = rand_get();
    float theta = u * 2.0f * PI;
    float phi = acosf(2.0f * v - 1.0f);
    float r = radius * cbrtf(rand_get());
    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);
    float sinPhi = sinf(phi);
    float cosPhi = cosf(phi);
    float x = r * sinPhi * cosTheta;
    float y = r * sinPhi * sinTheta;
    float z = r * cosPhi;
    return vec3(x, y, z);
}


vec3 rand_cone(const vec3 &dir, float cone_half_angle)
{
    float u = rand_get();
    float v = rand_get();
    float theta = u * 2.0f * PI;
    float phi = acosf(2.0f * v - 1.0f);
    phi = fmodf(phi, cone_half_angle * DEG2RAD);
    quat q = quat::from_to(vec3(0,0,1), dir.normalized());
    vec3 dir_z = q*vec3(0,0,1);//q.forward();
    vec3 dir_x = q*vec3(1,0,0);//q.right();
    vec3 res = quat::axis_angle(dir_x, phi * RAD2DEG) * dir;
    res = quat::axis_angle(dir_z, theta * RAD2DEG) * res;
    return res;
}