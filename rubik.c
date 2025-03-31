#include <rubik/rubik.h>

static enum r_color cw_white[] = { r_color_blue, r_color_red, r_color_green, r_color_orange };
static enum r_color cw_yellow[] = { r_color_blue, r_color_orange, r_color_green, r_color_red };
static enum r_color cw_blue[] = { r_color_red, r_color_white, r_color_orange, r_color_yellow };
static enum r_color cw_green[] = { r_color_red, r_color_yellow, r_color_orange, r_color_white };
static enum r_color cw_red[] = { r_color_white, r_color_blue, r_color_yellow, r_color_green };
static enum r_color cw_orange[] = { r_color_white, r_color_green, r_color_yellow, r_color_blue };
enum r_color *r_clockwise[6] = { cw_white, cw_yellow,
				 cw_blue, cw_green,
				 cw_red, cw_orange };

/* We record each face based on its center color, leaving 8 squares. The
 * encoding depends on which neighboring side is "current"ly on the top (if we
 * are facing the face). We record the eight squares in clockwise order from
 * the top-left, meaning the first 3 squares are the row adjacent to the face
 * corresponding to "current". */
struct r_face {
	struct r_cube *cube;
	enum r_color color;
	enum r_color current;
	int locked;
	enum r_color squares[8];
};

static void r_face_init(struct r_face *f, struct r_cube *cube,
			enum r_color color)
{
	int i;
	f->cube = cube;
	f->color = color;
	f->current = r_clockwise[color][0];
	f->locked = 0;
	for (i = 0; i < 8; i++)
		f->squares[i] = color;
		//f->squares[i] = i < 2 ? r_color_invert(color) : color;
}

static void r_face_copy(struct r_face *dst, const struct r_face *src,
			struct r_cube *cube)
{
	int i;
	dst->cube = cube;
	dst->color = src->color;
	dst->current = src->current;
	for (i = 0; i < 8; i++)
		dst->squares[i] = src->squares[i];
}

struct r_cube *r_face_cube(struct r_face *f)
{
	return f->cube;
}

enum r_color r_face_color(struct r_face *f)
{
	return f->color;
}

static unsigned int _cw_turns(enum r_color c, enum r_color a, enum r_color b)
{
	int i, prev = -1, gap = -1;
	enum r_color *cw = r_clockwise[c];
	for (i = 0; i < 4; i++)
		if (cw[i] == a) {
			prev = i;
			break;
		}
	assert(i < 4);
	for (i = 0; i < 4; i++)
		if (cw[(i+prev) % 4] == b) {
			gap = i;
			break;
		}
	assert(i < 4);
	assert(prev >= 0 && gap >= 0);
	return gap;
}

static unsigned int _table_cw_turns[6][6][6];

static void cw_turn_precalc(void)
{
	enum r_color c, a, b;
	for (c = r_color_first; c < r_color_illegal; c++)
		for (a = r_color_first; a < r_color_illegal; a++)
			for (b = r_color_first; b < r_color_illegal; b++)
				_table_cw_turns[c][a][b] = _cw_turns(c, a, b);
}

unsigned int cw_turns(enum r_color c, enum r_color a, enum r_color b)
{
	return _table_cw_turns[c][a][b];
}

void r_face_reorient(struct r_face *f, enum r_color top)
{
	assert(!f->locked);
	unsigned int gap = cw_turns(f->color, f->current, top);
	/* The number of squares to "rotate" in a clockwise face is 2x, where x
	 * is the number of 90-degree turns. */
	switch (gap) {
	case 0:
		break;
	case 1:
	{
		enum r_color b1 = f->squares[0];
		enum r_color b2 = f->squares[1];
		f->squares[0] = f->squares[2];
		f->squares[1] = f->squares[3];
		f->squares[2] = f->squares[4];
		f->squares[3] = f->squares[5];
		f->squares[4] = f->squares[6];
		f->squares[5] = f->squares[7];
		f->squares[6] = b1;
		f->squares[7] = b2;
	}
		break;
	case 2:
	{
		enum r_color b1 = f->squares[0];
		enum r_color b2 = f->squares[1];
		enum r_color b3 = f->squares[2];
		enum r_color b4 = f->squares[3];
		f->squares[0] = f->squares[4];
		f->squares[1] = f->squares[5];
		f->squares[2] = f->squares[6];
		f->squares[3] = f->squares[7];
		f->squares[4] = b1;
		f->squares[5] = b2;
		f->squares[6] = b3;
		f->squares[7] = b4;
	}
		break;
	case 3:
	{
		enum r_color b1 = f->squares[7];
		enum r_color b2 = f->squares[6];
		f->squares[7] = f->squares[5];
		f->squares[6] = f->squares[4];
		f->squares[5] = f->squares[3];
		f->squares[4] = f->squares[2];
		f->squares[3] = f->squares[1];
		f->squares[2] = f->squares[0];
		f->squares[1] = b1;
		f->squares[0] = b2;
	}
		break;
	}
	f->current = top;
}

unsigned int r_face_iforient(struct r_face *f, enum r_color top, unsigned int idx)
{
	unsigned int gap = cw_turns(f->color, f->current, top);
	return (idx - 2 * gap) & 7;
}

enum r_color *r_face_get_squares(struct r_face *f)
{
	assert(!f->locked);
	f->locked = 1;
	return f->squares;
}

