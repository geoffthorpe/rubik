#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/freeglut.h>

#include <rubik/rubik.h>
#include <private_cube.h>

#ifdef _MSC_VER
#pragma warning (disable: 4305 4244)
#endif

static const char *helpprompt[] = {"Press F1 for help", 0};
static const char *helptext[] = {
	"Rotate: left mouse drag",
	" Scale: right mouse drag up/down",
	"   Pan: middle mouse drag",
	"Thickness: t (thinner), T (fatter)",
	"Turn: [0123456789ab], 12 hard-coded turns",
	"Turn: z (anticlockwise), x (clockwise)",
	"Change side: s",
	"Random move: r",
	"Solve cube: l",
	"Toggle spinning: space",
	"Toggle glowing: g",
	"Toggle paille: p",
	"Toggle fullscreen: f",
	"Quit: escape",
	"",
	"Press F1 to hide help",
	0
};

void idle(void);
void display(void);
void print_help(void);
void reshape(int x, int y);
void keypress(unsigned char key, int x, int y);
void skeypress(int key, int x, int y);
void mouse(int bn, int st, int x, int y);
void motion(int x, int y);

int win_width, win_height;
float cam_theta, cam_phi = 25, cam_dist = 4;
float cam_pan[3];
int mouse_x, mouse_y;
int bnstate[8];
int anim, glowing, help, score, anim_last_time;
int turning, turning_last_time, turning_goal;
int cube_gap = 1;
int paille;
struct r_solver solver;
long anim_start, spin;
long nframes;
struct r_cube *cube;

#define TURNTIME_MS 600

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB	0x8db9
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809d
#endif

int main(int argc, char **argv)
{
	draw_cube_precalc();
	kociemba_precalc();
	r_precalc();
	cube = r_cube();
	glutInit(&argc, argv);
	glutInitWindowSize(1200, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("Geoff's Rubik's cube");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keypress);
	glutSpecialFunc(skeypress);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glutMainLoop();
	r_cube_destroy(cube);
	return 0;
}

void idle(void)
{
	glutPostRedisplay();
}

void keypress_next(void);

void display(void)
{
	int current_time = glutGet(GLUT_ELAPSED_TIME);
	float cwturn = 0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);
	glTranslatef(cam_pan[0], cam_pan[1], cam_pan[2]);

	glPushMatrix();
	if(anim) {
		spin += current_time - anim_last_time;
		anim_last_time = current_time;
	}
	glRotatef(spin / 27.0f, 1, 0, 0);
	glRotatef(spin / 21.0f, 0, 1, 0);
	glRotatef(spin / 17.0f, 0, 0, 1);
	if (turning) {
		if (current_time - turning_last_time > TURNTIME_MS) {
			turning = 0;
			r_cube_turn(cube, r_cube_color(cube), turning_goal);
		} else
			cwturn = turning_goal * 90.0 *
				(current_time - turning_last_time) / TURNTIME_MS;
	}
	if (!turning)
		keypress_next();
	r_cube_draw(cube, paille, glowing, cwturn);
	glPopMatrix();

	print_help();

	glutSwapBuffers();
	nframes++;
}

void print_help(void)
{
	int i;
	const char *s, **text;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_width, 0, win_height, -1, 1);

	text = help ? helptext : helpprompt;

	for(i=0; text[i]; i++) {
		glColor3f(0, 0.1, 0);
		glRasterPos2f(7, win_height - (i + 1) * 20 - 2);
		s = text[i];
		while(*s) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		}
		glColor3f(0, 0.9, 0);
		glRasterPos2f(5, win_height - (i + 1) * 20);
		s = text[i];
		while(*s) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		}
	}
	const char *msg = NULL;
	char msgbuf[60];
	if (solver.state == r_solver_running)
		msg = "Solving...";
	else if (solver.state == r_solver_done) {
		snprintf(msgbuf, 60, "Solving: ");
		for (i = 0; i < (int)solver.line.bestused; i++)
			snprintf(msgbuf + strlen(msgbuf), 60 - strlen(msgbuf),
				"%x", solver.line.bestmove[i]);
		msg = msgbuf;
	}
	if (msg) {
		glColor3f(0, 0.1, 0);
		glRasterPos2f(7, 35);
		s = msg;
		while(*s)
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		glColor3f(0, 0.9, 0);
		glRasterPos2f(5, 37);
		s = msg;
		while(*s)
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}

