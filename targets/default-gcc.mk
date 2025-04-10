CFLAGS := -pthread -O2 -Wall
#CFLAGS += -Werror -Wall -Wextra -Wshadow -Wstrict-prototypes -Wwrite-strings
CFLAGS += -Wall -Wextra -Wshadow -Wstrict-prototypes -Wwrite-strings -Wno-implicit-fallthrough
CFLAGS += -DNDEBUG

AR := ar
ARFLAGS := rcs

# Slurp in the standard routines for parsing Makefile content and
# generating dependencies
include targets/common-default.mk
