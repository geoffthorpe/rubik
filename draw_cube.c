#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/freeglut.h>

#include <rubik/rubik.h>
#include <rubik/vector.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

/* Mapping from r_color to 3-float RGB */
static float r2c[6][3] = {
	{ 1, 1, 1 }, // white
	{ 1, 1, 0 }, // yellow
	{ 0, 0, 1 }, // blue
	{ 0, 1, 0 }, // green
	{ 1, 0, 0 }, // red
	{ 1, 0.5, 0 }, // orange
};

/* Mapping from square index (0..7) to x, y (where x and y are -1, 0, 1) */
static int sq2pos[8][2] = {
	{-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}};

/* The squares are separated from each other and from the edges by 'GAP'. */
#define GAP 0.1
#define WIDTH ((2.0 - 4*GAP) / 3.0)
#define DEPTH 0.05
#define GLOWSPEED 100 /* milliseconds, cycle is 4 times this number */

static inline void set_color(enum r_color c, int is_side, unsigned int glow)
{
	int d = is_side ? 2 : 1;
	float r = r2c[c][0] / d;
	float g = r2c[c][1] / d;
	float b = r2c[c][2] / d;
	if (glow < (2 * GLOWSPEED)) {
		float m = (glow < GLOWSPEED) ?
			(float)(GLOWSPEED - glow) / GLOWSPEED:
			(float)(glow - GLOWSPEED) / GLOWSPEED;
		r *= m;
		g *= m;
		b *= m;
	} else {
		float m = (glow < (3 * GLOWSPEED)) ?
			(float)((3 * GLOWSPEED) - glow) / GLOWSPEED:
			(float)(glow - (3 * GLOWSPEED)) / GLOWSPEED;
		r = 1 - (1 - r) * m;
		g = 1 - (1 - g) * m;
		b = 1 - (1 - b) * m;
	}
	glColor3f(r, g, b);
}

/* So, we assume the caller has set up the reference frame so that we can draw our
 * cube to the "standard position", namely;
 * - in the unit square, from (-1,-1,-1) to (1,1,1);
 * - red on top (+y)
 * - orange on bottom (-y)
 * - blue on front (+z)
 * - green on back (-z)
 * - yellow on left (-x)
 * - white on right (+x)
 * Don't forget, we're in a right-handed system, so +x is right, +y is up, and
 * +z is out of the screen towards the viewer.
 *
 * When drawing each face, we provide the unit vectors that locally (to the face)
 * represent +z (normal), +x (from left to right), +y (from bottom to top), and
 * the neighbour color that is "top" from this representation.
 *
 * Our other job is to glow the "current_color" side, which we do using a
 * 4-cycle pattern of color adjustments.
 *
 */
struct cface {
	enum r_color top; // top-side neighbour
	vector x, y, z;
};
static struct cface cfaces[6] = {
	// white
	{ r_color_red, DECLARE_VECTOR(0, 0, -1), DECLARE_VECTOR(0, 1, 0),
			DECLARE_VECTOR(1, 0, 0) },
	// yellow
	{ r_color_red, DECLARE_VECTOR(0, 0, 1), DECLARE_VECTOR(0, 1, 0),
			DECLARE_VECTOR(-1, 0, 0) },
	// blue
	{ r_color_red, DECLARE_VECTOR(1, 0, 0), DECLARE_VECTOR(0, 1, 0),
			DECLARE_VECTOR(0, 0, 1) },
	// green
	{ r_color_red, DECLARE_VECTOR(-1, 0, 0), DECLARE_VECTOR(0, 1, 0),
			DECLARE_VECTOR(0, 0, -1) },
	// red
	{ r_color_green, DECLARE_VECTOR(1, 0, 0), DECLARE_VECTOR(0, 0, -1),
			DECLARE_VECTOR(0, 1, 0) },
	// orange
	{ r_color_green, DECLARE_VECTOR(-1, 0, 0), DECLARE_VECTOR(0, 0, -1),
			DECLARE_VECTOR(0, -1, 0) }
};

static void square_vector(vector *v, int x, int y,
			  int backside, int right, int bottom,
			  struct cface *cface)
{
	vector_copy(v, &cface->z);
	if (!backside)
		vector_scale(v, 1 + DEPTH);
	if (right)
		vector_addscale(v, &cface->x, WIDTH / 2 + x * (WIDTH + GAP));
	else
		vector_addscale(v, &cface->x, -WIDTH / 2 + x * (WIDTH + GAP));
	if (bottom)
		vector_addscale(v, &cface->y, -WIDTH / 2 + y * (WIDTH + GAP));
	else
		vector_addscale(v, &cface->y, WIDTH / 2 + y * (WIDTH + GAP));
}

