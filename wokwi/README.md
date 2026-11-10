# Wokwi closed-loop heater simulation

## Files
- `wokwi.toml` — firmware/elf paths for `wokwi-cli` / `idf.py wokwi`
- `diagram.json` — `esp32-s3-devkitc-1` + `chip-heater-plant` wiring:
  `GPIO3 -> PWM_IN`, `plant ADC_AIR -> GPIO1 (ADC_CHANNEL_0)`,
  `plant ADC_HEATER -> GPIO2 (ADC_CHANNEL_1)`
- `chips/heater_plant/` — custom chip (thermal two-mass + inverse
  Steinhart-Hart NTC). Attributes mirror `include/temp.h` + plant params.
- `plant_model.py` — host reference of the same model (testable without Wokwi)
- `test_plant_model.py` — `python -m pytest wokwi/ -v` (7 passed)

## Hooks (from prior discussion)
- Pins: `PWM_IN`, `ADC_AIR`, `ADC_HEATER`, `GND` (+ optional `FAULT_IN`/`DBG_TX`)
- Attrs: `pMaxW/cHeater/cAir/kHeaterToAir/kAirToAmb/tAmb/timestepMs/seed`,
  `rSeries/vRef/attenMaxV/sigmaCounts`, SH coeffs `aAir/bAir/cAir/aHtr/bHtr/cHtr`
- Test faults: `sensor-disconnect`, `heater-stuck-on` (see `HeaterPlant.inject_fault`)

## Run in CI (needs `WOKWI_CLI_TOKEN`)
```
wokwi-cli --timeout 30000 --serial-log-file sim.log .
```

## Known finding
- `include/temp.h:13-14` air point 1 (`25C @ 988000 ohms`) yields a
  negative-A SH fit (non-physical; heater pts fit normally). Flagged by
  `test_air_calibration_is_suspect`. Verify against the actual thermistor
  datasheet — likely `98800.0f` typo.
- C chip follows the Custom Chips API from docs but is not yet compiled
  against `wokwi-api.h`; host model is the tested artifact.
