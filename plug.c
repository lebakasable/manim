#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <raylib.h>
#include <raymath.h>
#include "common.h"

#define FONT_SIZE 52

#define COLOR_BASE   GetColor(0x1e1e2eff)
#define COLOR_RED    GetColor(0xf38ba8ff)
#define COLOR_YELLOW GetColor(0xf9e2afff)
#define COLOR_BLUE   GetColor(0x89b4faff)
#define COLOR_TEXT   GetColor(0xcdd6f4ff)

typedef struct {
  size_t i;
  float duration;
  bool loop;
} Animation;

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

typedef struct {
  Animation a;
  Font font;
} Plug;

static Plug *p = NULL;

void load_resources()
{
  p->font = LoadFontEx("fonts/ttf/JetBrainsMono-Regular.ttf", FONT_SIZE, 0, 250);
}

void unload_resources()
{
  UnloadFont(p->font);
}

void plug_init(void)
{
  TraceLog(LOG_INFO, "Initializing plugin");
  p = malloc(sizeof(*p));
  assert(p != NULL);
  memset(p, 0, sizeof(*p));
  p->a.loop = true;
  load_resources();
}

void *plug_pre_reload(void)
{
  unload_resources();
  return p;
}

void plug_post_reload(void *state)
{
  p = state;
  load_resources();
}

void plug_update(void)
{
  float rw = 100.0;
  float rh = 100.0;
  Vector2 cell_size = {rw, rh};
  float pad = rw*0.15;
  float w = GetScreenWidth();
  float h = GetScreenHeight();

  size_t offset = 7;
  Keyframe kfs[] = {
    {.from = w/2 - rw/2 - (offset + 0)*(rw + pad), .to = w/2 - rw/2 - (offset + 0)*(rw + pad), .duration = 0.5},
    {.from = w/2 - rw/2 - (offset + 0)*(rw + pad), .to = w/2 - rw/2 - (offset + 1)*(rw + pad), .duration = 0.5},
    {.from = w/2 - rw/2 - (offset + 1)*(rw + pad), .to = w/2 - rw/2 - (offset + 1)*(rw + pad), .duration = 0.5},
    {.from = w/2 - rw/2 - (offset + 1)*(rw + pad), .to = w/2 - rw/2 - (offset + 2)*(rw + pad), .duration = 0.5},
    {.from = w/2 - rw/2 - (offset + 2)*(rw + pad), .to = w/2 - rw/2 - (offset + 2)*(rw + pad), .duration = 0.5},
    {.from = w/2 - rw/2 - (offset + 2)*(rw + pad), .to = w/2 - rw/2 - (offset + 3)*(rw + pad), .duration = 0.5},
    //{.from = w/2 - rw/2 - (offset + 3)*(rw + pad), .to = w/2 - rw/2 - (offset + 3)*(rw + pad), .duration = 0.5},
    //{.from = w/2 - rw/2 - (offset + 3)*(rw + pad), .to = w/2 - rw/2 - (offset + 0)*(rw + pad), .duration = 0.5},
  };

#if DARK_MODE
  Color cell_color = COLOR_TEXT;
  Color head_color = COLOR_BLUE;
  Color background_color = COLOR_BASE;
#else
  Color cell_color = COLOR_BASE;
  Color head_color = COLOR_BLUE;
  Color background_color = COLOR_TEXT;
#endif

  BeginDrawing();
  animation_update(&p->a, kfs, ARRAY_LEN(kfs));
  float t = animation_value(p->a, kfs, ARRAY_LEN(kfs));
  ClearBackground(background_color);
  for (size_t i = 0; i < 200; ++i) {
    Rectangle rec = {
      .x = i*(rw + pad) + t,
      .y = h/2 - rh/2,
      .width = rw,
      .height = rh,
    };
    DrawRectangleRec(rec, cell_color);

    const char *text = "0";
    Vector2 text_size = MeasureTextEx(p->font, text, FONT_SIZE, 0);
    Vector2 position = { .x = rec.x, .y = rec.y };
    position = Vector2Add(position, Vector2Scale(cell_size, 0.5));
    position = Vector2Subtract(position, Vector2Scale(text_size, 0.5));
    DrawTextEx(p->font, text, position, FONT_SIZE, 0, background_color);
  }

  float head_thick = 20.0;
  Rectangle rec = {
    .width = rw + head_thick*3,
    .height = rh + head_thick*3,
  };
  rec.x = w/2 - rec.width/2;
  rec.y = h/2 - rec.height/2;
  DrawRectangleLinesEx(rec, head_thick, head_color);

  EndDrawing();
}
