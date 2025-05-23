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
	"Speed: k (slower), l (faster)",
	"Change side: s",
	"Random move: r",
	"Thousand random moves: R",
	"Solve cube: <enter>",
	"Toggle spinning: space",
	"Toggle glowing: g",
	"Toggle paille: p",
	"Toggle fullscreen: f",
	"Quit: <escape> or q",
	"",
	"Press F1 to hide help",
	0
};

void idle(void);
void display(void);
void print_help(void);
static void print_demo(void);
void reshape(int x, int y);
static void keypress_demo_mode(void);
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
static float cube_gap = 1;
int paille;
struct r_solver solver;
long anim_start;
float spin, spinspeed = 1.0;
long nframes;
struct r_cube *cube;
int message_set = 0;
const char *message;
int message_time;
char message_buffer[64];
static int demo_mode;
static int fullscr;
static int prev_xsz, prev_ysz;
static enum demo_state {
	demo_idle1,
	demo_randomize,
	demo_idle2,
	demo_solve
} demo_state = demo_idle1;
static int demo_state_gate = 0;
static int demo_state_lapse = 5000;
static int demo_spinspeed_gate = 0;
static int demo_spinspeed_lapse = 1000;
static float demo_spinspeed1 = 0.2, demo_spinspeed2 = 0.4;
static int demo_thin_gate = 0;
static int demo_thin_lapse = 1000;
static float demo_thin1 = 1, demo_thin2 = 2;


#define MESSAGETIME_MS 3000
#define TURNTIME_MS ((float)600 / spinspeed)

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB	0x8db9
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809d
#endif

#define ARG() \
	do { \
		argv++; \
		argc--; \
	} while (0)

int main(int argc, char **argv)
{
	ARG();
	if (argc && !strcmp(*argv, "--demo")) {
		printf("FOO: engaging demo mode\n");
		demo_mode = 1;
	}
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
	if (demo_mode)
		keypress_demo_mode();
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
		spin += (current_time - anim_last_time) * spinspeed;
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
	if (demo_mode)
		print_demo();

	glutSwapBuffers();
	nframes++;
}

void print_help(void)
{
	int i;
	const char *s, **text;
	float f = 1.0;

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
	} else if (message_set) {
		int current_time = glutGet(GLUT_ELAPSED_TIME);
		if (current_time - message_time >= MESSAGETIME_MS) {
			message_set = 0;
			glutIdleFunc(anim || glowing || turning || message_set || demo_mode ||
				(solver.state == r_solver_running) ? idle : 0);
		} else {
			f = 1.0 - (float)(current_time - message_time) / MESSAGETIME_MS;
			msg = message;
		}
	}
	if (msg) {
		glColor3f(0, 0.1 * f, 0);
		glRasterPos2f(7, 35);
		s = msg;
		while(*s)
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		glColor3f(0, 0.9 * f, 0);
		glRasterPos2f(5, 37);
		s = msg;
		while(*s)
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}

void print_demo(void)
{
	const char *s, *text;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_width, 0, win_height, -1, 1);

	text = demo_state == demo_idle1 ? "Waiting to randomize" :
		demo_state == demo_randomize ? "Randomizing" :
		demo_state == demo_idle2 ? "Waiting to solve" :
		demo_state == demo_solve ? "Solving" :
		"<unknown>";

	glColor3f(0, 0.1, 0);
	glRasterPos2f(win_width - (strlen(text) + 1) * 10,
		      win_height - 20 - 2);
	s = text;
	while(*s) {
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
	}
	glColor3f(0, 0.9, 0);
	glRasterPos2f(win_width - (strlen(text) + 1) * 10 + 2,
		      win_height - 20);
	s = text;
	while(*s) {
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

static void set_message(const char *m)
{
	message = m;
	message_set = 1;
	message_time = glutGet(GLUT_ELAPSED_TIME);
	glutIdleFunc(anim || glowing || turning || message_set || demo_mode ||
		(solver.state == r_solver_running) ? idle : 0);
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

	switch(key) {
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
		int dir = move & 1 ? 1 : -1;
		snprintf(message_buffer, sizeof(message_buffer),
			 "Turning %s %s",
			 r_color_string((enum r_color)(move / 2)),
			 dir == -1 ? "anticlockwise" : "clockwise");
		set_message(message_buffer);
		r_cube_set_color(cube, (enum r_color)(move / 2));
		keypress_turn(dir);
		return;
	}

	case 'r':
	{
		reset_solver();
		unsigned int move = rand() % 12;
		r_cube_set_color(cube, (enum r_color)(move / 2));
		keypress_turn(move & 1 ? 1 : -1);
		set_message("Random move");
		return;
	}

	case 'R':
	{
		reset_solver();
		unsigned int repeat = 1000;
		while (repeat--) {
			unsigned int move = rand() % 12;
			r_cube_turn(cube, (enum r_color)(move / 2),
				    move & 1 ? 1 : -1);
		}
		set_message("Thousand random moves");
		glutPostRedisplay();
		return;
	}

	case 'z':
		reset_solver();
		keypress_turn(-1);
		set_message("Turn anticlockwise");
		return;
	case 'x':
		reset_solver();
		keypress_turn(1);
		set_message("Turn clockwise");
		return;

	case 's':
		r_cube_next_color(cube);
		snprintf(message_buffer, sizeof(message_buffer),
			"Change side to %s",
			r_color_string(r_cube_color(cube)));
		set_message(message_buffer);
		glutPostRedisplay();
		break;

	case 13:
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
		set_message("Solving");
		glutIdleFunc(anim || glowing || turning || message_set || demo_mode ||
			(solver.state == r_solver_running) ? idle : 0);
		break;

	default:
		printf("FOO: unknown keypress %d(%c)\n",
			key, (char)key);
	}
	goto do_another;
}

