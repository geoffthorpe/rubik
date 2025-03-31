#include <rubik/rubik.h>

#include "build-kociemba/include/search.h"

/* We position our cube and make hard-coded associations between our colored
 * cube (blue, orange, ...) and the reference model used by the kociemba solver
 * (Up, Left, Back, ...).
 *
 *  Front=Blue
 *     Up=Red
 *   Left=Yellow
 *  Right=White
 *   Back=Green
 *   Down=orange
 */
char col2ref[6] = {
	'R', // white
	'L', // yellow
	'F', // blue
	'B', // green
	'U', // red
	'D'  // orange
};
enum r_color ref2col[256]; /* Map from ASCII to enum r_color */
void kociemba_precalc(void)
{
	ref2col['R'] = r_color_white;
	ref2col['L'] = r_color_yellow;
	ref2col['F'] = r_color_blue;
	ref2col['B'] = r_color_green;
	ref2col['U'] = r_color_red;
	ref2col['D'] = r_color_orange;
}
static void face2facelets(struct r_face *face, char *facelets)
{
	enum r_color *squares = r_face_get_squares(face);
	*(facelets++) = col2ref[squares[0]];
	*(facelets++) = col2ref[squares[1]];
	*(facelets++) = col2ref[squares[2]];
	*(facelets++) = col2ref[squares[7]];
	*(facelets++) = col2ref[r_face_color(face)];
	*(facelets++) = col2ref[squares[3]];
	*(facelets++) = col2ref[squares[6]];
	*(facelets++) = col2ref[squares[5]];
	*facelets = col2ref[squares[4]];
	r_face_put_squares(face);
}
static void find_improvement(struct r_line *line, struct r_cube *cube,
			     volatile int *terminate UNUSED)
{
	struct r_face *face;
	char facelets[55]; /* UU...RR...FF...DD...LL...BB... */
	/* Up = Red */
	face = r_cube_face(cube, r_color_red);
	r_face_reorient(face, r_color_green);
	face2facelets(face, facelets);
	/* Right = White */
	face = r_cube_face(cube, r_color_white);
	r_face_reorient(face, r_color_red);
	face2facelets(face, facelets + 9);
	/* Front = Blue */
	face = r_cube_face(cube, r_color_blue);
	r_face_reorient(face, r_color_red);
	face2facelets(face, facelets + 18);
	/* Down = Orange */
	face = r_cube_face(cube, r_color_orange);
	r_face_reorient(face, r_color_blue);
	face2facelets(face, facelets + 27);
	/* Left = Yellow */
	face = r_cube_face(cube, r_color_yellow);
	r_face_reorient(face, r_color_red);
	face2facelets(face, facelets + 36);
	/* Back = Green */
	face = r_cube_face(cube, r_color_green);
	r_face_reorient(face, r_color_red);
	face2facelets(face, facelets + 45);
	facelets[54]='\0';
	char *sol = solution(facelets, 24, 1000, 0, ".cache-kociemba");
	char *s = sol;
	line->bestused = 0;
	while (*s) {
		enum r_color color = ref2col[(unsigned char)*s];
		s++;
		int dir = 1; // default is clockwise
		if (*s == '\'')
			dir = -1;
		line->bestmove[line->bestused++] = color * 2 + (dir == 1 ? 1 : 0);
		if (*s == '2')
			line->bestmove[line->bestused++] =
						color * 2 + (dir == 1 ? 1 : 0);
		while (isspace(*(++s)))
			;
	}
	free(sol);
}

static void *solver_threadfn(void *_s)
{
	struct r_solver *s = _s;
	find_improvement(&s->line, s->cube, &s->terminate);
	assert(s->state == r_solver_running);
	s->state = r_solver_done;
	POSIXOK(pthread_cond_signal(&s->cond));
	return NULL;
}

int r_solver_start(struct r_solver *s, struct r_cube *cube)
{
	r_line_init(&s->line);
	s->cube = r_cube_clone(cube);
	POSIXOK(pthread_mutex_init(&s->mutex, NULL));
	POSIXOK(pthread_cond_init(&s->cond, NULL));
	s->terminate = 0;
	POSIXOK(pthread_create(&s->thread, NULL, solver_threadfn, s));
	s->state = r_solver_running;
	return 0;
}

int r_solver_terminate(struct r_solver *s)
{
	POSIXOK(pthread_mutex_lock(&s->mutex));
	s->terminate = 1;
	POSIXOK(pthread_mutex_unlock(&s->mutex));
	return 0;
}

int r_solver_join(struct r_solver *s)
{
	POSIXOK(pthread_mutex_lock(&s->mutex));
	while (s->state != r_solver_done)
		POSIXOK(pthread_cond_wait(&s->cond, &s->mutex));
	POSIXOK(pthread_mutex_unlock(&s->mutex));
	POSIXOK(pthread_join(s->thread, NULL));
	s->state = r_solver_idle;
	return 0;
}
