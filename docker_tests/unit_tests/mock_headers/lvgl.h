#pragma once

#include <stdint.h>
#include <stddef.h>

typedef float lv_subject_value_t;

typedef struct _lv_subject_t {
    lv_subject_value_t value;
} lv_subject_t;

typedef struct _lv_observer_t {
    void *user_data;
} lv_observer_t;

void lv_subject_add_observer(lv_subject_t *subject, lv_observer_t *observer, void *user_data);
lv_subject_value_t lv_subject_get_float(lv_subject_t *subject);