void r_face_put_squares(struct r_face *f)
{
	assert(f->locked);
	f->locked = 0;
}

int r_face_adjacent(struct r_face *f, enum r_color c, unsigned int idx)
{
	assert(r_color_invert(r_face_color(f)) != c);
	unsigned int ridx = r_face_iforient(f, c, idx);
	return ridx < 3;

}

struct r_cube {
	int ref;
	struct r_face faces[6];
	enum r_color current;
	int sig_valid;
	r_signature signature;
};

struct r_cube *r_cube(void)
{
	enum r_color color;
	struct r_cube *c = malloc(sizeof(*c));
	if (!c) return NULL;
	c->ref = 1;
	for (color = r_color_first; color < r_color_illegal; color++)
		r_face_init(&c->faces[color], c, color);
	c->current = r_color_first;
	c->sig_valid = 0;
	return c;
}

struct r_cube *r_cube_clone(const struct r_cube *_c)
{
	enum r_color color;
	struct r_cube *c = malloc(sizeof(*c));
	if (!c) return NULL;
	c->ref = 1;
	for (color = r_color_first; color < r_color_illegal; color++)
		r_face_copy(&c->faces[color], &_c->faces[color], c);
	c->current = r_color_first;
	c->sig_valid = 0;
	return c;
}

void r_cube_destroy(struct r_cube *c)
{
	if (!--c->ref)
		free(c);
}

enum r_color r_cube_color(struct r_cube *c)
{
	return c->current;
}

void r_cube_set_color(struct r_cube *c, enum r_color color)
{
	c->current = color;
}

void r_cube_next_color(struct r_cube *c)
{
	if (++c->current == r_color_illegal)
		c->current = r_color_first;
}

struct r_face *r_cube_face(struct r_cube *c, enum r_color color)
{
	return &c->faces[color];
}

void r_cube_turn(struct r_cube *c, enum r_color color, int dir)
{
	struct r_face *face = r_cube_face(c, color);
	if (!dir) return;
	c->sig_valid = 0;
	enum r_color *squares = r_face_get_squares(face);
	if (dir < 0) {
		/* anticlockwise */
		enum r_color b0 = squares[0];
		enum r_color b1 = squares[1];
		squares[0] = squares[2];
		squares[1] = squares[3];
		squares[2] = squares[4];
		squares[3] = squares[5];
		squares[4] = squares[6];
		squares[5] = squares[7];
		squares[6] = b0;
		squares[7] = b1;
	} else {
		/* clockwise */
		enum r_color b7 = squares[7];
		enum r_color b6 = squares[6];
		squares[7] = squares[5];
		squares[6] = squares[4];
		squares[5] = squares[3];
		squares[4] = squares[2];
		squares[3] = squares[1];
		squares[2] = squares[0];
		squares[1] = b7;
		squares[0] = b6;
	}
	r_face_put_squares(face); squares = NULL;
	enum r_color *cadj = r_clockwise[color];
	struct r_face *fadj[4];
	enum r_color *sadj[4];
	int i;
	for (i = 0; i < 4; i++) {
		fadj[i] = r_cube_face(c, cadj[i]);
		r_face_reorient(fadj[i], color);
		sadj[i] = r_face_get_squares(fadj[i]);
	}
	if (dir < 0) {
		/* anticlockwise */
		enum r_color b0 = sadj[0][0];
		enum r_color b1 = sadj[0][1];
		enum r_color b2 = sadj[0][2];
		sadj[0][0] = sadj[1][0]; sadj[0][1] = sadj[1][1]; sadj[0][2] = sadj[1][2];
		sadj[1][0] = sadj[2][0]; sadj[1][1] = sadj[2][1]; sadj[1][2] = sadj[2][2];
		sadj[2][0] = sadj[3][0]; sadj[2][1] = sadj[3][1]; sadj[2][2] = sadj[3][2];
		sadj[3][0] = b0; sadj[3][1] = b1; sadj[3][2] = b2;
	} else {
		/* clockwise */
		enum r_color b0 = sadj[3][0];
		enum r_color b1 = sadj[3][1];
		enum r_color b2 = sadj[3][2];
		sadj[3][0] = sadj[2][0]; sadj[3][1] = sadj[2][1]; sadj[3][2] = sadj[2][2];
		sadj[2][0] = sadj[1][0]; sadj[2][1] = sadj[1][1]; sadj[2][2] = sadj[1][2];
		sadj[1][0] = sadj[0][0]; sadj[1][1] = sadj[0][1]; sadj[1][2] = sadj[0][2];
		sadj[0][0] = b0; sadj[0][1] = b1; sadj[0][2] = b2;
	}
	for (i = 0; i < 4; i++)
		r_face_put_squares(fadj[i]);
}
const r_signature *r_cube_signature(struct r_cube *c)
{
	if (!c->sig_valid) {
		enum r_color color, *s = &c->signature[0];
		for (color = r_color_first; color < r_color_illegal; color++) {
			enum r_color **ref = &r_clockwise[color];
			struct r_face *face = r_cube_face(c, color);
			r_face_reorient(face, (*ref)[0]);
			memcpy(s, r_face_get_squares(face), 8 * sizeof(*s));
			r_face_put_squares(face);
			s += 8;
		}
		c->sig_valid = 1;
	}
	return &c->signature;
}

void r_precalc(void)
{
	cw_turn_precalc();
}
