# BLACKOUT RUN

BLACKOUT RUN is a finished Game Boy Advance arcade game about a bicycle courier whose successful routes teach the city where to move. Deliver parcels, ring the bell to split crowds, brake through tight corners, and watch yesterday's shortcut become tomorrow's congestion.

## Play

- D-pad: ride and steer
- A: ring bell
- B: brake
- Start: pause
- Select: route-memory overlay during a shift

Bell, delivery, and missed-job cues use the GBA's native PSG channels.

The story campaign runs for three two-job shifts and ends with a blackout run. Free Ride is unlocked from the title menu immediately.

## Build and validate

Requires devkitARM and libgba under `/opt/devkitpro`.

```sh
make
make test verify size
```

The ROM is written to `blackout-run.gba`. The static showcase is under `site/` and `make site` places a verified ROM download in it.

## Release scope

This public release contains the complete three-shift arcade campaign, its
runtime art, tests, native evidence, and static showcase. Local concept and
visual-development working material is intentionally not part of the release.