#define ZNEAR	0.5f
void reshape(int x, int y)
{
	float vsz, aspect = (float)x / (float)y;
	win_width = x;
	win_height = y;

	glViewport(0, 0, x, y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	vsz = 0.4663f * ZNEAR;
	glFrustum(-aspect * vsz, aspect * vsz, -vsz, vsz, 0.5, 500.0);
}

static void keypress_turn(int dir)
{
	assert(!turning);
	turning = 1;
	turning_goal = dir;
	turning_last_time = glutGet(GLUT_ELAPSED_TIME);
	glutIdleFunc(idle);
	glutPostRedisplay();
	return;
}

#define KEYBUFFER_DEPTH 8
struct keybuffer {
	unsigned char key[KEYBUFFER_DEPTH];
	unsigned int used;
};

static void reset_solver(void)
{
	if (solver.state != r_solver_idle) {
		POSIXOK(r_solver_terminate(&solver));
		POSIXOK(r_solver_join(&solver));
	}
	if (solver.state == r_solver_done) {
		solver.state = r_solver_idle;
		r_line_init(&solver.line);
		solver.terminate = 0;
	}
}

struct keybuffer keybuffer;

void keypress_next(void)
{
do_another:
	if (!keybuffer.used) {
		if ((solver.state == r_solver_done) && solver.line.bestused) {
			unsigned char move = solver.line.bestmove[0];
			if (--solver.line.bestused)
				memmove(&solver.line.bestmove[0],
					&solver.line.bestmove[1],
					solver.line.bestused *
						sizeof(solver.line.bestmove[0]));
			else
				reset_solver();
			r_cube_set_color(cube, (enum r_color)(move / 2));
			keypress_turn(move & 1 ? 1 : -1);
		}
		return;
	}
	unsigned char key = keybuffer.key[0];
	if (--keybuffer.used)
		memmove(&keybuffer.key[0], &keybuffer.key[1],
			keybuffer.used * sizeof(keybuffer.key[0]));

	static int fullscr;
	static int prev_xsz, prev_ysz;

	switch(key) {
	case 27:
	case 'q':
		reset_solver();
		glutLeaveMainLoop();
		break;

	case ' ':
		anim ^= 1;
		glutIdleFunc(anim || glowing || turning ||
			(solver.state == r_solver_running) ? idle : 0);
		glutPostRedisplay();

		if(anim) {
			anim_last_time = glutGet(GLUT_ELAPSED_TIME);
			anim_start = anim_last_time;
			nframes = 0;
		} else {
			long tm = glutGet(GLUT_ELAPSED_TIME) - anim_start;
			long fps = (nframes * 100000) / tm;
			printf("framerate: %ld.%ld fps\n", fps / 100, fps % 100);
		}
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'a':
	case 'b':
	{
		reset_solver();
		unsigned int move = (key >= '0' && key <= '9') ? key - '0' :
			key - 'a' + 10;
		r_cube_set_color(cube, (enum r_color)(move / 2));
		keypress_turn(move & 1 ? 1 : -1);
		return;
	}

	case 'r':
	{
		reset_solver();
		unsigned int move = rand() % 12;
		r_cube_set_color(cube, (enum r_color)(move / 2));
		keypress_turn(move & 1 ? 1 : -1);
		return;
	}

	case 'z':
		reset_solver();
		keypress_turn(-1);
		return;
	case 'x':
		reset_solver();
		keypress_turn(1);
		return;

	case 'g':
		glowing ^= 1;
		glutIdleFunc(anim || glowing || turning ||
			(solver.state == r_solver_running) ? idle : 0);
		glutPostRedisplay();
		break;

	case 'p':
		paille ^= 1;
		glutPostRedisplay();
		break;

	case 's':
		r_cube_next_color(cube);
		glutPostRedisplay();
		break;

	case 't':
		if (--cube_gap < 0)
			cube_gap = 0;
		cube_set_gap(cube_gap);
		glutPostRedisplay();
		break;

	case 'T':
		++cube_gap;
		cube_set_gap(cube_gap);
		glutPostRedisplay();
		break;

	case 'l':
		switch (solver.state) {
		case r_solver_idle:
			POSIXOK(r_solver_start(&solver, cube));
			break;
		case r_solver_running:
			reset_solver();
			break;
		case r_solver_done:
			POSIXOK(r_solver_join(&solver));
			POSIXOK(r_solver_start(&solver, cube));
			break;
		}
		glutIdleFunc(anim || glowing || turning ||
			(solver.state == r_solver_running) ? idle : 0);
		break;

	case 'f':
		fullscr ^= 1;
		if(fullscr) {
			prev_xsz = glutGet(GLUT_WINDOW_WIDTH);
			prev_ysz = glutGet(GLUT_WINDOW_HEIGHT);
			glutFullScreen();
		} else {
			glutReshapeWindow(prev_xsz, prev_ysz);
		}
		break;
	}
	goto do_another;
}

void keypress(unsigned char key, int x UNUSED, int y UNUSED)
{
	if (keybuffer.used == KEYBUFFER_DEPTH)
		return;
	keybuffer.key[keybuffer.used++] = key;
	if (!turning)
		keypress_next();
}

void skeypress(int key, int x UNUSED, int y UNUSED)
{
	switch(key) {
	case GLUT_KEY_F1:
		help ^= 1;
		glutPostRedisplay();
		break;
	case GLUT_KEY_F2:
		score ^= 1;
		glutPostRedisplay();
		break;
	default:
		break;
	}
}

void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	bnstate[bidx] = st == GLUT_DOWN;
	mouse_x = x;
	mouse_y = y;
}

void motion(int x, int y)
{
	int dx = x - mouse_x;
	int dy = y - mouse_y;
	mouse_x = x;
	mouse_y = y;

	if(!(dx | dy)) return;

	if(bnstate[0]) {
		cam_theta += dx * 0.5;
		cam_phi += dy * 0.5;
		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
		glutPostRedisplay();
	}
	if(bnstate[1]) {
		float up[3], right[3];
		float theta = cam_theta * M_PI / 180.0f;
		float phi = cam_phi * M_PI / 180.0f;

		up[0] = -sin(theta) * sin(phi);
		up[1] = -cos(phi);
		up[2] = cos(theta) * sin(phi);
		right[0] = cos(theta);
		right[1] = 0;
		right[2] = sin(theta);

		cam_pan[0] += (right[0] * dx + up[0] * dy) * 0.01;
		cam_pan[1] += up[1] * dy * 0.01;
		cam_pan[2] += (right[2] * dx + up[2] * dy) * 0.01;
		glutPostRedisplay();
	}
	if(bnstate[2]) {
		cam_dist += dy * 0.1;
		if(cam_dist < 0) cam_dist = 0;
		glutPostRedisplay();
	}
}
