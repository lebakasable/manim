CFLAGS=-Wall -Wextra -pedantic
LIBS=-lraylib

OUT_LIBPLUG=build/libplug.so
OUT_BIN=build/main

build: $(OUT_BIN)

$(OUT_LIBPLUG): plug.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(LIBS)

$(OUT_BIN): main.c $(OUT_LIBPLUG)
	$(CC) $(CFLAGS) -DLIBPLUG_PATH=\"$(OUT_LIBPLUG)\" -o $@ $< $(LIBS)

run: $(OUT_BIN)
	./$<
