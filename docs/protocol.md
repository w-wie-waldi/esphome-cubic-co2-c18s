# CO2-C18S protocol notes

Everything here was obtained by sniffing the UART of a `CO2-C18S_V6A` module taken
out of a Moes BSS-X-CO2-U CO₂ monitor. There is no public datasheet for this
exact variant, so treat these notes as empirical rather than official.

## Physical layer

| | |
|---|---|
| Baud rate | 9600, 8N1, no flow control |
| Logic level | 3.3 V TTL on TX/RX |
| Supply | 5 V |
| Direction | **sensor → host only**, active upload |

## Frame

The module sends a 7-byte frame roughly every **3 seconds**, unprompted:

| Offset | Bytes | Meaning |
|---|---|---|
| 0 | `5C` | Header |
| 1 | `00` | constant |
| 2 | `01` | constant |
| 3 | `02` | constant |
| 4 | `HI` | CO₂ high byte |
| 5 | `LO` | CO₂ low byte |
| 6 | `XOR` | Checksum |

Checksum: XOR over bytes 0…5.

CO₂ concentration in ppm: `(bytes[4] << 8) | bytes[5]`.

### Example

```
5C 00 01 02 06 8F D6
```

- `0x068F` = 1679 ppm
- Checksum check: `5C ^ 00 ^ 01 ^ 02 ^ 06 ^ 8F = D6` ✓

## Two traps

**Bytes 3–4 are not the value.** They read `02 06` on most units and never change.
Reading there gives a rock-solid, completely fake 518 ppm (`0x0206`). The value
sits one byte further along, at offsets 4–5.

**Frames are not preceded by a request.** The module does not answer queries —
it only streams. A parser that waits for a reply sees nothing at all.

## Why the built-in platforms don't work

| Platform | Why it fails |
|---|---|
| `cm1106` | Expects query/response with header `0x64`. The C18S never answers → `Reading from UART timed out at byte 0`. |
| `hc8` | Uses the `0x5C` header like this module, but starts the conversation with `64 69 03 5E 4E`. The C18S ignores the query and never sends that frame shape. |
| `senseair`, `mhz19`, `scd30`, `scd4x` | Different protocols entirely (different headers, or I²C). |
| `tuya` / TuyaMCU | Only works if the original Tuya radio module is still attached and translating. |

That leaves two options: force the module into a query mode (no evidence the OEM
firmware allows it), or parse the active stream. This component parses the stream.

## Timing behaviour

- One frame every ~3000 ms, jitter of a few dozen ms.
- No warm-up burst, no periodic status frames — just the same 7 bytes.
- After power-up the first frame arrives within a few seconds.

Because the cycle is fixed, any rate limit you configure can only ever be hit in
multiples of 3 s. That is why the component has no `update_interval` of its own
and leaves throttling to ESPHome's `throttle` filter.

## Verifying the stream

```yaml
uart:
  - id: co2_uart
    rx_pin: GPIO18
    baud_rate: 9600
    debug:
      direction: RX
      dummy_receiver: true
      after:
        delimiter: "\n"
      sequence:
        - lambda: UARTDebug::log_hex(direction, bytes, ' ');
```

You should see `5C ...` frames every ~3 s. Remove the `debug:` block again before
using the component: `dummy_receiver: true` consumes every byte, leaving the entity
`unknown`. Bytes without a `5C` header mean wrong baud rate or a floating RX line
(endless `00` with occasional `FF`/`FE`).

## Open questions

- Whether the module exposes a command interface at all (sleep, baseline reset).
  No valid command frame has been observed; the OEM host never sends anything.
- Accuracy specification. Not published for this variant. The behaviour matches
  single-beam NDIR sensors of its price class: good for ventilation decisions,
  not a reference instrument.
- Whether ABC can be disabled. Nothing observed suggests a command path for it.
