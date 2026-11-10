"""Host reference implementation of the heater_plant custom chip.

Mirrors chips/heater_plant/heater_plant.c so the thermal + NTC model can be
tested on the host without Wokwi. Wokwi chip is the integration wrapper;
this is the logic under test.
"""

import math
import random


def steinhart_hart_temp(r_ohms, A, B, C):
    if r_ohms <= 0:
        return -273.15
    ln_r = math.log(r_ohms)
    inv_t = A + B * ln_r + C * ln_r ** 3
    if inv_t <= 0:
        return -273.15
    return 1.0 / inv_t - 273.15


def temp_to_resistance(t_c, A, B, C, iters=64):
    """Inverse Steinhart-Hart via bisection (R is monotonic decreasing in T)."""
    lo, hi = 100.0, 5_000_000.0
    for _ in range(iters):
        mid = 0.5 * (lo + hi)
        t_mid = steinhart_hart_temp(mid, A, B, C)
        # higher R -> lower T
        if t_mid > t_c:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def temp_to_adc_counts(t_c, A, B, C, r_series=100000.0, vref=3.3, atten_max_v=2.2, rng=None, sigma_counts=8.0):
    r = temp_to_resistance(t_c, A, B, C)
    v = vref * r / (r + r_series)
    # ADC_ATTEN_DB_6: saturates at atten_max_v
    v = min(v, atten_max_v)
    counts = v / atten_max_v * 4095.0
    if rng is not None:
        counts += rng.gauss(0.0, sigma_counts)
    return max(0, min(4095, int(round(counts))))


class HeaterPlant:
    """Two-mass thermal model: fast heater element + slow air volume."""

    def __init__(self, p_max_w=60.0, c_heater=8.0, c_air=120.0,
                 k_heater_to_air=1.2, k_air_to_amb=0.4, t_amb=25.0,
                 dt=0.01, seed=1):
        self.p_max_w = p_max_w
        self.c_heater = c_heater
        self.c_air = c_air
        self.k_ha = k_heater_to_air
        self.k_amb = k_air_to_amb
        self.t_amb = t_amb
        self.dt = dt
        self.rng = random.Random(seed)
        self.t_heater = t_amb
        self.t_air = t_amb
        self.duty = 0  # 0-255
        self.fault = None  # None | 'sensor-disconnect' | 'stuck-adc' | 'heater-stuck-on'

    def set_duty(self, duty):
        self.duty = max(0, min(255, int(duty)))

    def set_ambient(self, t_amb):
        self.t_amb = t_amb

    def inject_fault(self, fault):
        self.fault = fault

    def step(self):
        duty = self.duty
        if self.fault == 'heater-stuck-on':
            duty = 255
        p_in = self.p_max_w * duty / 255.0
        dth = (p_in - self.k_ha * (self.t_heater - self.t_air)) / self.c_heater
        dta = (self.k_ha * (self.t_heater - self.t_air)
               - self.k_amb * (self.t_air - self.t_amb)) / self.c_air
        self.t_heater += dth * self.dt
        self.t_air += dta * self.dt
        return self.t_heater, self.t_air

    def run(self, seconds, duty=None):
        if duty is not None:
            self.set_duty(duty)
        n = int(seconds / self.dt)
        for _ in range(n):
            self.step()
        return self.t_heater, self.t_air
