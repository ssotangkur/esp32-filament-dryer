// Wokwi custom chip: closed-loop heater + dual-NTC plant.
// Follows docs.wokwi.com custom-chips API (chip_init, pin_watch, timer events).
// Thermal model mirrors wokwi/plant_model.py; Steinhart-Hart coeffs are passed
// as attributes so air/heater sensors match include/temp.h calibration.

#include "wokwi-api.h"
#include <math.h>
#include <stdlib.h>

typedef struct {
  pin_t pwm_pin;
  pin_t adc_air_pin;
  pin_t adc_heater_pin;
  timer_t timer;

  // Thermal state
  float t_heater;
  float t_air;
  float t_amb;
  float p_max_w, c_heater, c_air, k_ha, k_amb;
  float dt_s;

  // Sensor model
  float a_air, b_air, c_air_sh;
  float a_htr, b_htr, c_htr_sh;
  float r_series, v_ref, atten_max_v, sigma;
  unsigned rng_state;

  // PWM measurement
  uint8_t duty;          // 0-255 estimated
  uint32_t high_us;      // accumulated high time in window
  uint32_t window_us;
  uint32_t last_change_us;

  const char *fault;     // NULL or "heater-stuck-on" / "sensor-disconnect"
} chip_state_t;

static float sh_temp(float r, float A, float B, float C) {
  if (r <= 0) return -273.15f;
  float ln_r = logf(r);
  float inv = A + B * ln_r + C * ln_r * ln_r * ln_r;
  if (inv <= 0) return -273.15f;
  return 1.0f / inv - 273.15f;
}

static float temp_to_r(float t_c, float A, float B, float C) {
  float lo = 100.0f, hi = 5000000.0f;
  for (int i = 0; i < 48; i++) {
    float mid = 0.5f * (lo + hi);
    if (sh_temp(mid, A, B, C) > t_c) lo = mid;
    else hi = mid;
  }
  return 0.5f * (lo + hi);
}

static float randn(chip_state_t *s) {
  // xorshift + Box-Muller lite (deterministic per seed attr)
  s->rng_state ^= s->rng_state << 13;
  s->rng_state ^= s->rng_state >> 17;
  s->rng_state ^= s->rng_state << 5;
  float u = (s->rng_state & 0xffffff) / (float)0x1000000 + 1e-6f;
  return sqrtf(-2.0f * logf(u)) * cosf(6.2831853f * u);
}

static float temp_to_volts(chip_state_t *s, float t, float A, float B, float C) {
  if (s->fault && !strcmp(s->fault, "sensor-disconnect")) return 0.0f;
  float r = temp_to_r(t, A, B, C);
  float v = s->v_ref * r / (r + s->r_series);
  if (v > s->atten_max_v) v = s->atten_max_v;
  return v;
}

static void chip_timer_event(void *user_data) {
  chip_state_t *s = (chip_state_t*)user_data;

  // Estimate duty from measured high-time in window (default: hold last)
  if (s->window_us > 0) {
    s->duty = (uint8_t)(255.0f * s->high_us / (float)s->window_us + 0.5f);
    s->high_us = 0;
    s->window_us = 0;
  }
  uint8_t duty = s->duty;
  if (s->fault && !strcmp(s->fault, "heater-stuck-on")) duty = 255;

  float p_in = s->p_max_w * duty / 255.0f;
  float dth = (p_in - s->k_ha * (s->t_heater - s->t_air)) / s->c_heater;
  float dta = (s->k_ha * (s->t_heater - s->t_air)
             - s->k_amb * (s->t_air - s->t_amb)) / s->c_air;
  s->t_heater += dth * s->dt_s;
  s->t_air += dta * s->dt_s;

  float v_air = temp_to_volts(s, s->t_air, s->a_air, s->b_air, s->c_air_sh);
  float v_htr = temp_to_volts(s, s->t_heater, s->a_htr, s->b_htr, s->c_htr_sh);
  if (s->sigma > 0) {
    v_air += randn(s) * (s->sigma / 4095.0f) * s->atten_max_v;
    v_htr += randn(s) * (s->sigma / 4095.0f) * s->atten_max_v;
  }
  pin_dac_write(s->adc_air_pin, v_air);
  pin_dac_write(s->adc_heater_pin, v_htr);
}

static void pwm_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *s = (chip_state_t*)user_data;
  uint32_t now = get_time_us();
  uint32_t dt = now - s->last_change_us;
  s->last_change_us = now;
  // value==HIGH means pin just went high; accumulate previous level time
  if (!value) s->high_us += dt; // pin was high during dt
  s->window_us += dt;
}

void chip_init(void) {
  chip_state_t *s = malloc(sizeof(chip_state_t));
  s->pwm_pin = pin_init("PWM_IN", INPUT);
  s->adc_air_pin = pin_init("ADC_AIR", ANALOG);
  s->adc_heater_pin = pin_init("ADC_HEATER", ANALOG);

  s->t_amb = attr_read_float(attr_init_float("tAmb", 25));
  s->t_heater = s->t_amb;
  s->t_air = s->t_amb;
  s->p_max_w = attr_read_float(attr_init_float("pMaxW", 60));
  s->c_heater = attr_read_float(attr_init_float("cHeater", 8));
  s->c_air = attr_read_float(attr_init_float("cAir", 120));
  s->k_ha = attr_read_float(attr_init_float("kHeaterToAir", 1.2));
  s->k_amb = attr_read_float(attr_init_float("kAirToAmb", 0.4));
  s->dt_s = attr_read_float(attr_init_float("timestepMs", 10)) / 1000.0f;
  s->r_series = attr_read_float(attr_init_float("rSeries", 100000));
  s->v_ref = attr_read_float(attr_init_float("vRef", 3.3));
  s->atten_max_v = attr_read_float(attr_init_float("attenMaxV", 2.2));
  s->sigma = attr_read_float(attr_init_float("sigmaCounts", 8));
  s->rng_state = (unsigned)attr_read_float(attr_init_float("seed", 1)) + 1;

  // SH coeffs: set via attributes to match include/temp.h (defaults = air sensor fit)
  s->a_air = attr_read_float(attr_init_float("aAir", 0.000828f));
  s->b_air = attr_read_float(attr_init_float("bAir", 0.000208f));
  s->c_air_sh = attr_read_float(attr_init_float("cAir", 0.000000089f));
  s->a_htr = attr_read_float(attr_init_float("aHtr", 0.000828f));
  s->b_htr = attr_read_float(attr_init_float("bHtr", 0.000208f));
  s->c_htr_sh = attr_read_float(attr_init_float("cHtr", 0.000000089f));

  s->duty = 0;
  s->high_us = 0;
  s->window_us = 0;
  s->last_change_us = get_time_us();
  s->fault = NULL;

  pin_watch(s->pwm_pin, pwm_change, s);
  timer_config_t tc = {
    .callback = chip_timer_event,
    .user_data = s,
    .period_us = (uint32_t)(s->dt_s * 1000000),
  };
  s->timer = timer_init(&tc);
  timer_start(s->timer);
}
