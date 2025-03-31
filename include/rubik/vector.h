#ifndef HEADER_RUBIK_VECTOR_H
#define HEADER_RUBIK_VECTOR_H

#include <rubik/common.h>

typedef float vector[3];

#define DECLARE_VECTOR(x,y,z) { x, y, z }
static inline void vector_init(vector *v, float x, float y, float z)
{
	(*v)[0] = x;
	(*v)[1] = y;
	(*v)[2] = z;
}
static inline void vector_copy(vector *dst, vector *src)
{
	(*dst)[0] = (*src)[0];
	(*dst)[1] = (*src)[1];
	(*dst)[2] = (*src)[2];
}
static inline void vector_scale(vector *v, float m)
{
	(*v)[0] *= m;
	(*v)[1] *= m;
	(*v)[2] *= m;
}
static inline void vector_addscale(vector *v, vector *a, float b)
{
	(*v)[0] += (*a)[0] * b;
	(*v)[1] += (*a)[1] * b;
	(*v)[2] += (*a)[2] * b;
}
static inline void vector_midpoint(vector *v, vector *a, vector *b)
{
	(*v)[0] = 0.5 * ((*a)[0] + (*b)[0]);
	(*v)[1] = 0.5 * ((*a)[1] + (*b)[1]);
	(*v)[2] = 0.5 * ((*a)[2] + (*b)[2]);
}

#endif /* !HEADER_RUBIK_VECTOR */
