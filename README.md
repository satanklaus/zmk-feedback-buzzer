# zmk-feedback-buzzer

Sound notifications for ZMK: a passive buzzer on a PWM channel plays short
melodies, from the keymap with `&beep <melody>` or automatically on keyboard
events.

- `zmk,buzzer`: the buzzer. Melodies are its child nodes, listed as
  frequency (Hz) / duration (ms) pairs, numbered from 0 in the order they are
  written. Note macros (`NOTE_C6`, `NOTE_FS5`, `NOTE_REST`, ...) are in
  `<dt-bindings/zmk/buzzer.h>`.
- `zmk,behavior-buzzer`: plays a melody on key press. A melody always plays to
  the end, however long the key is held.
- `zmk,buzzer-triggers`: plays melodies at startup, on USB connect/disconnect,
  and when chosen layers turn on or off.
- Starting a melody while another one plays cuts the old one off.
- Built on the central (or a non-split keyboard) only: put the buzzer on that
  board.
- Other code can call `zmk_buzzer_play(dev, melody)` from
  `<zmk_buzzer/buzzer.h>`.
- ZMK Studio shows the melodies by node name.

## Usage

Add the module (ZMK_EXTRA_MODULES or west.yml), then define a buzzer, a
behavior and, optionally, triggers:

```dts
#include <dt-bindings/pwm/pwm.h>
#include <dt-bindings/zmk/buzzer.h>

/ {
    buzzer: buzzer {
        compatible = "zmk,buzzer";
        pwms = <&pwm0 0 PWM_HZ(1000) PWM_POLARITY_NORMAL>;
        duty-percent = <50>;   /* default; lower is quieter */

        beep { notes = <NOTE_A6 60>; };                                 /* &beep 0 */
        hello: hello { notes = <NOTE_C6 70 NOTE_E6 70 NOTE_G6 140>; };  /* &beep 1 */
        up: up { notes = <NOTE_E7 40 NOTE_REST 30 NOTE_E7 40>; };
        down: down { notes = <NOTE_E6 80>; };
    };

    behaviors {
        beep: buzzer {
            compatible = "zmk,behavior-buzzer";
            #binding-cells = <1>;
            buzzer = <&buzzer>;
        };
    };

    buzzer_triggers {
        compatible = "zmk,buzzer-triggers";
        startup = <&hello>;
        usb-connected = <&up>;
        usb-disconnected = <&down>;

        nav {
            layer = <2>;
            on = <&up>;
            off = <&down>;
        };
    };
};
```

Each trigger points at a melody node, so give the melodies you use there a
label. Leave a trigger out for no sound.

- `usb-connected` plays when a host enumerates the keyboard. When `startup` is
  set, the first connection after boot is silent so the two don't clash.
- ZMK counts a suspended host as connected, so a sleeping host stays quiet.
- A keyboard powered only by USB loses power when unplugged, so it can't play
  `usb-disconnected` for that. It still plays when a host goes away and the
  port stays powered (e.g. a KVM switching to another computer).

The PWM channel needs a pin. On nRF52, route it in a board or shield overlay:

```dts
&pinctrl {
    pwm0_buzzer: pwm0_buzzer {
        group1 { psels = <NRF_PSEL(PWM_OUT0, 0, 2)>; };
    };
    pwm0_buzzer_sleep: pwm0_buzzer_sleep {
        group1 { psels = <NRF_PSEL(PWM_OUT0, 0, 2)>; low-power-enable; };
    };
};

&pwm0 {
    status = "okay";
    pinctrl-0 = <&pwm0_buzzer>;
    pinctrl-1 = <&pwm0_buzzer_sleep>;
    pinctrl-names = "default", "sleep";
};
```

On a split keyboard, put this in an overlay for the central only. On the halves
the pin may belong to the matrix, and an enabled PWM would take it over.

## Wiring a passive buzzer

Use a *passive* buzzer (no built-in oscillator). An active one only beeps at its
own pitch.

**Magnetic buzzer** (about 30 mA): switch it with a logic-level N-MOSFET, like a
vibration motor:

```
 3V3 ───────────┬────────────┐
                │            │
            (buzzer)    1N4148 ▲   (cathode to 3V3)
                │            │
                ├────────────┘
                │ D
 GPIO ─[100 Ω]──┤ G   AO3400 / 2N7002
           │    │ S
        [100 kΩ]│
           │    │
 GND ──────┴────┘
```

**Piezo disc or transducer** (a few mA): it can hang straight off the pin
through a 100 Ω resistor, but is quieter. Put a 1 kΩ resistor across it so it
discharges between cycles.

Piezo transducers are loudest near their resonant frequency, usually
2–4 kHz (`NOTE_C7`–`NOTE_C8`).

## Tests

`tests/buzzer` holds ZMK native_sim tests: single notes, melodies with rests,
retriggering mid-melody, layer on/off sounds and the startup sound. They use a
logging test PWM (`zmk,buzzer-test-pwm`), so the snapshots also check the period
and duty cycle of each note. From `zmk/app`:

```sh
ZMK_EXTRA_MODULES=/path/to/zmk-feedback-buzzer \
    ./run-test.sh /path/to/zmk-feedback-buzzer/tests/buzzer
```
