# simple makefile

CC=gcc
#CFLAGS+= --std=c99 -fPIC -Wno-format-security -static -no-pie -pedantic -Wall -Wextra -Werror -D_XOPEN_SOURCE=700 -g 
CFLAGS+= --std=c99 -fPIC -Wno-format-security -static -no-pie -pedantic -Wall -Wextra -Werror -D_XOPEN_SOURCE=700
LDFLAGS+=

SOURCES=sister.c connection-fork.c request-http.c log.c cmdline.c i4httools.c dirlisting.c md5.c
PROG=sister
TESTS=test-request
# Libraries you use. specify path or -lNAME
#LIBS=lib/cmdline.o lib/i4httools.o

#yes/no
ENABLE_PKGCONFIG=no
#packages you use: e.g. sdl curses ...
PACKAGES=

#################################################

OBJ=$(patsubst %.c,%.o,$(SOURCES))

ifneq (,$(filter-out %.o,$(OBJ)))
$(error "Unexpected files in SOURCES: $(filter-out %.o,$(OBJ))")
endif

ifeq (yes, $(ENABLE_PKGCONFIG))
CFLAGS+=$(shell pkg-config --cflags $(PACKAGES))
#LIBS+=$(shell pkg-config --libs $(PACKAGES))
endif

CCC=$(CC) $(CFLAGS) -c
LINKTO=$(CC) $(LDFLAGS) $(CFLAGS) -o


.PHONY: all clean

all: $(PROG)	

$(PROG):$(OBJ)
	$(LINKTO) $(PROG) $(OBJ)
	strip -s $(PROG)
clean:
	rm -f $(PROG) $(OBJ) || true


%.o: %.c
	$(CCC) $<

################## TESTS

test-request:test-request.o request-http.o log.o
	$(LINKTO) test-request $^ lib/i4httools.o lib/cmdline.o