static void set_normal(vector *v, int flip)
{
	if (flip)
		glNormal3f(-(*v)[0], -(*v)[1], -(*v)[2]);
	else
		glNormal3fv(&(*v)[0]);
}

void square_render(enum r_color c, int x, int y, int sel, unsigned int glow,
			struct cface *cface, vector *normal, float cwturn)
{
	if (sel && cwturn) {
		glPushMatrix();
		glRotatef(-cwturn, (*normal)[0], (*normal)[1], (*normal)[2]);
	}
	if (!sel) {
		glow = 0;
		cwturn = 0;
	}
	/* We prepare the 8 vectors of the 'square'. The first four are the bright-colored
	 * face that are (1+DEPTH) from the origin in the +z direction, the second four are
	 * the corresponding points on the backside of the square. */
	vector v[8];
	square_vector(&v[0], x, y, 0, 0, 0, cface);
	square_vector(&v[1], x, y, 0, 1, 0, cface);
	square_vector(&v[2], x, y, 0, 1, 1, cface);
	square_vector(&v[3], x, y, 0, 0, 1, cface);
	square_vector(&v[4], x, y, 1, 0, 0, cface);
	square_vector(&v[5], x, y, 1, 1, 0, cface);
	square_vector(&v[6], x, y, 1, 1, 1, cface);
	square_vector(&v[7], x, y, 1, 0, 1, cface);
	glBegin(GL_QUADS);
	/* Bright front; +y */
	set_color(c, 0, glow);
	set_normal(&cface->z, 0);
	glVertex3fv(v[0]);
	glVertex3fv(v[3]);
	glVertex3fv(v[2]);
	glVertex3fv(v[1]);
	/* Dark back; -y */
	set_color(c, 1, glow);
	set_normal(&cface->z, 1);
	glVertex3fv(v[4]);
	glVertex3fv(v[5]);
	glVertex3fv(v[6]);
	glVertex3fv(v[7]);
	/* Dark left; -x */
	set_color(c, 1, glow);
	set_normal(&cface->x, 1);
	glVertex3fv(v[0]);
	glVertex3fv(v[4]);
	glVertex3fv(v[7]);
	glVertex3fv(v[3]);
	/* Dark right; +x */
	set_color(c, 1, glow);
	set_normal(&cface->x, 0);
	glVertex3fv(v[5]);
	glVertex3fv(v[1]);
	glVertex3fv(v[2]);
	glVertex3fv(v[6]);
	/* Dark top; +y */
	set_color(c, 1, glow);
	set_normal(&cface->y, 0);
	glVertex3fv(v[0]);
	glVertex3fv(v[1]);
	glVertex3fv(v[5]);
	glVertex3fv(v[4]);
	/* Dark bottom; -y */
	set_color(c, 1, glow);
	set_normal(&cface->y, 1);
	glVertex3fv(v[7]);
	glVertex3fv(v[6]);
	glVertex3fv(v[2]);
	glVertex3fv(v[3]);
	glEnd();
	if (sel && cwturn)
		glPopMatrix();
}

void face_render(struct r_face *face, struct cface *cface, unsigned int glow,
		 vector *normal, float cwturn)
{
	r_face_reorient(face, cface->top);
	enum r_color *c = r_face_get_squares(face);
	enum r_color center = r_face_color(face);
	enum r_color current_color = r_cube_color(r_face_cube(face));
	int sq;
	/* Render the 8 squares around the center */
	for (sq = 0; sq < 8; sq++, c++) {
		int x = sq2pos[sq][0];
		int y = sq2pos[sq][1];
		int sel = 0;
		/* Only pass a non-zero glow/turn if the square really should glow/turn */
		if ((center == current_color) ||
				((r_color_invert(center) != current_color) &&
				r_face_adjacent(face, current_color, sq)))
			sel = 1;
		square_render(*c, x, y, sel, glow, cface, normal, cwturn);
	}
	r_face_put_squares(face);
	/* Render the center square */
	square_render(center, 0, 0, center == current_color, glow, cface,
		      normal, cwturn);
}