int keypress_always(unsigned char key)
{
	switch(key) {
	case 'q':
	case 27:
		reset_solver();
		glutLeaveMainLoop();
		return 1;

	case ' ':
		anim ^= 1;
		set_message(anim ? "Starting animation" : "Stopping animation");
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
		return 1;

	case 'g':
		glowing ^= 1;
		glutIdleFunc(anim || glowing || turning || message_set || demo_mode ||
			(solver.state == r_solver_running) ? idle : 0);
		snprintf(message_buffer, sizeof(message_buffer),
			"%s glowing", glowing ? "Enable" : "Disable");
		set_message(message_buffer);
		glutPostRedisplay();
		return 1;

	case 'p':
		paille ^= 1;
		snprintf(message_buffer, sizeof(message_buffer),
			"%s paille", glowing ? "Enable" : "Disable");
		set_message(message_buffer);
		glutPostRedisplay();
		return 1;

	case 't':
		if (--cube_gap < 0)
			cube_gap = 0;
		cube_set_gap(cube_gap);
		set_message("Thinner squares");
		glutPostRedisplay();
		return 1;

	case 'T':
		++cube_gap;
		cube_set_gap(cube_gap);
		set_message("Fatter squares");
		glutPostRedisplay();
		return 1;

	case 'f':
		fullscr ^= 1;
		if(fullscr) {
			prev_xsz = glutGet(GLUT_WINDOW_WIDTH);
			prev_ysz = glutGet(GLUT_WINDOW_HEIGHT);
			set_message("Enable full screen");
			glutFullScreen();
		} else {
			set_message("Disable full screen");
			glutReshapeWindow(prev_xsz, prev_ysz);
		}
		return 1;

	case 'k':
		spinspeed -= 0.2;
		if (spinspeed < 0.2)
			spinspeed = 0.2;
		else
			set_message("Slower");
		return 1;

	case 'l':
		spinspeed += 0.2;
		if (spinspeed > 2.0)
			spinspeed = 2.0;
		else
			set_message("Faster");
		return 1;
	}
	return 0;
}

static void keypress_demo_mode(void)
{
	int now = glutGet(GLUT_ELAPSED_TIME);
	/* Spin speed */
	if (demo_spinspeed_gate + demo_spinspeed_lapse < now) {
		demo_spinspeed_gate = now;
		demo_spinspeed1 = spinspeed;
		demo_spinspeed2 = rand() * 1.8 / RAND_MAX + 0.2;
		demo_spinspeed_lapse = (int)(1000 + (rand() * 9000.0) / RAND_MAX);
	}
	spinspeed = demo_spinspeed1 +
		(demo_spinspeed2 - demo_spinspeed1) *
			(now - demo_spinspeed_gate) / demo_spinspeed_lapse;
	/* Thinness */
	if (demo_thin_gate + demo_thin_lapse < now) {
		demo_thin_gate = now;
		demo_thin1 = cube_gap;
		demo_thin2 = rand() * 5.0 / RAND_MAX;
		demo_thin_lapse = (int)(1000 + (rand() * 9000.0) / RAND_MAX);
		printf("FOO: thin target %0.2f, ms=%d\n",
			demo_thin2, demo_thin_lapse);
	}
	cube_gap = demo_thin1 +
		(demo_thin2 - demo_thin1) *
			(now - demo_thin_gate) / demo_thin_lapse;
	cube_set_gap(cube_gap);
	/* State */
	switch (demo_state) {
	case demo_idle1:
		if (demo_state_gate + demo_state_lapse < now) {
			demo_state_gate = now;
			demo_state = demo_randomize;
			demo_state_lapse = (int)(10000 + (rand() * 20000.0) / RAND_MAX);
			printf("FOO: randomize, ms=%d\n", demo_state_lapse);
		}
		if (demo_state != demo_randomize)
			break;
	case demo_randomize:
		if (demo_state_gate + demo_state_lapse < now) {
			demo_state_gate = now;
			demo_state = demo_idle2;
			demo_state_lapse = (int)(3000 + (rand() * 10000.0) / RAND_MAX);
			printf("FOO: idle2, ms=%d\n", demo_state_lapse);
		} else if (!keybuffer.used)
			keybuffer.key[keybuffer.used++] = 'r';
		if (demo_state != demo_idle2)
			break;
	case demo_idle2:
		if (demo_state_gate + demo_state_lapse < now) {
			demo_state_gate = now;
			demo_state = demo_solve;
			printf("FOO: solve\n");
		} else if (!keybuffer.used && solver.state != r_solver_running)
			keybuffer.key[keybuffer.used++] = 13;
		if (demo_state != demo_solve)
			break;
	case demo_solve:
		if (solver.state == r_solver_idle) {
			demo_state_gate = now;
			demo_state = demo_idle1;
			demo_state_lapse = (int)(10000 + (rand() * 20000.0) / RAND_MAX);
			printf("FOO: idle1, ms=%d\n", demo_state_lapse);
		}
	}
}

void keypress(unsigned char key, int x UNUSED, int y UNUSED)
{
	if (keypress_always(key))
		return;
	if (demo_mode) {
		set_message("Can't interrupt demo mode\n");
		return;
	}
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
