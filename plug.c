#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <raylib.h>
#include <raymath.h>
#include "common.h"
#include "ffmpeg.h"

#define RENDER_WIDTH 900
#define RENDER_HEIGHT 450
#define RENDER_FPS 60
#define RENDER_DELTA_TIME (1.0/RENDER_FPS)

#define FONT_SIZE 52
#define CELL_WIDTH 100
#define CELL_HEIGHT 100
#define CELL_PAD (CELL_WIDTH*0.15)

typedef struct {
  size_t i;
  float duration;
} Animation;

typedef struct {
  float from;
  float to;
  float duration;
} Keyframe;

float keyframes_duration(Keyframe *kfs, size_t kfs_count)
{
  float duration = 0.0;
  for (size_t i = 0; i < kfs_count; ++i) {
    duration += kfs[i].duration;
  }
  return duration;
}

float animation_value(Animation *a, Keyframe *kfs, size_t kfs_count)
{
  assert(kfs_count > 0);
  Keyframe *kf = &kfs[a->i];
  float t = a->duration/kf->duration;
  t = (sinf(PI*t - PI*0.5) + 1)*0.5;
  return Lerp(kf->from, kf->to, t);
}

void animation_update(Animation *a, float dt, Keyframe *kfs, size_t kfs_count)
{
  assert(kfs_count > 0);

  a->i = a->i%kfs_count;
  a->duration += dt;

  while (a->duration >= kfs[a->i].duration) {
    a->duration -= kfs[a->i].duration;
    a->i = (a->i + 1)%kfs_count;
  }
}

typedef struct {
  size_t size;

  Animation a;
  FFMPEG *ffmpeg;
  RenderTexture2D screen;
  float rendering_duration;

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
  p->size = sizeof(*p);
  p->screen = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);
  load_resources();
}

void *plug_pre_reload(void)
{
  TraceLog(LOG_INFO, "Reloading plugin");
  unload_resources();
  return p;
}

void plug_post_reload(void *state)
{
  p = state;
  if (p->size < sizeof(*p)) {
    TraceLog(LOG_INFO, "Migrating plugin state scheme %zu bytes -> %zu bytes", p->size, sizeof(*p));
    p = realloc(p, sizeof(*p));
    p->size = sizeof(*p);
  }

  load_resources();
}

void turing_machine_tape(Animation *a, float dt, float w, float h)
{
  float offset = 7.0;
  Keyframe kfs[] = {
    {.from = w/2 - CELL_WIDTH/2 - (offset + 0)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 0)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    {.from = w/2 - CELL_WIDTH/2 - (offset + 0)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 1)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    {.from = w/2 - CELL_WIDTH/2 - (offset + 1)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 1)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    {.from = w/2 - CELL_WIDTH/2 - (offset + 1)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 2)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    {.from = w/2 - CELL_WIDTH/2 - (offset + 2)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 2)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    {.from = w/2 - CELL_WIDTH/2 - (offset + 2)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 3)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    //{.from = w/2 - CELL_WIDTH/2 - (offset + 3)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 3)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
    //{.from = w/2 - CELL_WIDTH/2 - (offset + 3)*(CELL_WIDTH + CELL_PAD), .to = w/2 - CELL_WIDTH/2 - (offset + 0)*(CELL_WIDTH + CELL_PAD), .duration = 0.5},
  };

  Vector2 cell_size = {CELL_WIDTH, CELL_HEIGHT};
#if DARK_MODE
  Color cell_color = COLOR_TEXT;
  Color head_color = COLOR_BLUE;
  Color background_color = COLOR_BASE;
#else
  Color cell_color = COLOR_BASE;
  Color head_color = COLOR_BLUE;
  Color background_color = COLOR_TEXT;
#endif

  animation_update(a, dt, kfs, ARRAY_LEN(kfs));
  float t = animation_value(a, kfs, ARRAY_LEN(kfs));

  ClearBackground(background_color);
  for (size_t i = 0; i < 200; ++i) {
    Rectangle rec = {
      .x = i*(CELL_WIDTH + CELL_PAD) + t,
      .y = h/2 - CELL_HEIGHT/2,
      .width = CELL_WIDTH,
      .height = CELL_HEIGHT,
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
    .width = CELL_WIDTH + head_thick*3,
    .height = CELL_HEIGHT + head_thick*3,
  };
  rec.x = w/2 - rec.width/2;
  rec.y = h/2 - rec.height/2;
  DrawRectangleLinesEx(rec, head_thick, head_color);
}

void plug_update(void)
{
  BeginDrawing();
  if (p->ffmpeg != NULL) {
    if (p->rendering_duration >= 3) {
      ffmpeg_end_rendering(p->ffmpeg);
      memset(&p->a, 0, sizeof(p->a));
      p->ffmpeg = NULL;
      SetTraceLogLevel(LOG_INFO);
    } else {
      BeginTextureMode(p->screen);
      turing_machine_tape(&p->a, RENDER_DELTA_TIME, RENDER_WIDTH, RENDER_HEIGHT);
      p->rendering_duration += RENDER_DELTA_TIME;
      EndTextureMode();

      Image image = LoadImageFromTexture(p->screen.texture);
      if (!ffmpeg_send_frame_flipped(p->ffmpeg, image.data, image.width, image.height)) {
        ffmpeg_end_rendering(p->ffmpeg);
        memset(&p->a, 0, sizeof(p->a));
        p->ffmpeg = NULL;
        SetTraceLogLevel(LOG_INFO);
      }
      UnloadImage(image);
    }
  } else {
    if (IsKeyPressed(KEY_R)) {
      SetTraceLogLevel(LOG_WARNING);
      p->ffmpeg = ffmpeg_start_rendering(RENDER_WIDTH, RENDER_HEIGHT, RENDER_FPS);
      p->rendering_duration = 0.0;
      memset(&p->a, 0, sizeof(p->a));
    }
    turing_machine_tape(&p->a, GetFrameTime(), GetScreenWidth(), GetScreenHeight());
  }
  EndDrawing();
}
