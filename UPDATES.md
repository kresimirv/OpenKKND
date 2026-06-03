# UPDATES.md — OpenKKND Porting Progress

## Goal
Port OpenKKND to 32-bit Linux, fix all crashes so the game runs (main menu → gameplay).

## Done

### Build System
- `-m32` compiler/linker flags for 32-bit GCC build.
- SDL2 2.32.10 built from source as 32-bit shared library at `/tmp/SDL2-install/`.
- ASan only in Debug builds; Release builds are clean (no ASan).
- Game data path changed from CWD-relative to exe-relative (`game_data_installation_dir`).

### Crash Fixes — x87 FPU (SIGFPE)
Root cause: game code leaves x87 FPU with stale values on stack AND unmasked exceptions. SDL's `filds`/`fstps` triggers SIGFPE.
- Wrapped `SDL_RenderClear` / `SDL_WarpMouseInWindow` with `fnstcw` + `fnclex` + `fldcw` to mask exceptions + clear pending before SDL calls.

### Crash Fixes — Use-After-Free / Mismatched Allocators
- `GAME_ParseCommandLine()`: fixed use-after-scope (`Development.cpp:144`).
- `render_string_list_free`, `sprite_list_free`, `stru2_list_free`, `stru37_stru38_list_free`, `render_cleanup`, `render_on_quit`: changed `free` → `delete[]`.

### Crash Fixes — Sentinel Overflow (Global-Buffer-Overflow)
Sentinel trick: two adjacent pointer globals used as `next`/`prev` of implicit sentinel. ASan redzones between globals break adjacency. Fixed all known instances:

| Sentinel | File(s) | Fix |
|---|---|---|
| `bitmap_list` / `sprite_list` / `stru38_list` / `stru37_list` | (previous session) | Named struct with explicit `next`/`prev` |
| `_479D48_stru7` | `Pathfind.cpp` | 1-element → array of 20 |
| `__47CFC4_mobd_lookup_speeds` | `kknd.cpp:14248` | `[256]` → `[257]` |
| `player_sprite_color_by_player_side` | `kknd.cpp:14305` | sentinel loop → counted loop |
| `sidebar_sentinel` / `sidebar_button_sentinel` / `production_option_sentinel` | `Sidebar.cpp/.h` | Struct with `next`/`prev` |
| `oilspot_sentinel` | `kknd.cpp`, `SaveLoad.cpp`, `EntityFactory.cpp`, `Vehicles.cpp` | Struct with `next`/`prev` |
| `stru13construct_sentinel` | `kknd.cpp`, `Cursor.cpp` | Struct with `next`/`prev` |
| `stru31_sentinel` | `stru31.cpp/.h`, `Aircraft.cpp`, `Infantry.cpp`, `Towers.cpp` | Struct with `next`/`prev` |
| `stru11unit_sentinel` | `kknd.cpp` | Struct with `next`/`prev` |
| `entity_default_stru60_ptr` | `_unsorted_data.cpp/.h`, `kknd.cpp`, `EntityFactory.cpp` | `void*` → struct with `next`/`prev` |

### Other Bug Fixes
- `_47CBC0_fow` loop: `sizeof(array)` → `sizeof(array)/sizeof(array[0])` (`Map.cpp:372`).
- `stru26_array_initialize()`: sentinel address comparison → counted for loop (`kknd.cpp:4826`).
- `_47CA18_sidebar_production_buttons[5]`: sentinel loop → counted for loop (`Sidebar.cpp:1594`).
- `sound_pans`: increased from `[17]` to `[33]` (`Sound.cpp:67,153`), matching 0–32 pan offset range.
- Uppercased resource filenames in `Level.cpp`, `Game.cpp`, `Video.cpp`, `_unsorted_data.cpp` (level names, wavs, vbcs, kknd.sve).

## Current Status
- **ASan Debug build**: game runs through main menu → campaign start → gameplay without ASan errors.
- **Release build (no ASan)**: builds clean, runs 15+ seconds on X display without crash.
- Heap corruption ("malloc(): unaligned tcache chunk detected" at `malloc(0xCC)`) observed pre-fixes. May no longer occur after all sentinel overflows fixed — needs verification on real hardware.

## Remaining Concerns
- No display for full gameplay test in this environment.
- Heap corruption at `malloc(0xCC)` not yet verified as fixed on non-ASan real-hardware run.
- Low-risk printf-format warnings remain (`%d` → `size_t` args).
