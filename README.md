# Tankerkoenig

Tankerkoenig is a planned native AmigaOS fuel-price finder. It will resolve locations through Open-Meteo and retrieve current German E5, E10 and diesel prices through the Tankerkoenig API. Each user will supply a personal API key.

## Current Status

Phase 1 is implemented:

- C89-compatible AmigaOS 1.3 project foundation.
- bebbo/amiga-gcc build using the nix13 runtime.
- Small launcher that starts the application core with a 65000-byte stack.
- Dynamically resizable Workbench window with refresh and close handling.
- Fallback to a minimum-size window at position `0,0`.
- Central cleanup for windows, libraries and allocated memory.
- Large future network and JSON buffers allocated in public memory instead of the process stack.
- Validated configuration for API key, location, coordinates, radius, fuel, sorting, open-only filtering and update interval.
- A user-owned API key is loaded and saved but never displayed or logged.

Phase 2 adds validated loading and saving of `Tankerkoenig.conf`. The personal API key is never displayed or logged. Network requests and JSON parsing are not implemented yet.

## Build

```sh
make clean && make
```

Build output: `build/Tankerkoenig` and `build/tkcore`. Keep both in the same directory and start `Tankerkoenig`.

## Development Plan

See [PLAN.md](PLAN.md).
