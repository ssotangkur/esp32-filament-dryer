/**
 * @file lv_conf.h
 * Configuration file for LVGL
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1/8/16/32*/
#define LV_COLOR_DEPTH     32

/* Swap the 2 bytes of RGB565 color. Useful if the display has a different endianness */
#define LV_COLOR_16_SWAP   0

/* Enable features to draw on transparent background. 
 * It's required if opa, and transform_* style properties are used. */
#define LV_COLOR_SCREEN_TRANSP    0

/* Enable indexed (palette) colors */
#define LV_COLOR_CHROMA_KEY    lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* 1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()` */
#define LV_MEM_CUSTOM      0
#if LV_MEM_CUSTOM == 0
    /* Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB) */
    #define LV_MEM_SIZE    (32U * 1024U)          /* [bytes] */

    /* Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too. */
    #define LV_MEM_ADR          0
    
    /* Automatically defrag. on free. Defragmentation uses some CPU time every time a memory is freed */
    #define LV_MEM_AUTO_DEFRAG  1
#else       /*LV_MEM_CUSTOM*/
    #include <stdlib.h>
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV_MEM_CUSTOM*/

/* Use the standard `memcpy` and `memset` instead of LVGL's own functions. 
 * The standard functions might be more optimized. */
#define LV_MEMCPY_MEMSET_STD    0

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period. [ms] */
#define LV_DISP_DEF_REFR_PERIOD     30

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD    30

/* Use a custom tick source that tells the elapsed time in milliseconds.
 * It removes the need to manually update the tick with `lv_tick_inc()`) */
#define LV_TICK_CUSTOM     0
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE  "Arduino.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())     /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

/* Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings.
 *(Not so important, you can adjust it to modify default sizes and spaces)*/
#define LV_DPI_DEF                  130

/*========================
 * RENDERING CONFIGURATION
 *========================*/

/* Use independent rendering layer scheduler for each display */
#define LV_USE_PER_DISP_SCHEDULER   0

/*===================
 * OPERATING SYSTEM
 *===================*/

/* Use FreeRTOS' task notification instead of a mutex/semaphore to wake up a task waiting for VBlank */
#define LV_FREERTOS_TICK_INIT  0

/* Enable the use of tickless idle mode */
#define LV_TICKLESS_IDLE     0

/*========================
 * RENDERING ENGINE USAGE 
 *========================*/

/* Use GPU for some functions */
#define LV_USE_GPU_SDL          0

/* Enable drawing complex gradients in software */
#define LV_DRAW_COMPLEX_GRADIENTS 1

/*==================
 * WIDGET USAGE
 *================*/

#define LV_USE_ANIMIMG    1

#define LV_USE_BTN        1
#if LV_USE_BTN != 0
    #define LV_BTN_INK_EFFECT   0
#endif

#define LV_USE_BTNMATRIX  1

#define LV_USE_CANVAS     1

#define LV_USE_CHECKBOX   1

#define LV_USE_DROPDOWN   1
#define LV_DROPDOWN_POS_LAST  0

#define LV_USE_IMG        1
#define LV_USE_IMGBTN     1

#define LV_USE_LABEL      1
#if LV_USE_LABEL != 0
    #define LV_LABEL_TEXT_SELECTION         1
    #define LV_LABEL_LONG_TXT_HINT          1
#endif

#define LV_USE_LINE       1

#define LV_USE_ROLLER     1
#if LV_USE_ROLLER != 0
    #define LV_ROLLER_INF_PAGES       7
#endif

#define LV_USE_SLIDER     1

#define LV_USE_SWITCH     1

#define LV_USE_TEXTAREA   1
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME   1500
#endif

#define LV_USE_TABLE      1

/*==================
 * THEMES USAGE
 *==================*/

#define LV_USE_THEME_DEFAULT    1
#define LV_USE_THEME_BASIC      1
#define LV_USE_THEME_EMPTY      1

/*==================
 * LAYOUTS USAGE
 *==================*/

#define LV_USE_FLEX     1
#define LV_USE_GRID     1

/*====================
 * 3RD PARTS LIBRARIES
 *====================*/

#define LV_USE_FREETYPE       0
#define LV_USE_PNG            1
#define LV_USE_BMP            1
#define LV_USE_JPG            1
#define LV_USE_TJPGD          1
#define LV_USE_RLOTTIE        0

/*==================
 * OTHERS
 *==================*/

/* 1: Show memory usage and FPS in the right bottom corner */
#define LV_USE_PERF_MONITOR     0

/* 1: Show the used memory and the memory fragmentation in the left bottom corner */
#define LV_USE_MEM_MONITOR      0

/* 1: Draw random colored rectangles over the redrawn areas */
#define LV_USE_REFR_DEBUG       0

/* Maximum buffer size to use for rendering a rectangle.
 * This affects the performance of drawing larger rectangles */
#define LV_DRAW_BUF_MAX_NUM     16

/* Number of stops allowed per gradient */
#define LV_GRAD_STOP_MAX        2

/* Default cache size in bytes.
 * Used by image decoders such as PNG or JPG to cache decoded image data */
#define LV_DRAW_IMG_DECODER_CACHE_SIZE    32

/* Enable the line drawing API */
#define LV_DRAW_COMPLEX_ENABLED   1

/* Enable anti-aliasing (lines, and radiuses will be smoothed) */
#define LV_ANTIALIAS              1

/*================
 * OTHERS
 *================*/

/* Maximum number of display buffers (used for double/triple buffering) */
#define LV_DISPLAY_ROT_MAX    1

/* Enable API to take snapshot from displays */
#define LV_USE_SNAPSHOT       1

/* Enable file system APIs (lv_fs_) */
#define LV_USE_FS_IF    0

/*==================
 * DECRYPTION RELATED
 *==================*/

#define LV_USE_GPU_STM32_DMA2D  0

/*==================
 * TYPING RELATED
 *==================*/

#define LV_USE_USER_DATA      1

/*==================
 * COMPILER RELATED
 *==================*/

#define LV_ATTRIBUTE_MEM_ALIGN

#define LV_ATTRIBUTE_LARGE_RAM_AREA

/* Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that
 * should also appear on the API's of bindings. */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /* The default value just prevents GCC warning */

/*===================
 * SDL CONFIGURATION
 *===================*/
#ifndef LV_USE_SDL
#  define LV_USE_SDL        0  /* Disable SDL drivers for Emscripten */
#endif

#ifndef LV_SDL_MOUSEWHEEL_MODE
#  define LV_SDL_MOUSEWHEEL_MODE LV_SDL_MOUSEWHEEL_MODE_ENCODER
#endif

#ifndef LV_SDL_INCLUDE_PATH
#  define LV_SDL_INCLUDE_PATH <SDL.h>
#endif

#endif /*LV_CONF_H*/