# muon-detector

### Coincidence cosmic muon detector — STM32F405, derived from CosmicWatch (MIT)

Two scintillator+SiPM channels in coincidence to distinguish real
cosmic rays from electronic noise, with barometric correction of the
measured rate. Directly links the MadGraph particle-physics project to
the embedded work in the other repositories.

## What's real and verified (19 tests)

| Component | Verified how |
|---|---|
| **Pulse train generation** (Poisson) | Standard model for a cosmic-ray flux |
| **Coincidence detection** | **The most important test**: two independent high-rate noise streams produce only a low rate of accidental coincidence, while genuinely correlated events (with realistic measurement jitter) are almost all recovered — coincidence genuinely rejects noise, not just "runs without error" |
| **Barometric correction** | **Two real physics sign errors caught by the tests** and fixed: the correction was going in the wrong direction (a measurement at higher pressure must be corrected upward, not downward — more pressure = more atmospheric absorption = lower measured rate). Validated by regenerating synthetic data with a known coefficient and checking that the fit recovers it exactly |
| **C port of coincidence detection** | Numerically compared to Python via `ctypes`, at a realistic detector rate. Quantization limit at very high event density explicitly documented rather than hidden (see `tests/test_coincidence_c_matches_python.py`) |
| **STM32F405 firmware** | Actually compiles and **links** (24.5 KB) — reuses the already-validated I2C/clock drivers from the nanodrone project (same MCU family) |

## What is NOT verified

- **The analog circuit** (amplifier, comparator, DC-DC converter for
  SiPM bias): real components cited in `docs/hardware.md`, but no
  schematic or component values provided — the original CosmicWatch
  documentation (full published schematic) is the right starting
  point rather than blindly re-routing this circuit here.
- **This detector's actual barometric coefficient**: the fitting
  module (`barometric.py`) is ready and tested on synthetic data, but
  the real value of β must be measured on the assembled detector.
- **No measurement on a real detector** — no hardware available in
  this development environment.

## Bill of materials

**[docs/hardware.md](docs/hardware.md)** — real components checked
against public CosmicWatch documentation (onsemi C-Series SiPM,
polystyrene+PPO+POPOP 5×5×1cm scintillator, MAX5026 DC-DC converter),
pinout, and why two channels rather than one.

## Repository structure

```
sim/muon_sim/
├── pulse_train.py       # pulse train generation (Poisson)
├── coincidence.py         # coincidence detection + statistics
└── barometric.py            # barometric correction (fit + application)
firmware/
├── Core/Inc, Core/Src        # pulse_timer (TIM2 capture), coincidence.c, bme280.c
├── Drivers/                    # vendored CMSIS headers (STM32F405)
├── startup/                     # linker script + startup
└── Makefile                      # arm-none-eabi-gcc build
tests/                             # 19 tests, including C/Python cross-validation
docs/hardware.md                     # bill of materials + pinout
.github/workflows/build.yml            # CI: tests + firmware compilation
```

## Installation and usage

```bash
pip install -r sim/requirements.txt
pytest tests/ -v                    # 19 tests

cd firmware
make                                 # produces build/muon_detector.elf
```

## License

MIT for original code — see `LICENSE`. Vendored CMSIS files under
Apache 2.0 — see `THIRD_PARTY_LICENSES.md`.
