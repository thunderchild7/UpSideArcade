CC ?= cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src
APP_CFLAGS := -std=c99 -Wall -Istubs -I../src -Wno-unused-function -Wno-unused-variable -Wno-unused-const-variable

.PHONY: test clean

test: test_face_logic
	./test_face_logic
	$(CC) $(APP_CFLAGS) -c ../src/feature_draw_bitmap.c -o /tmp/upside-arcade-feature.o
	$(CC) $(APP_CFLAGS) -c ../src/face_logic.c -o /tmp/upside-arcade-face-logic.o
	rm -f /tmp/upside-arcade-feature.o /tmp/upside-arcade-face-logic.o

test_face_logic: test_face_logic.c ../src/face_logic.c ../src/face_logic.h
	$(CC) $(CFLAGS) -o $@ test_face_logic.c ../src/face_logic.c

clean:
	rm -f test_face_logic
