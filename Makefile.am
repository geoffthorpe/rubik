SUBDIRS = build-kociemba

lib_LIBRARIES = rubik
bin_BINARIES = cube sphere

rubik_SOURCES = rubik.c draw_cube.c solve_cube.c

cube_CFLAGS = $(rubik_CFLAGS)
cube_SOURCES = gui_cube.c
cube_LDADD = rubik kociemba
cube_LINKFLAGS = -lglut -lGL -lm

sphere_SOURCES = gui_sphere.c draw_sphere.c
sphere_LINKFLAGS = -lglut -lGL -lm
