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

No network requests, API-key handling or JSON parsing are implemented yet.

## Build

```sh
make clean && make
```

Build output: `build/Tankerkoenig` and `build/tkcore`. Keep both in the same directory and start `Tankerkoenig`.

## Development Plan

See [PLAN.md](PLAN.md).
