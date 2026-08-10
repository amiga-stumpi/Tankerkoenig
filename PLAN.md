# Tankerkoenig Development Plan

## Project Goals

- Native, dynamically resizable AmigaOS Workbench application.
- AmigaOS 1.3 and m68k compatibility using bebbo/amiga-gcc and `-mcrt=nix13`.
- Location search through the Open-Meteo geocoding API.
- Current German fuel prices through the Tankerkoenig API.
- HTTPS through `amitls13.library` and networking through `bsdsocket.library`.
- Every user supplies a personal Tankerkoenig API key.
- Small bounded JSON parser without a large external runtime.

## Phase 1: Project Foundation

- Create the C89-compatible source tree and Makefile.
- Add a launcher/core arrangement if a larger runtime stack is required.
- Open a basic Workbench window using Intuition and graphics.library.
- Add clean startup and shutdown paths for every opened resource.
- Keep large network and JSON buffers out of the process stack by using `AllocMem(MEMF_PUBLIC)`.

## Phase 2: Configuration

- Add `Tankerkoenig.conf` in the program directory.
- Support these initial settings:

```ini
apikey=
location=Berlin
latitude=52.5200
longitude=13.4050
radius=10
fuel=e10
sort=price
open_only=0
update_minutes=10
```

- Never compile an API key into the application.
- Never write the API key to logs or status messages.
- Validate radius, fuel type, sorting mode and update interval.

## Phase 3: HTTPS Client

- Open and validate `bsdsocket.library` and `amitls13.library`.
- Implement DNS lookup, TCP connection, TLS with SNI, bounded timeouts and cancellation.
- Handle partial writes and reads correctly.
- Parse the HTTP status line and headers.
- Support `Content-Length`, connection-close bodies and bounded redirects.
- Close TLS, sockets and libraries in a defined and crash-safe order.
- Display a clear message when AmiTLS13 is unavailable.

## Phase 4: Bounded JSON Parser

- Implement a small C89 JSON parser for objects, arrays, strings, numbers, booleans and null.
- Decode JSON escapes and convert UTF-8 display text to Amiga Latin-1 where possible.
- Skip unknown fields safely.
- Enforce response, nesting, string and result-count limits.
- Add host-side fixtures for valid, incomplete and malformed responses.

## Phase 5: Open-Meteo Location Search

- URL-encode place names and postal codes.
- Query the Open-Meteo geocoding API over HTTPS.
- Extract name, administrative region, country, latitude and longitude.
- Show up to four selectable results.
- Save the selected location and coordinates in the configuration.

## Phase 6: Tankerkoenig API

- Query `list.php` using the selected coordinates, radius, fuel type and sorting mode.
- Always inspect the API `ok` flag before processing results.
- Parse station UUID, name, brand, address, distance, opening state and E5/E10/diesel prices.
- Handle missing fuel types and closed stations without invalid numeric output.
- Keep the personal API key out of diagnostics.

## Phase 7: Main User Interface

- Add location input, Search and Update controls.
- Add fuel selection for E5, E10, diesel and all fuels.
- Add radius, sorting and open-only controls.
- Add a scrollable station list showing price, name, distance and opening state.
- Add menus for Project, Settings and Info.
- Keep all drawing inside window borders at every supported size and depth.
- Use Workbench pens rather than fixed RGB colors.

## Phase 8: Settings and Persistence

- Add settings windows for API key, location, radius, fuel type and update interval.
- Save main and settings window positions and sizes.
- Fall back to position `0,0` if a saved window cannot be opened.
- Cache the last successful station response and mark cached prices clearly when refresh fails.

## Phase 9: Errors, Attribution and API Rules

- Provide explicit messages for missing stack, missing AmiTLS13, missing/invalid key, DNS failure, timeout, HTTP failure, invalid JSON, rate limiting and empty results.
- Credit Tankerkoenig and MTS-K in the Info window.
- Include the required CC BY 4.0 attribution.
- Do not issue automatic mass requests.
- Respect the free API radius and request-frequency limits.
- Apply filters only when explicitly selected by the user.

## Phase 10: Regression and Release Tests

- Test on AmigaOS 1.3 and AmigaOS 3.2.
- Test on a 68000 and at low Workbench color depth.
- Test small memory and stack environments.
- Test valid, missing and rejected API keys.
- Test unavailable TCP/IP stack and unavailable AmiTLS13.
- Test DNS errors, timeout, disconnect, HTTP errors, malformed JSON and no stations in range.
- Test repeated updates and clean shutdown after successful and failed requests.
- Build a warning-clean release and document installation, configuration, API-key registration and attribution.

## Planned Milestones

1. Window and configuration prototype.
2. Verified HTTPS and JSON test client.
3. Working Open-Meteo location selection.
4. Working Tankerkoenig station list.
5. Complete settings, cache and error handling.
6. AmigaOS 1.3/3.2 release candidate.
7. Version 1.0 release.
