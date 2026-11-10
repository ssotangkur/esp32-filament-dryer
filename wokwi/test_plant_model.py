"""SIL tests for the heater_plant model. Run: python -m pytest wokwi/ -v"""
import math

from wokwi.plant_model import (
    HeaterPlant, steinhart_hart_temp, temp_to_resistance, temp_to_adc_counts,
)

# Calibration points from include/temp.h (heater sensor; air sensor point 1
# reads 988000 ohms which yields non-physical negative-A SH fit, flagged below)
HEATER_PTS = [(25.0, 100600.0), (50.0, 35980.0), (85.0, 10420.0)]


def solve_sh_coeffs(pts):
    """Solve Steinhart-Hart A,B,C from 3 (T,R) points (T in Celsius)."""
    import numpy as np
    Y = np.array([1.0 / (t + 273.15) for t, _ in pts])
    X = np.array([[1.0, math.log(r), math.log(r) ** 3] for _, r in pts])
    return np.linalg.solve(X, Y)


def test_steinhart_roundtrip():
    A, B, C = solve_sh_coeffs(HEATER_PTS)
    for t, _ in HEATER_PTS:
        r = temp_to_resistance(t, A, B, C)
        t2 = steinhart_hart_temp(r, A, B, C)
        assert abs(t2 - t) < 0.5, (t, t2)


def test_air_calibration_is_suspect():
    """include/temp.h AIR point 1 (988k @ 25C) gives negative-A fit; flag it."""
    AIR_PTS = [(25.0, 988000.0), (50.0, 35230.0), (85.0, 10560.0)]
    A, _, _ = solve_sh_coeffs(AIR_PTS)
    assert A < 0  # documents the suspect calibration, not model failure


def test_heater_raises_temp_monotonic():
    p = HeaterPlant(seed=7)
    _, t0 = p.run(5, duty=0)
    p2 = HeaterPlant(seed=7)
    _, t1 = p2.run(60, duty=255)
    assert t1 > t0 + 5.0
    assert p2.t_heater > p2.t_air  # heater mass leads air mass


def test_cooling_when_off():
    p = HeaterPlant(seed=3)
    p.run(120, duty=255)
    hot = p.t_air
    p.run(120, duty=0)
    assert p.t_air < hot
    assert p.t_air > p.t_amb  # still above ambient after 2 min (slow air mass)


def test_door_open_drops_air_temp():
    p = HeaterPlant(seed=3)
    p.run(120, duty=255)
    before = p.t_air
    p.set_ambient(15.0)  # door opened to cold room, heater off
    p.run(120, duty=0)
    assert p.t_air < before


def test_adc_saturates_hot_and_cold():
    import numpy as np
    A, B, C = solve_sh_coeffs(HEATER_PTS)
    cold = temp_to_adc_counts(-10.0, A, B, C, rng=None)
    hot = temp_to_adc_counts(150.0, A, B, C, rng=None)
    mid = temp_to_adc_counts(50.0, A, B, C, rng=None)
    assert cold >= mid >= hot  # NTC: hot -> low R -> low V -> low counts
    assert 0 <= hot <= 4095 and 0 <= cold <= 4095


def test_fault_heater_stuck_on_overheats():
    p = HeaterPlant(seed=1)
    p.inject_fault('heater-stuck-on')
    p.run(180, duty=0)  # firmware commands off, plant ignores it
    assert p.t_heater > 120.0