/* The paille vectors are interesting. They're actually vectors made out of
 * vector coefficients, because when they get used, each x/y/z component is
 * used as a scalar multiplier of a unit vector. So we arbitrarily choose to
 * make 'x' be the multiplier of the normal, and let y and z be the multipliers
 * of the two other axis vectors (spanning the face of the cube). */
#define PAILLE_RADIUS 0.1
#define PAILLE_LENGTH 2.0
#define PAILLE_SUBDIV_ROUND 42
static float _paille[PAILLE_SUBDIV_ROUND][2];
static inline float paille_get(unsigned int subdiv, int isy)
{ return _paille[subdiv][isy]; }
static inline void paille_set(unsigned int subdiv, int isy, float v)
{ _paille[subdiv][isy] = v * PAILLE_RADIUS; }
/* Wrap set_color() in a rainbow remapping */
static void paille_color(enum r_color color, int is_side)
{
	static enum r_color map[] = {
		r_color_white,
		r_color_red,
		r_color_orange,
		r_color_yellow,
		r_color_green,
		r_color_blue
	};
	set_color(map[color], is_side, 0);
}

void paille_render(struct cface *cface)
{
	glBegin(GL_QUADS);
	unsigned int subdiv;
	for (subdiv = 0; subdiv < PAILLE_SUBDIV_ROUND; subdiv++) {
		vector v1, v2, v3, v4, normal, anormal;
		vector_init(&v1, 0, 0, 0);
		vector_init(&v2, 0, 0, 0);
		vector_addscale(&v1, &cface->x, paille_get(subdiv, 0));
		vector_addscale(&v1, &cface->y, paille_get(subdiv, 1));
		vector_addscale(&v2, &cface->x,
			paille_get((subdiv+1)%PAILLE_SUBDIV_ROUND, 0));
		vector_addscale(&v2, &cface->y,
			paille_get((subdiv+1)%PAILLE_SUBDIV_ROUND, 1));
		vector_midpoint(&normal, &v1, &v2);
		vector_copy(&anormal, &normal);
		vector_scale(&anormal, -1);
		vector_addscale(&v1, &cface->z, 1 + DEPTH);
		vector_addscale(&v2, &cface->z, 1 + DEPTH);
		vector_copy(&v3, &v2);
		vector_copy(&v4, &v1);
		vector_addscale(&v3, &cface->z, PAILLE_LENGTH);
		vector_addscale(&v4, &cface->z, PAILLE_LENGTH);
		enum r_color color1 = subdiv % 6;
		enum r_color color2 = (color1 + 1) % 6;
		glNormal3fv(&normal[0]);
		paille_color(color1, 0);
		glVertex3fv(&v1[0]);
		paille_color(color2, 0);
		glVertex3fv(&v2[0]);
		glVertex3fv(&v3[0]);
		paille_color(color1, 0);
		glVertex3fv(&v4[0]);
		glNormal3fv(&anormal[0]);
		paille_color(color2, 1);
		glVertex3fv(&v2[0]);
		paille_color(color1, 1);
		glVertex3fv(&v1[0]);
		glVertex3fv(&v4[0]);
		paille_color(color2, 1);
		glVertex3fv(&v3[0]);
	}
	glEnd();
}

void draw_cube_precalc(void)
{
	unsigned int subdiv;
	for (subdiv = 0; subdiv < PAILLE_SUBDIV_ROUND; subdiv++) {
		paille_set(subdiv, 0, cos(4 * M_PI * subdiv / PAILLE_SUBDIV_ROUND));
		paille_set(subdiv, 1, sin(4 * M_PI * subdiv / PAILLE_SUBDIV_ROUND));
	}
}

void r_cube_draw(struct r_cube *cube, int paille, int glowing, float cwturn)
{
	unsigned int glow = glowing ?
		glutGet(GLUT_ELAPSED_TIME) % (4 * GLOWSPEED) : 0;
	enum r_color color = r_color_first;
	struct cface *cface = &cfaces[0];
	vector *normal = &cfaces[r_cube_color(cube)].z;
	while (color < r_color_illegal) {
		struct r_face *face = r_cube_face(cube, color);
		face_render(face, cface, glow, normal, cwturn);
		color++; cface++;
	};
	if (paille) {
		cface = &cfaces[r_cube_color(cube)];
		paille_render(cface);
	}
}
