# Tankerkoenig

Tankerkoenig is a planned native AmigaOS fuel-price finder. It will resolve locations through Open-Meteo and retrieve current German E5, E10 and diesel prices through the Tankerkoenig API. Each user will supply a personal API key.

## Current Status

Phases 1 through 4 are implemented:

- C89-compatible AmigaOS 1.3 project foundation.
- bebbo/amiga-gcc build using the nix13 runtime.
- Small launcher that starts the application core with a 65000-byte stack.
- Dynamically resizable Workbench window with refresh and close handling.
- Fallback to a minimum-size window at position `0,0`.
- Central cleanup for windows, libraries and allocated memory.
- Large future network and JSON buffers allocated in public memory instead of the process stack.
- Validated configuration for API key, location, coordinates, radius, fuel, sorting, open-only filtering and update interval.
- A user-owned API key is loaded and saved but never displayed or logged.
- Reusable HTTPS GET client with SNI, HTTP status parsing, Content-Length handling, bounded responses and up to three HTTPS redirects.
- Manual Open-Meteo transport test with the `T` key.
- Graceful startup when AmiTLS13 or the TCP/IP stack is unavailable.
- Core launcher stack increased to 131072 bytes for AmiTLS13.
- Bounded JSON parser for objects, arrays, strings, numbers, booleans and null.
- JSON escape and UTF-8 to Amiga Latin-1 conversion where representable.
- Safe skipping of unknown values with nesting, string and result-count limits.
- Host-side JSON regression fixtures and `make test-json` target.

Phase 2 adds validated loading and saving of `Tankerkoenig.conf`. Phase 3 adds a reusable in-memory HTTPS client through AmiTLS13 2.0. Phase 4 adds a bounded, allocation-free JSON token parser with Amiga Latin-1 conversion. Production API requests are not implemented yet.

## Build

```sh
make clean && make
```

Build output: `build/Tankerkoenig` and `build/tkcore`. Keep both in the same directory and start `Tankerkoenig`.

## Development Plan

See [PLAN.md](PLAN.md).

## HTTPS Test

Install `amitls13.library` version 2.0 or newer in `LIBS:` and start the
application through the `Tankerkoenig` launcher. The status line reports
whether HTTPS initialization succeeded. Press `T` to fetch a small Open-Meteo
geocoding response. A successful request displays `HTTPS test successful`.

Do not start `tkcore` directly: the launcher supplies the 131072-byte stack
recommended by the AmiTLS13 SDK.

### Current TLS Security

AmiTLS13 2.0 currently provides encrypted TLS 1.2 transport and SNI, but it does
not yet validate certificate authorities or hostnames. Tankerkoenig therefore
uses the documented insecure transport flag for now. This protects traffic from
plain-text observation but does not protect against an active man-in-the-middle
attack. Production certificate verification remains dependent on a future
AmiTLS13 implementation.
