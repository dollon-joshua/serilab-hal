# serilab-hal

A tiny C++ hardware-abstraction layer for the [Daisy Seed](https://electro-smith.com/products/daisy-seed)
that lets firmware read inputs and drive outputs identically whether they're
real GPIO/ADC pins or values coming from [Serilab](https://serilab.site)
(a browser-based dashboard for wiring up virtual buttons, knobs, faders,
encoders, LEDs and an LCD over USB serial, with no hardware attached).

## Why

Application code is written once against the `DashboardHAL` interface. You
flash the **same firmware logic** with one of two backends:

- `SerialHAL` — reads/writes the `channel:value\n` USB-CDC protocol so the
  [Serilab dashboard](https://serilab.site) can drive your inputs and observe
  your outputs before any hardware exists.
- `HardwareHAL` — reads real GPIO/ADC pins on the Daisy Seed once you've
  wired up physical pots, buttons, switches and LEDs.

Switching between them is a single compile flag.

## Layout

```
hal/
  DashboardHAL.h   # abstract interface (Init, Update, Get*/Set*)
  SerialHAL.h      # USB-CDC "channel:value" protocol implementation
  HardwareHAL.h    # real GPIO/ADC implementation
example-hal/
  main.cpp         # minimal example wiring up both backends
  Makefile
```

## Serial protocol

Plain text over USB CDC, one message per line: `channel:value\n`.

| Direction | Example | Meaning |
|---|---|---|
| Dashboard → Seed | `pot1:64` | Analog channel `pot1` set to 64 (0–127) |
| Dashboard → Seed | `btn1:1` | Digital channel `btn1` pressed |
| Seed → Dashboard | `led1:1` | LED channel `led1` turned on |
| Seed → Dashboard | `lcd1:12.3` | LCD channel `lcd1` set to arbitrary text |

Analog values are 0–127 on the wire, normalized to `0.0–1.0` by `GetAnalog`/
`SendAnalog`. Encoder channels (registered via `RegisterEncoder`) accumulate
incoming values as deltas rather than absolute positions.

## Usage

```cpp
#include "daisy_seed.h"

#ifdef DASHBOARD_TEST
  #include "../hal/SerialHAL.h"
  using HAL = SerialHAL;
#else
  #include "../hal/HardwareHAL.h"
  using HAL = HardwareHAL;
#endif

daisy::DaisySeed hw;
HAL hal(hw);

int main() {
#ifdef DASHBOARD_TEST
    hal.RegisterEncoder("enc1");
#else
    hal.RegisterAnalog("pot1", hw.GetPin(21));
    hal.RegisterButton("btn1", hw.GetPin(28));
    hal.RegisterLed("led1", hw.GetPin(22));
#endif
    hal.Init();

    while (true) {
        hal.Update();
        hal.SetLed("led1", hal.GetButton("btn1"));
        System::Delay(10);
    }
}
```

See [`example-hal/main.cpp`](example-hal/main.cpp) for a fuller example
covering analog, button, toggle, encoder, LED and LCD channels in both modes.

## Building

Requires [libDaisy](https://github.com/electro-smith/libDaisy) checked out as
a sibling directory (`../libDaisy` relative to `example-hal/`):

```bash
git clone https://github.com/electro-smith/libDaisy
# build libDaisy per its own README, then:

cd example-hal
make test   # serial/dashboard mode — control everything from Serilab over USB
make        # hardware mode — real pots/buttons/LEDs on the pins you registered
```

Flashing uses the standard libDaisy/OpenOCD toolchain; see libDaisy's docs for
programmer setup.

## Companion dashboard

[Serilab](https://serilab.site) ([source](https://github.com/dollon-joshua/serial-dashboard))
is the browser dashboard this HAL's `SerialHAL` backend talks to — drag-and-drop
widgets, map each to a serial channel, and control/observe this firmware over
USB before wiring any physical hardware.

## License

MIT — see [LICENSE](LICENSE).
