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

### Optimization Fix
- `-fno-strict-aliasing` added to GCC flags (`CMakeLists.txt`). Original MSVC code uses `*(_DWORD *)ptr` type-punning throughout, which GCC's strict aliasing (`-O2`/`-O3`) breaks. Without this flag, game window was gray (no main menu rendered).

### Sound Compacted — DirectSound→SDL2 Migration & Cleanup
- Replaced `<dsound.h>` / `<process.h>` with `DirectSoundSdl2.h`, removing DirectSound dependency.
- Added `Sound::~Sound()` destructor to release `pdsb` buffer properly.
- `sound_volumes[]` / `sound_pans[]` increased from `[17]` → `[33]` to match 0–32 range (was off-by-one causing silent pans/volumes at index 32).
- `sound_initialize()`: pre-opens audio device pool via `pds->InitDevices()`.
- `LVL_LoadSlv()`: added null check for `sound_structure_1` before dereference; `_47C4E0_sounds` pointer is now validated.
- `sound_play()`: fixed `memcpy` arg order (`v29, script` → `script, v29`); moved `SetPan`/`SetVolume` to `sound_buffer_3` (was `sound->pdsb`) before `Unlock()`.
- `sound_play_threaded()`: path separator `\\` → `/`, uses `game_data_installation_dir` instead of `OsGetCurrentDirectory()`.
- `_439C10_sound_thread()` / `sound_cleanup()`: `Sleep()` → `SDL_Delay()` for portability.
- `_43A370_process_sound()`:
  - **Fixed volume transition**: `sound_pans[v10]` → `sound_volumes[v10]` (was using pan array for volume value).
  - Added bounds clamping (`v10 >= 33 → 32`, `< 0 → 0`) for volume/pan index.
  - Changed infinite `while(1)` loops → bounded `while(counter < 32)` to prevent runaway.
- Added debug `fprintf` logging throughout for diagnostics.

### Other Bug Fixes
- `_47CBC0_fow` loop: `sizeof(array)` → `sizeof(array)/sizeof(array[0])` (`Map.cpp:372`).
- `stru26_array_initialize()`: sentinel address comparison → counted for loop (`kknd.cpp:4826`).
- `_47CA18_sidebar_production_buttons[5]`: sentinel loop → counted for loop (`Sidebar.cpp:1594`).
- `sound_pans`: increased from `[17]` to `[33]` (`Sound.cpp:67,153`), matching 0–32 pan offset range.
- Uppercased resource filenames in `Level.cpp`, `Game.cpp`, `Video.cpp`, `_unsorted_data.cpp` (level names, wavs, vbcs, kknd.sve).

### In-Game Menu — Game Pause & Sound Isolation
- `DirectSoundSdl2.h` `PauseAll()`/`ResumeAll()`: skip stream (BGM) device; only pause SFX devices.
- `sound_pause_all()` / `sound_resume_all()` (`Sound.cpp`): set/clear `sound_suspended` flag.
- `GameMenu.cpp`: `sound_pause_all()` called at LABEL_26 (dialog opens), `sound_resume_all()` at LABEL_51 (dialog closes) and in Resume button handler.
- `Entity.cpp`: `entity_move()` returns early when `single_player_game && is_async_execution_supported` (menu dialog open).
- `Cursor.cpp`: `cursor_group_orders()` returns early when `single_player_game && is_async_execution_supported`.
- `Game.cpp`: `boxd_40EA20_gameloop_update()` and `_44C4B0_mess_with_turrets()` skipped when `single_player_game && is_async_execution_supported`.
- All changes guarded with `single_player_game` — multiplayer behavior is completely unaffected.

### AI Fix — Enemy Units Now Attack Without a Base
- `EnemyAI.cpp`: Generic AI initializes `_278_x_offset = -1` (no base). When `== -1`, the entire attacker marshalling and attack decision code was skipped via `goto LABEL_272` — enemies never attacked.
- Fix (formation creation): Inside the no-base block, before the `goto`, added code that forms non-scout attackers from `attacker_list_48` into a formation and inserts it into `list_11C`. The existing `list_11C` attack loop then calls `stru24_40B020()` to find the nearest enemy and issue `EVT_CMD_ENTITY_ATTACK`.
- **Second root cause — `_2A4_player_side` uninitialized**: `_2A4_player_side` is normally set at lines 366-368 when an AI building is created. For no-base AI, no building is ever built, so the field stays `0` (from `memset` at line 3172). When the condition at line 323 checks `stats->player_side != v4->_2A4_player_side`, a Survivor player unit (side `0`) compared against `0` yields `false`, routing the unit into the `else` allegiance branch (line 347) instead of adding it to `enemy_list_108`. That branch marks the unit as allied and removes it from the enemy list, leaving the list empty — the attack block at line 1581 is gated on `enemy_list_108` not being empty, so the entire formation/attack path was skipped despite the formation creation fix.
- Fix (`_2A4_player_side`): Initialize `v4->_2A4_player_side = (PLAYER_SIDE)v75;` alongside `_2A0_player_side` at line 3175 in the generic AI init loop, so no-base AI correctly identifies enemy units of different sides instead of routing them into the ally code path.

### Music Volume — Faster Response
- `Sound.cpp` (`_439C10_sound_thread`): Reduced BGM streaming buffer from `3 * nAvgBytesPerSec` to `nAvgBytesPerSec / 2` (0.5 seconds). Previously, old-volume audio data in the 3-second SDL queue had to drain before the new volume was heard — causing a multi-second delay. The smaller buffer drains 6x faster, making volume changes audible within ~600ms.
- `Sound.cpp`: After `CreateSoundBuffer`, BGM buffer's `device_id` is forced to `pds->m_stream_device` regardless of buffer size. This prevents the `>= 110000` streaming threshold from misclassifying the smaller buffer as SFX, which would assign it to a device in the SFX pool — causing BGM to get paused by `PauseAll()` when the in-game menu opens.

## Current Status
- **ASan Debug build**: game runs through main menu → campaign start → gameplay without ASan errors.
- **Release build (`-O3 -DNDEBUG -fno-strict-aliasing`)**: builds clean, runs with full rendering and sound.
- Heap corruption ("malloc(): unaligned tcache chunk detected" at `malloc(0xCC)`) observed pre-fixes. May no longer occur after all sentinel overflows fixed — needs verification on real hardware.

## Remaining Concerns
- No display/audio for full gameplay test in this environment.
- Heap corruption at `malloc(0xCC)` not yet verified as fixed on non-ASan real-hardware run.
- `SetCurrentAnimFrame(256): index out of bounds` warning during gameplay (minor, doesn't crash).
- Low-risk printf-format warnings remain (`%d` → `size_t` args).

### In-Game Menu — Restart / Return to Main Menu Crash Fix
- Root cause: when "Restart Game" or "Return to Main Menu" was selected, `is_async_execution_supported` stayed `1` (set by the menu event loop) and `sound_suspended` stayed `true` (set by `sound_pause_all()` at menu open). Neither was reset on the restart/quit path because the normal menu-dismiss path (which resets both) was bypassed.
- `is_async_execution_supported = 1` blocked entity movement (`Entity.cpp:180`), cursor commands (`Cursor.cpp:1880`), and game tick counters (`Game.cpp:155`) in the next level — units couldn't move, no commands worked, the game appeared frozen/crashed.
- `sound_suspended = true` blocked all sound playback in the restarted mission or main menu.
- Fix (`Game.cpp:173-184`): added `is_async_execution_supported = 0; sound_resume_all();` in both the quit-to-main-menu path (before `goto LABEL_5`) and the restart path (after `on_level_finished()` returns).
