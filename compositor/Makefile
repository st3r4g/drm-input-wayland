CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic-errors -Wno-unused
INCLUDE=-I/usr/include/libdrm
LIBS=-lm -ldrm -lgbm -lEGL -lGLESv2 -lwayland-server

main: algebra.o compositor.o output.o region.o surface.o xdg_shell.o \
protocols/xdg-shell-unstable-v6-code.o renderer.o main.o
	${CC} ${LIBS} -o $@ $^

algebra.o: algebra.c algebra.h
	${CC} ${CFLAGS} -c -o $@ $<

compositor.o: compositor.c compositor.h surface.h region.h
	${CC} ${CFLAGS} -c -o $@ $<

output.o: output.c output.h
	${CC} ${CFLAGS} -c -o $@ $<

region.o: region.c region.h
	${CC} ${CFLAGS} -c -o $@ $<

surface.o: surface.c surface.h renderer.h
	${CC} ${CFLAGS} -c -o $@ $<

xdg_shell.o: xdg_shell.c xdg_shell.h \
protocols/xdg-shell-unstable-v6-server-protocol.h
	${CC} ${CFLAGS} -c -o $@ $<

renderer.o: renderer.c
	${CC} ${CFLAGS} -c -o $@ $<

main.o: main.c algebra.h compositor.h output.h surface.h xdg_shell.h
	${CC} ${CFLAGS} ${INCLUDE} -c -o $@ $<

clean:
	rm -f *.o protocols/*.o main
