#ifndef HEADER_RUBIK_H
#define HEADER_RUBIK_H

#include <rubik/common.h>

enum r_color {
	r_color_first,
	r_color_white = r_color_first,
	r_color_yellow,
	r_color_blue,
	r_color_green,
	r_color_red,
	r_color_orange,
	r_color_illegal
};
struct r_face;
struct r_cube;

/* General entry point for all precalculation */
void r_precalc(void);

/* Low-level structures that hardcode the orientation of the cube's faces
 * relative to each other.
 * - r_clockwise[color] gives a list of 4 colors, representing the 4
 *   adjacent faces to the one with 'color' at its center, in clockwise
 *   order.
 */
extern enum r_color *r_clockwise[6];

/***********/
/* r_color */
/***********/

static inline const char *r_color_string(enum r_color c)
{
	return c == r_color_white ? "white" :
		c == r_color_yellow ? "yellow" :
		c == r_color_blue ? "blue" :
		c == r_color_green ? "green" :
		c == r_color_red ? "red" :
		c == r_color_orange ? "orange" :
		"<unknown>";
}
static inline char r_color_char(enum r_color c)
{
	return c == r_color_white ? 'w' :
		c == r_color_yellow ? 'y' :
		c == r_color_blue ? 'b' :
		c == r_color_green ? 'g' :
		c == r_color_red ? 'r' :
		c == r_color_orange ? 'o' :
		'-';
}
static inline enum r_color r_color_invert(enum r_color c)
{
	unsigned int v = (unsigned int)c;
	v = (v / 2) * 2 + !(v &  1);
	return (enum r_color)v;
}

/***************/
/* r_signature */
/***************/

typedef enum r_color r_signature[48];

static inline void r_signature_copy(r_signature *dst, const r_signature *src)
{
	unsigned int i;
	for (i = 0; i < 48; i++)
		(*dst)[i] = (*src)[i];
}

static inline int r_signature_cmp(const r_signature *a, const r_signature *b)
{
	unsigned int i;
	for (i = 0; i < 48; i++) {
		if ((*a)[i] < (*b)[i])
			return -1;
		if ((*a)[i] > (*b)[i])
			return 1;
	}
	return 0;
}

/**********/
/* r_face */
/**********/

struct r_cube *r_face_cube(struct r_face *f);
enum r_color r_face_color(struct r_face *f);
enum r_color r_face_current(struct r_face *f);
unsigned int r_face_iforient(struct r_face *f, enum r_color top,
			     unsigned int idx);
enum r_color *r_face_get_squares(struct r_face *f, enum r_color top);
void r_face_put_squares(struct r_face *f);
int r_face_adjacent(struct r_face *f, enum r_color c, unsigned int idx);

/**********/
/* r_cube */
/**********/

void r_cube_precalc(void);
struct r_cube *r_cube(void);
struct r_cube *r_cube_clone(const struct r_cube *c);
void r_cube_destroy(struct r_cube *c);
enum r_color r_cube_color(struct r_cube *c);
void r_cube_set_color(struct r_cube *c, enum r_color color);
void r_cube_next_color(struct r_cube *c);
struct r_face *r_cube_face(struct r_cube *c, enum r_color color);
void r_cube_turn(struct r_cube *c, enum r_color color, int dir);
const r_signature *r_cube_signature(struct r_cube *c);
void r_cube_draw(struct r_cube *c, int paille, int glowing, float cwturn);

/**********/
/* r_line */
/**********/

#define R_LINE_MAX 32
struct r_line {
	unsigned char move[R_LINE_MAX];
	unsigned int used;
	unsigned int bestscore;
	unsigned char bestmove[R_LINE_MAX];
	unsigned int bestused;
};

static inline void r_line_init(struct r_line *line)
{
	line->used = line->bestscore = line->bestused = 0;
}

/************/
/* r_solver */
/************/

struct r_solver {
	int terminate;
	enum r_solver_state {
		r_solver_idle,
		r_solver_running,
		r_solver_done,
	} state;
	pthread_t thread;
	struct r_line line;
	struct r_cube *cube;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

/* Use POSIXOK() on the return values */
int r_solver_start(struct r_solver *s, struct r_cube *cube);
int r_solver_terminate(struct r_solver *s);
int r_solver_join(struct r_solver *s);

#endif /* !HEADER_RUBIK_H */
