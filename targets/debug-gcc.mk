CFLAGS := -pthread -ggdb3 -Wall
CFLAGS += -Werror -Wall -Wextra -Wshadow -Wstrict-prototypes -Wwrite-strings

AR := ar
ARFLAGS := rcs

# Slurp in the standard routines for parsing Makefile content and
# generating dependencies
include targets/common-default.mk
