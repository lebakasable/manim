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

#define TAPE_COUNT 20
#define HEAD_ANIMATION_DURATION 0.5

static inline float sinstep(float t)
{
  return (sinf(PI*t - PI*0.5) + 1)*0.5;
}

typedef struct {
  const char *from;
  const char *to;
  float t;
} Cell;

typedef enum {
  HS_MOVING,
  HS_WRITING,
  HS_HALT,
} HeadState;

typedef struct {
  HeadState state;
  size_t from, to;
  float t;
} Head;

typedef struct {
  size_t size;

  FFMPEG *ffmpeg;
  RenderTexture2D screen;
  float rendering_duration;

  Cell tape[TAPE_COUNT];
  Head head;

  Font font;
} Plug;

static Plug *p = NULL;

static void load_resources(void)
{
  p->font = LoadFontEx("fonts/ttf/JetBrainsMono-Regular.ttf", FONT_SIZE, 0, 250);
}

static void unload_resources(void)
{
  UnloadFont(p->font);
}

static void reset_animation(void)
{
  for (size_t i = 0; i < TAPE_COUNT; ++i) {
    p->tape[i].from = "69";
    p->tape[i].to = "420";
  }
  p->head.state = HS_MOVING;
  p->head.from = 7;
  p->head.to = 8;
  p->head.t = 0;
}

void plug_init(void)
{
  TraceLog(LOG_INFO, "Initializing plugin");
  p = malloc(sizeof(*p));
  assert(p != NULL);
  memset(p, 0, sizeof(*p));
  p->size = sizeof(*p);
  p->screen = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);
  reset_animation();
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

static void turing_machine(float dt, float w, float h)
{
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

  ClearBackground(background_color);

  float t = 0.0;
  switch (p->head.state) {
    case HS_MOVING: {
      p->head.t = (p->head.t*HEAD_ANIMATION_DURATION + dt)/HEAD_ANIMATION_DURATION;
      if (p->head.t >= 1.0) {
        p->head.from = p->head.to;
        p->head.state = HS_WRITING;

        p->tape[p->head.from].t = 0;
      }
      t = (float)p->head.from + ((float)p->head.to - (float)p->head.from)*sinstep(p->head.t);
    } break;
    case HS_WRITING: {
      p->tape[p->head.from].t = (p->tape[p->head.from].t*HEAD_ANIMATION_DURATION + dt)/HEAD_ANIMATION_DURATION;
      if (p->tape[p->head.from].t >= 1.0) {
        if (p->head.from + 1 >= TAPE_COUNT) {
          p->head.state = HS_HALT;
        } else {
          p->head.to = p->head.from + 1;
          p->head.t = 0.0;
          p->head.state = HS_MOVING;
        }
      }

      t = (float)p->head.from;
    } break;
    case HS_HALT: {
      t = (float)p->head.from;
    } break;
  }

  for (size_t i = 0; i < TAPE_COUNT; ++i) {
    Rectangle rec = {
      .x = i*(CELL_WIDTH + CELL_PAD) + w/2 - CELL_WIDTH/2 - t*(CELL_WIDTH + CELL_PAD),
      .y = h/2 - CELL_HEIGHT/2,
      .width = CELL_WIDTH,
      .height = CELL_HEIGHT,
    };
    DrawRectangleRec(rec, cell_color);

    {
      const char *text = p->tape[i].from;
      float q = (1 - p->tape[i].t);
      float font_size = FONT_SIZE*q;
      Vector2 text_size = MeasureTextEx(p->font, text, font_size, 0);
      Vector2 position = { .x = rec.x, .y = rec.y };
      position = Vector2Add(position, Vector2Scale(cell_size, 0.5));
      position = Vector2Subtract(position, Vector2Scale(text_size, 0.5));
      DrawTextEx(p->font, text, position, font_size, 0, ColorAlpha(background_color, q));
    }

    {
      const char *text = p->tape[i].to;
      float q = p->tape[i].t;
      float font_size = FONT_SIZE*q;
      Vector2 text_size = MeasureTextEx(p->font, text, font_size, 0);
      Vector2 position = { .x = rec.x, .y = rec.y };
      position = Vector2Add(position, Vector2Scale(cell_size, 0.5));
      position = Vector2Subtract(position, Vector2Scale(text_size, 0.5));
      DrawTextEx(p->font, text, position, font_size, 0, ColorAlpha(background_color, q));
    }
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
    if (p->head.state >= HS_HALT) {
      ffmpeg_end_rendering(p->ffmpeg);
      reset_animation();
      p->ffmpeg = NULL;
      SetTraceLogLevel(LOG_INFO);
    } else {
      BeginTextureMode(p->screen);
      turing_machine(RENDER_DELTA_TIME, RENDER_WIDTH, RENDER_HEIGHT);
      p->rendering_duration += RENDER_DELTA_TIME;
      EndTextureMode();

      Image image = LoadImageFromTexture(p->screen.texture);
      if (!ffmpeg_send_frame_flipped(p->ffmpeg, image.data, image.width, image.height)) {
        ffmpeg_end_rendering(p->ffmpeg);
        reset_animation();
        p->ffmpeg = NULL;
        SetTraceLogLevel(LOG_INFO);
      }
      UnloadImage(image);
    }
  } else {
    if (IsKeyPressed(KEY_F)) {
      SetTraceLogLevel(LOG_WARNING);
      p->ffmpeg = ffmpeg_start_rendering(RENDER_WIDTH, RENDER_HEIGHT, RENDER_FPS);
      p->rendering_duration = 0.0;
      reset_animation();
    }
    turing_machine(GetFrameTime(), GetScreenWidth(), GetScreenHeight());
  }
  EndDrawing();
}
