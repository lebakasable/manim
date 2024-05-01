CFLAGS=-Wall -Wextra
LIBS=-lraylib

LIBPLUG_DEPS=ffmpeg.c
LIBPLUG_OUT=build/libplug.so
BIN_OUT=build/main

DARK_MODE=1

build: $(BIN_OUT)

$(LIBPLUG_OUT): plug.c $(LIBPLUG_DEPS)
	$(CC) $(CFLAGS) -fPIC -shared -DDARK_MODE=$(DARK_MODE) -o $@ $(LIBPLUG_DEPS) $< $(LIBS)

$(BIN_OUT): main.c $(LIBPLUG_OUT)
	$(CC) $(CFLAGS) -DLIBPLUG_PATH=\"$(LIBPLUG_OUT)\" -o $@ $< $(LIBS)

run: $(BIN_OUT)
	./$<
