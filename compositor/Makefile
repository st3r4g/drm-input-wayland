CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic-errors -Wno-unused
INCLUDE=-I/usr/include/libdrm
LIBS=-lm -ldrm -lgbm -lEGL -lGLESv2 -lwayland-server -lxkbcommon

main: algebra.o egl.o renderer.o compositor.o keyboard.o output.o region.o \
seat.o surface.o xdg_shell.o protocols/xdg-shell-unstable-v6-code.o main.o
	${CC} ${LIBS} -o $@ $^

algebra.o: algebra.c algebra.h
	${CC} ${CFLAGS} -c -o $@ $<

egl.o: egl.c egl.h 
	${CC} ${CFLAGS} -c -o $@ $<

renderer.o: renderer.c renderer.h algebra.h egl.h
	${CC} ${CFLAGS} -c -o $@ $<

# --- WAYLAND INTERFACES ---

compositor.o: compositor.c compositor.h surface.h region.h
	${CC} ${CFLAGS} -c -o $@ $<

keyboard.o: keyboard.c
	${CC} ${CFLAGS} -c -o $@ $<

output.o: output.c output.h
	${CC} ${CFLAGS} -c -o $@ $<

region.o: region.c region.h
	${CC} ${CFLAGS} -c -o $@ $<

seat.o: seat.c seat.h keyboard.h
	${CC} ${CFLAGS} -c -o $@ $<

surface.o: surface.c surface.h egl.h renderer.h
	${CC} ${CFLAGS} -c -o $@ $<

xdg_shell.o: xdg_shell.c xdg_shell.h \
protocols/xdg-shell-unstable-v6-server-protocol.h
	${CC} ${CFLAGS} -c -o $@ $<

# --- MAIN ---

main.o: main.c algebra.h egl.h renderer.h compositor.h output.h seat.h \
surface.h xdg_shell.h
	${CC} ${CFLAGS} ${INCLUDE} -c -o $@ $<

clean:
	rm -f *.o protocols/*.o main
