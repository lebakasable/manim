#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdbool.h>

#include <raylib.h>
#include "plug.h"

#ifndef LIBPLUG_PATH
#error "Constant LIBPLUG_PATH is undefined. Try adding -DLIBPLUG_PATH=<path> to your compilation command, or add a define directive in the top of the source file."
#endif

static void *libplug = NULL; 

bool reload_libplug(void)
{
  if (libplug != NULL) {
    dlclose(libplug);
  }

  libplug = dlopen(LIBPLUG_PATH, RTLD_NOW);
  if (libplug == NULL) {
    fprintf(stderr, "ERROR: %s\n", dlerror());
    return false;
  }

  plug_init = dlsym(libplug, "plug_init");
  if (plug_init == NULL) {
    fprintf(stderr, "ERROR: %s\n", dlerror());
    return false;
  }

  plug_pre_reload = dlsym(libplug, "plug_pre_reload");
  if (plug_pre_reload == NULL) {
    fprintf(stderr, "ERROR: %s\n", dlerror());
    return false;
  }

  plug_post_reload = dlsym(libplug, "plug_post_reload");
  if (plug_post_reload == NULL) {
    fprintf(stderr, "ERROR: %s\n", dlerror());
    return false;
  }

  plug_update = dlsym(libplug, "plug_update");
  if (plug_update == NULL) {
    fprintf(stderr, "ERROR: %s\n", dlerror());
    return false;
  }

  return true;
}

int main()
{
  if (!reload_libplug()) return 1;

  float factor = 50.0f;
  InitWindow(16*factor, 9*factor, "Manim");
  plug_init();

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      void *state = plug_pre_reload();
      reload_libplug();
      plug_post_reload(state);
    }

    plug_update();
  }

  CloseWindow();

  return 0;
}
