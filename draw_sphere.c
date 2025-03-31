#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/freeglut.h>

#include <rubik/common.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define SUBDIV 15
typedef float vector[3];
static void vector_add(vector *r, vector *a, vector *b)
{
	(*r)[0] = (*a)[0] + (*b)[0];
	(*r)[1] = (*a)[1] + (*b)[1];
	(*r)[2] = (*a)[2] + (*b)[2];
}
static void vector_cpy(vector *dst, vector *src)
{
	(*dst)[0] = (*src)[0];
	(*dst)[1] = (*src)[1];
	(*dst)[2] = (*src)[2];
}
static void vector_scale(vector *dst, float m)
{
	(*dst)[0] *= m;
	(*dst)[1] *= m;
	(*dst)[2] *= m;
}

static vector southpole = { 0, -1, 0 };
static vector northpole = { 0, 1, 0 };
static vector vertices[2*SUBDIV-1][4*SUBDIV];
void color_from_vertex(vector *vertex)
{
	vector v, v111 = { 1, 1, 1};
	vector_cpy(&v, vertex);
	vector_add(&v, &v, &v111);
	vector_scale(&v, 0.5);
	glColor3fv(&v[0]);
}
static void precalc_vertices(void)
{
	int i, j;
	for (i = 0; i < 2 * SUBDIV - 1; i++) {
		float yangle = (i + 1) * M_PI / (2 * SUBDIV);
		float y = -cos(yangle);
		float r = sin(yangle);
		for (j = 0; j < 4 * SUBDIV; j++) {
			float pangle = j * M_PI / (2 * SUBDIV);
			float x = r * cos(pangle);
			float z = -r * sin(pangle);
			vector *v = &vertices[i][j];
			(*v)[0] = x;
			(*v)[1] = y;
			(*v)[2] = z;
		}
	}
}
struct tri {
	vector *v[3];
	vector normal;
};
struct quad {
	vector *v[4];
	vector normal;
};
static struct tri tris[2][4*SUBDIV];
static struct quad quads[2*SUBDIV-2][4*SUBDIV];
void precalc_tris(void)
{
	int i, j;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4 * SUBDIV; j++) {
			struct tri *t = &tris[i][j];
			if (i) {
				t->v[0] = &northpole;
				t->v[1] = &vertices[2*SUBDIV-2][j];
				t->v[2] = &vertices[2*SUBDIV-2][(j+1)%(4*SUBDIV)];
			} else {
				t->v[0] = &southpole;
				t->v[2] = &vertices[0][j];
				t->v[1] = &vertices[0][(j+1)%(4*SUBDIV)];
			}
			vector_add(&t->normal, t->v[0], t->v[1]);
			vector_add(&t->normal, &t->normal, t->v[2]);
		}
	}
}
void precalc_quads(void)
{
	int i, j;
	for (i = 0; i < 2 * SUBDIV - 2; i++) {
		for (j = 0; j < 4 * SUBDIV; j++) {
			struct quad *q = &quads[i][j];
			q->v[0] = &vertices[i][j];
			q->v[1] = &vertices[i][(j+1)%(4*SUBDIV)];
			q->v[2] = &vertices[i+1][(j+1)%(4*SUBDIV)];
			q->v[3] = &vertices[i+1][j];
			vector_add(&q->normal, q->v[0], q->v[1]);
			vector_add(&q->normal, &q->normal, q->v[2]);
			vector_add(&q->normal, &q->normal, q->v[3]);
		}
	}
}
void draw_tris(void)
{
	int i, j;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4 * SUBDIV; j++) {
			struct tri *t = &tris[i][j];
			glNormal3fv(t->normal);
			color_from_vertex(t->v[0]);
			glVertex3fv(*t->v[0]);
			color_from_vertex(t->v[1]);
			glVertex3fv(*t->v[1]);
			color_from_vertex(t->v[2]);
			glVertex3fv(*t->v[2]);
		}
	}
}

void draw_quads(void)
{
	int i, j;
	for (i = 0; i < 2 * SUBDIV - 2; i++) {
		for (j = 0; j < 4 * SUBDIV; j++) {
			struct quad *q = &quads[i][j];
			glNormal3fv(q->normal);
			color_from_vertex(q->v[0]);
			glVertex3fv(*q->v[0]);
			color_from_vertex(q->v[1]);
			glVertex3fv(*q->v[1]);
			color_from_vertex(q->v[2]);
			glVertex3fv(*q->v[2]);
			color_from_vertex(q->v[3]);
			glVertex3fv(*q->v[3]);
		}
	}
}

void sphere_precalc(void)
{
	precalc_vertices();
	precalc_tris();
	precalc_quads();
}

void sphere_draw(void)
{
	glBegin(GL_TRIANGLES);
	draw_tris();
	glEnd();
	glBegin(GL_QUADS);
	draw_quads();
	glEnd();
}
