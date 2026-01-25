#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// Define BUILD_FOR_SIMULATOR before including any headers
#define BUILD_FOR_SIMULATOR

#include "ui_stub.h"
#include "lvgl.h"

// Include the UI header files to get the function declarations
#include "../../main/ui/ui.h"
#include "../../main/ui/analog_dial.h"

// SDL and display related includes for simulator
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// SDL related includes
#include <SDL2/SDL.h>

// Display configuration for simulator
#define DISPLAY_HOR_RES 320
#define DISPLAY_VER_RES 170

static lv_display_t *disp = NULL;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static volatile bool draw_request = false;

// Callback to handle display flushing
static void sdl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
  // Pass the draw request to the main thread
  draw_request = true;

  // Inform LVGL that the flush is done
  lv_display_flush_ready(disp);
}

// Function to render the display to SDL
static void sdl_render()
{
  if (!draw_request)
    return;

  draw_request = false;

  // Lock texture for pixel access
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = DISPLAY_HOR_RES;
  rect.v = DISPLAY_VER_RES;

  // Render texture to screen
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

// Main loop function for Emscripten
static void main_loop(void *param)
{
  (void)param;

  // Periodically call the lv_timer handler
  lv_timer_handler();

  // Render the UI
  sdl_render();
}

int main(int argc, char **argv)
{
  printf("Starting LVGL UI Simulator\n");

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  // Create window
  window = SDL_CreateWindow("Filament Dryer UI Simulator",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            DISPLAY_HOR_RES, DISPLAY_VER_RES,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window)
  {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  // Create renderer
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
  {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // Create texture that holds the rendered image
  texture = SDL_CreateTexture(renderer,
                              SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_TARGET,
                              DISPLAY_HOR_RES, DISPLAY_VER_RES);
  if (!texture)
  {
    printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // Initialize LVGL
  lv_init();

  // Create a display
  disp = lv_display_create(DISPLAY_HOR_RES, DISPLAY_VER_RES);
  lv_display_set_flush_cb(disp, sdl_display_flush);

  // Set the display buffer
  static lv_color_t buf1[DISPLAY_HOR_RES * 100]; // Only allocate part of the screen height
  lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Initialize the UI
  init_ui();

#ifdef __EMSCRIPTEN__
  // Run the main loop continuously with Emscripten
  emscripten_set_main_loop_arg(main_loop, NULL, 0, 1);
#else
  // Native SDL main loop
  bool quit = false;
  SDL_Event event;

  while (!quit)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        quit = true;
      }

      // Handle LVGL tasks
      lv_timer_handler();

      // Render the UI
      sdl_render();

      SDL_Delay(5); // Small delay to prevent 100% CPU usage
    }
  }
#endif

  // Cleanup
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

// UI implementation code (copied from ui.c and adapted)
static lv_obj_t *needle_line;

static void set_needle_line_value(lv_obj_t *obj, int32_t v)
{
  lv_scale_set_line_needle_value(obj, needle_line, 60, v);
}

/* Initialize user interface */
void init_ui(void)
{
  lvgl_port_lock(0);

  // Create only a single simple label to test basic functionality
  static const char *TAG = "UI";
  ESP_LOGI(TAG, "UI initialization started - creating minimal test objects");

  /* Change the active screen's background color */
  lv_obj_set_style_bg_color(lv_screen_active(), lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);

  lv_obj_set_layout(lv_screen_active(), LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(lv_screen_active(), LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(lv_screen_active(), LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

  /* Create a single test label */
  lv_obj_t *test_label = lv_label_create(lv_screen_active());
  lv_label_set_text(test_label, "LVGL 9.x OK");

  /* Create round scale */
  lv_obj_t *scale_line = lv_scale_create(lv_screen_active());

  lv_obj_set_size(scale_line, 100, 100);
  lv_scale_set_mode(scale_line, LV_SCALE_MODE_ROUND_INNER);
  lv_obj_set_style_bg_opa(scale_line, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(scale_line, lv_color_white(), 0);
  lv_obj_set_style_radius(scale_line, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(scale_line, true, 0);
  lv_obj_align(scale_line, LV_ALIGN_LEFT_MID, LV_PCT(2), 0);

  lv_scale_set_label_show(scale_line, true);

  lv_scale_set_total_tick_count(scale_line, 31);
  lv_scale_set_major_tick_every(scale_line, 5);

  lv_obj_set_style_length(scale_line, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(scale_line, 10, LV_PART_INDICATOR);
  lv_scale_set_range(scale_line, 10, 40);

  lv_scale_set_angle_range(scale_line, 270);
  lv_scale_set_rotation(scale_line, 135);

  needle_line = lv_line_create(scale_line);

  lv_obj_set_style_line_width(needle_line, 3, LV_PART_MAIN);
  lv_obj_set_style_line_color(needle_line, lv_palette_darken(LV_PALETTE_RED, 3), LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

  set_needle_line_value(scale_line, 32);

  struct analog_dial_t *dial = create_analog_dial(lv_screen_active());
  set_analog_dial_value(dial, 75);

  /* Try any of the demos by uncommenting one of the lines below */
  // lv_demo_widgets();
  // lv_demo_keypad_encoder();
  // lv_demo_benchmark();
  // lv_demo_stress();

  ESP_LOGI(TAG, "UI initialization completed - objects created successfully");
  lvgl_port_unlock();
}

// Analog dial implementation code (copied from analog_dial.c and adapted)
struct analog_dial_t
{
  lv_obj_t *container;
  lv_obj_t *scale;
  lv_obj_t *needle_line;
  int32_t needle_length;
};

void set_analog_dial_value(struct analog_dial_t *dial, int32_t value)
{
  lv_scale_set_line_needle_value(
      dial->scale,
      dial->needle_line,
      dial->needle_length,
      value);
}

struct analog_dial_t *create_analog_dial(
    lv_obj_t *parent)
{
  /* Dial is bigger that the window it occupies so we need to wrap it
    in a container and use padding to shift it to the right location.
              ◄──container_width──►

             ┌──────Container──────┐
      ▲ ┌───────────────────────────────┐
      │ │    │                     │    │
      │ │    │                     │    │
      d │    │          +──────────┼────┼────
      i │    │                     │    │  |
      a │    │                     │    │  │
      l │    │                     │    │ Offset
      │ │    └─────────────────────┘    │  │
      d │                               │  ▼
      i │               +───────────────┼────
      a │                               │
      m │                               │
      e │                               │
      t │                               │
      e │                               │
      r │                               │
      │ │                               │
      ▼ └───────────────────────────────┘
          ◄────────dial_diameter────────►

  */

  /* Constants */
  int32_t major_tick_length = 10;
  int32_t major_tick_width = 1;
  int32_t minor_tick_length = 5;
  int32_t minor_tick_width = 1;

  /* Calculate padding values */
  int d2 = ANALOG_DIAL_DIAMETER / 2;
  int c2 = ANALOG_DIAL_CONTAINER_HEIGHT / 2;
  int horiz_padding = (ANALOG_DIAL_CONTAINER_WIDTH - ANALOG_DIAL_DIAMETER) / 2;
  int top_padding = d2 + ANALOG_DIAL_VERT_SHIFT - c2;
  int bottom_padding = d2 - ANALOG_DIAL_VERT_SHIFT - c2;

  struct analog_dial_t *dial = malloc(sizeof(struct analog_dial_t));

  lv_obj_t *container = lv_obj_create(parent);
  dial->container = container;
  lv_obj_set_style_bg_color(container, lv_color_white(), LV_PART_MAIN);
  // lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_top(container, top_padding, LV_PART_MAIN);
  lv_obj_set_style_pad_left(container, horiz_padding, LV_PART_MAIN);
  lv_obj_set_style_pad_right(container, horiz_padding, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(container, bottom_padding, LV_PART_MAIN);
  lv_obj_set_size(container, ANALOG_DIAL_CONTAINER_WIDTH, ANALOG_DIAL_CONTAINER_HEIGHT);
  lv_obj_set_style_radius(container, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
  /* Remove scrollbar */
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

  /* Create round scale */
  lv_obj_t *scale_line = lv_scale_create(container);
  dial->scale = scale_line;

  lv_obj_set_size(scale_line, ANALOG_DIAL_DIAMETER, ANALOG_DIAL_DIAMETER);
  lv_obj_set_style_radius(scale_line, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(scale_line, 0, LV_PART_MAIN);
  lv_obj_set_style_margin_all(scale_line, 0, LV_PART_MAIN);
  /* Set origin of scale to be top middle */
  // lv_obj_set_align(scale_line, LV_ALIGN_TOP_MID);
  lv_obj_center(scale_line);

  lv_scale_set_mode(scale_line, LV_SCALE_MODE_ROUND_OUTER);
  lv_obj_set_style_bg_opa(scale_line, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(scale_line, lv_color_white(), 0);
  lv_obj_set_style_radius(scale_line, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(scale_line, false, LV_PART_INDICATOR);
  lv_obj_set_style_clip_corner(scale_line, false, LV_PART_MAIN);
  lv_scale_set_draw_ticks_on_top(scale_line, true);

  lv_scale_set_label_show(scale_line, true);

  lv_scale_set_total_tick_count(scale_line, 31);
  lv_scale_set_major_tick_every(scale_line, 5);
  lv_obj_set_style_pad_radial(scale_line, -10, LV_PART_INDICATOR);
  lv_obj_set_style_text_font(scale_line, &lv_font_montserrat_12, LV_PART_INDICATOR);

  lv_obj_set_style_length(scale_line, 5, LV_PART_ITEMS);
  lv_obj_set_style_line_width(scale_line, 1, LV_PART_ITEMS);
  lv_obj_set_style_length(scale_line, 10, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(scale_line, 1, LV_PART_INDICATOR);
  /* Rotate labels to match tick angle */
  // lv_obj_set_style_transform_rotation(scale_line, LV_SCALE_LABEL_ROTATE_MATCH_TICKS + 900, LV_PART_INDICATOR);

  lv_scale_set_range(scale_line, ANALOG_DIAL_RANGE_START, ANALOG_DIAL_RANGE_END);

  lv_scale_set_angle_range(scale_line, ANALOG_DIAL_ANGLE_RANGE);
  lv_scale_set_rotation(scale_line, 180 + (180 - ANALOG_DIAL_ANGLE_RANGE) / 2);

  lv_obj_t *needle_line = lv_line_create(scale_line);
  dial->needle_line = needle_line;

  lv_obj_set_style_line_width(needle_line, 1, LV_PART_MAIN);
  lv_obj_set_style_line_color(needle_line, lv_palette_darken(LV_PALETTE_RED, 3), LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

  dial->needle_length = d2 + major_tick_length;

  set_analog_dial_value(dial, 53);
  // lv_scale_set_line_needle_value(scale_line, needle_line, d2 + major_tick_length, 53);

  return dial;
}

void free_analog_dial(struct analog_dial_t *dial)
{
  /* deletes recursively delete children too but we will still try to delete
     from bottom up explicitly */
  lv_obj_delete(dial->needle_line);
  lv_obj_delete(dial->scale);
  lv_obj_delete(dial->container);
  free(dial);
}