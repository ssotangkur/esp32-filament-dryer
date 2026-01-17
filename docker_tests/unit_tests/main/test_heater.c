#include "unity.h"
#include "heater.h"

// Tag used in heater.c
static const char *TAG = "HEATER";

void test_heater_init(void)
{
    heater_init();
}

void test_set_heat_power(void)
{
    set_heat_power(128);
    set_heat_power(0);
    set_heat_power(255);
}
