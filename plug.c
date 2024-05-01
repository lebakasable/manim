#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <raylib.h>
#include <raymath.h>
#include "common.h"

#define COLOR_BASE   GetColor(0x1e1e2eff)
#define COLOR_RED    GetColor(0xf38ba8ff)
#define COLOR_YELLOW GetColor(0xf9e2afff)

typedef struct {
  size_t i;
  float duration;
  bool loop;
} Animation;

typedef struct {
  Color background;
  Animation a;
} Plug;

static Plug *p = NULL;

void plug_init(void)
{
  TraceLog(LOG_INFO, "Initializing plugin");
  p = malloc(sizeof(*p));
  assert(p != NULL);
  memset(p, 0, sizeof(*p));
  p->background = COLOR_BASE;
  p->a.loop = true;
}

void *plug_pre_reload(void)
{
  return p;
}

void plug_post_reload(void *state)
{
  p = state;
}

typedef struct {
  float from;
  float to;
  float duration;
} Keyframe;

float animation_value(Animation a, Keyframe *kfs, size_t kfs_count)
{
  assert(kfs_count > 0);
  if (a.i >= kfs_count) {
    return kfs[kfs_count - 1].to;
  }

  Keyframe *kf = &kfs[a.i];

  return Lerp(kf->from, kf->to, a.duration/kf->duration);
}

void animation_update(Animation *a, Keyframe *kfs, size_t kfs_count)
{
  assert(kfs_count > 0);

  if (a->i >= kfs_count) {
    if (a->loop) {
      a->i = 0;
      a->duration = 0;
    } else {
      return;
    }
  }

  float dt = GetFrameTime();
  a->duration += dt;

  while (a->i < kfs_count && a->duration >= kfs[a->i].duration) {
    a->duration -= kfs[a->i].duration;
    a->i += 1;
  }
}

void plug_update(void)
{
  Keyframe kfs[] = {
    {
      .from = 0.0,
      .to = 0.250,
      .duration = 0.250,
    },
    {
      .from = 0.25,
      .to = 0.25,
      .duration = 0.5,
    },
    {
      .from = 0.25,
      .to = 1.0,
      .duration = 0.5,
    },
  };

  float w = GetScreenWidth();
  float h = GetScreenHeight();

  BeginDrawing();
  animation_update(&p->a, kfs, ARRAY_LEN(kfs));
  float t = animation_value(p->a, kfs, ARRAY_LEN(kfs));
  ClearBackground(a->background);
  float rw = 100.0;
  float rh = 100.0;
  float pad = rw*0.15;
  Color cell_color = COLOR_RED;
  for (size_t i = 0; i < 10; ++i) {
    DrawRectangle(i*(rw + pad) - w*t, h/2 - rh/2, rw, rh, cell_color);
  }

  float head_thick = 20.0;
  Rectangle rec = {
    .width = rw + head_thick*3,
    .height = rh + head_thick*3,
  };
  rec.x = w/2 - rec.width/2;
  rec.y = h/2 - rec.height/2;
  DrawRectangleLinesEx(rec, head_thick, COLOR_YELLOW);

  EndDrawing();
}
