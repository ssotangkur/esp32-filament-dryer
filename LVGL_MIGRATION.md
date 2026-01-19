# LVGL 8.4 to 9.x Migration Plan

## Overview
This document outlines the migration from LVGL 8.4 to LVGL 9.x for the ESP32 filament dryer project.

**Current State:**
- LVGL 8.4 (via `lvgl/lvgl: ^8` in idf_component.yml)
- esp_lvgl_port v2.0.0 (via `espressif/esp_lvgl_port: ^2.0.0`)
- Uses lv_meter widgets for temperature display
- Custom FPS monitoring system

**Target State:**
- LVGL 9.x (latest stable)
- Compatible esp_lvgl_port version
- lv_scale widgets replacing lv_meter
- Updated FPS monitoring using new event system

## Migration Steps

### 1. Update Dependencies
- Change `lvgl/lvgl: ^8` to `lvgl/lvgl: ^9` in `main/idf_component.yml`
- Update `espressif/esp_lvgl_port` to version compatible with LVGL 9.x (likely `^3.0.0` or newer)
- Run `idf.py reconfigure` to update component dependencies

### 2. API Function Renames
Apply these systematic renames throughout the codebase:

#### Display/Screen Functions
- `lv_scr_act()` → `lv_screen_act()`
- `lv_disp_drv_t` → `lv_display_t` (major structural change)
- `lv_disp_draw_buf_t` → removed (integrated into display)

#### Widget Function Prefixes
- `lv_btn_` → `lv_button_`
- `lv_btnmatrix_` → `lv_buttonmatrix_`
- `lv_img_` → `lv_image_`
- `lv_dropdown_` → unchanged

#### Object Functions
- `lv_obj_clear_flag()` → `lv_obj_remove_flag()`
- `lv_obj_clear_state()` → `lv_obj_remove_state()`

#### Other Changes
- `lv_coord_t` → `int32_t`
- `zoom` parameter → `scale`
- `angle` parameter → `rotation`

### 3. Replace Removed Widgets

#### lv_meter → lv_scale Migration
The `lv_meter` widget has been completely removed in LVGL 9.x and replaced with `lv_scale`. This requires significant code changes:

**Before (LVGL 8.x):**
```c
lv_obj_t *meter = lv_meter_create(parent);
lv_meter_scale_t *scale = lv_meter_add_scale(meter);
lv_meter_set_scale_ticks(meter, scale, 7, 2, 10, lv_palette_main(LV_PALETTE_GREY));
lv_meter_set_scale_range(meter, scale, 0, 120, 270, 12);
lv_meter_indicator_t *needle = lv_meter_add_needle_line(meter, scale, 4, lv_color_hex(0xFFFFFF), -10);
lv_meter_set_indicator_value(meter, needle, value);
```

**After (LVGL 9.x):**
```c
lv_obj_t *scale_obj = lv_scale_create(parent);
lv_scale_set_mode(scale_obj, LV_SCALE_MODE_ROUND_INNER);
lv_scale_set_range(scale_obj, 0, 120);
lv_scale_set_angle_range(scale_obj, 270);
lv_scale_set_rotation(scale_obj, 90);
lv_obj_t *needle = lv_scale_add_needle_line(scale_obj, 4, lv_color_hex(0xFFFFFF), -10);
lv_scale_set_indicator_value(scale_obj, needle, value);
```

### 4. Display Driver Overhaul
The display driver API has been completely restructured:

**Before (LVGL 8.x):**
```c
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = AMOLED_WIDTH;
disp_drv.ver_res = AMOLED_HEIGHT;
disp_drv.flush_cb = flush_cb;
disp_drv.draw_buf = &draw_buf;
lv_disp_drv_register(&disp_drv);
```

**After (LVGL 9.x):**
```c
lv_display_t *disp = lv_display_create(AMOLED_WIDTH, AMOLED_HEIGHT);
lv_display_set_flush_cb(disp, flush_cb);
lv_display_set_buffers(disp, buf1, buf2, buf_size_in_bytes, mode);
// Additional configuration...
```

### 5. FPS Monitoring System Rewrite
The `monitor_cb` in display drivers has been replaced with events:

**Before (LVGL 8.x):**
```c
static void fps_monitor_cb(lv_disp_drv_t *drv, uint32_t time, uint32_t px) {
    // Monitor callback implementation
}
disp_drv.monitor_cb = fps_monitor_cb;
```

**After (LVGL 9.x):**
```c
static void fps_monitor_event_cb(lv_event_t *e) {
    // Handle LV_EVENT_RENDER_READY
    lv_display_t *disp = lv_event_get_target(e);
    // Get render time and pixel count from event data
}
lv_display_add_event_cb(disp, fps_monitor_event_cb, LV_EVENT_RENDER_READY, NULL);
```

### 6. Configuration Updates
- Update `lv_conf.h` from the new LVGL 9.x template
- Review new configuration options
- Check for deprecated/removed configuration macros

### 7. Event System Updates
- Review event callback function signatures
- Update any event handling code
- Check for renamed event types

## Files Requiring Changes

1. **`main/idf_component.yml`** - Update dependency versions
2. **`main/display.c`** - Major updates:
   - Replace all `lv_meter_*` calls with `lv_scale_*`
   - Update `lv_scr_act()` calls
   - Rewrite display initialization
3. **`main/diagnostic.c`** - Update FPS monitoring for new display API
4. **`lv_conf.h`** - Update configuration if using custom config
5. **Any other files using LVGL APIs** - Apply systematic renames

## Testing Strategy

1. **Compile Test** - Ensure no compilation errors after each major change
2. **Display Test** - Verify LCD initialization and basic rendering
3. **Widget Test** - Test temperature meters (now scales) functionality
4. **FPS Monitoring Test** - Verify FPS calculation still works
5. **Touch/Input Test** - Ensure touch interactions work
6. **Performance Test** - Monitor for any performance regressions

## Rollback Plan

Since code has been committed before migration:
- Use `git checkout <commit-hash>` to revert to pre-migration state
- Keep backup branches of working LVGL 8.x code

## Expected Challenges

1. **esp_lvgl_port Compatibility** - May need newer version for LVGL 9.x support
2. **Scale Widget Differences** - lv_scale has different styling and configuration than lv_meter
3. **Display Driver Complexity** - New display API is more complex initially
4. **Event System Changes** - FPS monitoring needs complete rewrite

## Success Criteria

- Project compiles without errors
- Display initializes correctly
- Temperature scales render and update properly
- FPS monitoring provides accurate readings
- Touch interactions work as expected
- No performance degradation

## Timeline Estimate

- **Dependency Updates**: 30 minutes
- **API Renames**: 1-2 hours
- **lv_meter → lv_scale Migration**: 2-3 hours
- **Display Driver Rewrite**: 2-4 hours
- **FPS Monitoring Rewrite**: 1-2 hours
- **Testing & Debugging**: 2-4 hours

**Total Estimated Time**: 8-15 hours

## References

- [LVGL 9.0 Migration Guide](https://docs.lvgl.io/master/CHANGELOG.html#v9-0-0-2022-09-13)
- [LVGL API Documentation](https://docs.lvgl.io/master/)
- [ESP32 LVGL Port](https://components.espressif.com/components/espressif/esp_lvgl_port)
