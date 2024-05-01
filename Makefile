CFLAGS=-Wall -Wextra
LIBS=-lraylib

OUT_LIBPLUG=build/libplug.so
OUT_BIN=build/main

DARK_MODE=1

build: $(OUT_BIN)

$(OUT_LIBPLUG): plug.c
	$(CC) $(CFLAGS) -fPIC -shared -DDARK_MODE=$(DARK_MODE) -o $@ $< $(LIBS)

$(OUT_BIN): main.c $(OUT_LIBPLUG)
	$(CC) $(CFLAGS) -DLIBPLUG_PATH=\"$(OUT_LIBPLUG)\" -o $@ $< $(LIBS)

run: $(OUT_BIN)
	./$<
