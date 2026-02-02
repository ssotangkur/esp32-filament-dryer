/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#include <emscripten.h>
#include "lvgl.h"
// #include "demos/lv_demos.h"
// #include "examples/lv_examples.h"
#include "ui/ui.h"

/* Forward declaration for mock data generator */
void mock_data_init(void);

/* Custom tick function using browser's high-resolution timer */
static uint32_t custom_tick_get(void)
{
    return (uint32_t)emscripten_get_now();
}

/*********************
 *      DEFINES
 *********************/

/*On OSX SDL needs different handling*/
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
#if __APPLE__ && TARGET_OS_MAC
#define SDL_APPLE
#endif
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
static int tick_thread(void *data);
static void memory_monitor(lv_timer_t *param);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_display_t *disp1 = NULL;

int monitor_hor_res = 170; /* Set to our T-Display S3 width */
int monitor_ver_res = 320; /* Set to our T-Display S3 height */

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void do_loop(void *arg);

/* Allows disabling CHOSEN_DEMO */
static void lv_example_noop(void)
{
}

int main(int argc, char **argv)
{
  printf("Starting with screen resolution of %dx%d px\n", monitor_hor_res, monitor_ver_res);

  /*Initialize LittlevGL*/
  lv_init();

  /* Set custom tick callback BEFORE hal_init for accurate browser timing
   * This overrides the default tick function before SDL sets it */
  lv_tick_set_cb(custom_tick_get);

  /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
  hal_init();

  init_ui();

  /* Start mock data generator for UI simulator */
  mock_data_init();

  /* Use 0 fps for requestAnimationFrame (browser's natural 60fps), 
   * false for simulate_infinite_loop for better performance */
  emscripten_set_main_loop_arg(do_loop, NULL, 0, 0);
}

void do_loop(void *arg)
{
  /* Call the lv_timer handler - this will handle all LVGL tasks including
   * the display refresh timer which runs at 60fps (16ms period) */
  lv_timer_handler();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
  disp1 = lv_sdl_window_create(monitor_hor_res, monitor_ver_res);

  lv_group_t *g = lv_group_create();
  lv_group_set_default(g);

  lv_sdl_mouse_create();
  lv_indev_t *mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_group(mousewheel, lv_group_get_default());

  lv_indev_t *keyboard = lv_sdl_keyboard_create();
  lv_indev_set_group(keyboard, lv_group_get_default());
}
