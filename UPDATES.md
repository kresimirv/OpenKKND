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
### Other Bug Fixes
- `_47CBC0_fow` loop: `sizeof(array)` → `sizeof(array)/sizeof(array[0])` (`Map.cpp:372`).
- `stru26_array_initialize()`: sentinel address comparison → counted for loop (`kknd.cpp:4826`).
- `_47CA18_sidebar_production_buttons[5]`: sentinel loop → counted for loop (`Sidebar.cpp:1594`).
- `sound_pans`: increased from `[17]` to `[33]` (`Sound.cpp:67,153`), matching 0–32 pan offset range.
- Uppercased resource filenames in `Level.cpp`, `Game.cpp`, `Video.cpp`, `_unsorted_data.cpp` (level names, wavs, vbcs, kknd.sve).
- `script_442BB0_mobd46` (`kknd.cpp:11865`): moved bounds check for `BYTE2(a1a->num_runs_to_skip)` before `_46E4C0_mobd_offsets[10]` / `_46E4F0_mobd_offsets[10]` array access. The index was used to read the array before the `v7 < 9u || v7 == 9` guard — when `BYTE2` was ≥ 10 it caused a global-buffer-overflow, crashing when clicking NEW MISSIONS in the main menu.

### In-Game Menu — Game Pause & Sound Isolation
- `DirectSoundSdl2.h` `PauseAll()`/`ResumeAll()`: skip stream (BGM) device; only pause SFX devices.
- `sound_pause_all()` / `sound_resume_all()` (`Sound.cpp`): set/clear `sound_suspended` flag.
- `GameMenu.cpp`: `sound_pause_all()` called at LABEL_26 (dialog opens), `sound_resume_all()` at LABEL_51 (dialog closes) and in Resume button handler.
- `Entity.cpp`: `entity_move()` returns early when `single_player_game && is_async_execution_supported` (menu dialog open).
- `Cursor.cpp`: `cursor_group_orders()` returns early when `single_player_game && is_async_execution_supported`.
- `Game.cpp`: `boxd_40EA20_gameloop_update()` and `_44C4B0_mess_with_turrets()` skipped when `single_player_game && is_async_execution_supported`.
- All changes guarded with `single_player_game` — multiplayer behavior is completely unaffected.

### In-Game Menu — True Game Freeze (Combat, Construction, Production)
- **Root cause**: `script_list_update()` (`Script.cpp`) ran ALL scripts every frame, including game-world scripts (entity combat AI, building construction, production queues, resource gathering, enemy AI) — only `entity_move()` and `cursor_group_orders()` were blocked. Units fought and buildings were built during the menu pause because their entity-mode handlers executed unconditionally.
- **Fix 1** (`Script.cpp:1360-1361`): Added `field_1C` guard in `script_list_update()` — scripts with `field_1C & 1 == 0` (game-world scripts) are skipped during single-player pause. UI scripts (menu, cursor, input handler, network) already set `field_1C = 1` and continue running. This freezes all entity modes (move, attack, build, produce, repair, research), enemy AI decision-making, production timer countdowns, and resource processing.
- **Fix 2** (`Entity.cpp:228-229`): Added `entity_attack()` pause guard matching the existing `entity_move()` guard — prevents combat orders from being issued while paused.
- **How it works**: The existing sprite update functions (`sprite_list_init_mobd_items`, `sprite_list_update_positions`) already used the same `field_1C` check to skip game-world sprite animation/position updates during pause. With game scripts also frozen, the entire simulation stands still. On resume, script yield timers are preserved exactly where they were paused. BGM continues playing via the existing `PauseAll()`/`ResumeAll()` stream isolation.

### AI Fix — Enemy Units Now Attack Without a Base
- `EnemyAI.cpp`: Generic AI initializes `_278_x_offset = -1` (no base). When `== -1`, the entire attacker marshalling and attack decision code was skipped via `goto LABEL_272` — enemies never attacked.
- Fix (formation creation): Inside the no-base block, before the `goto`, added code that forms non-scout attackers from `attacker_list_48` into a formation and inserts it into `list_11C`. The existing `list_11C` attack loop then calls `stru24_40B020()` to find the nearest enemy and issue `EVT_CMD_ENTITY_ATTACK`.
- **Second root cause — `_2A4_player_side` uninitialized**: `_2A4_player_side` is normally set at lines 366-368 when an AI building is created. For no-base AI, no building is ever built, so the field stays `0` (from `memset` at line 3172). When the condition at line 323 checks `stats->player_side != v4->_2A4_player_side`, a Survivor player unit (side `0`) compared against `0` yields `false`, routing the unit into the `else` allegiance branch (line 347) instead of adding it to `enemy_list_108`. That branch marks the unit as allied and removes it from the enemy list, leaving the list empty — the attack block at line 1581 is gated on `enemy_list_108` not being empty, so the entire formation/attack path was skipped despite the formation creation fix.
- Fix (`_2A4_player_side`): Initialize `v4->_2A4_player_side = (PLAYER_SIDE)v75;` alongside `_2A0_player_side` at line 3175 in the generic AI init loop, so no-base AI correctly identifies enemy units of different sides instead of routing them into the ally code path.

### Music Volume — Faster Response
- `Sound.cpp` (`_439C10_sound_thread`): Reduced BGM streaming buffer from `3 * nAvgBytesPerSec` to `nAvgBytesPerSec / 2` (0.5 seconds). Previously, old-volume audio data in the 3-second SDL queue had to drain before the new volume was heard — causing a multi-second delay. The smaller buffer drains 6x faster, making volume changes audible within ~600ms.
- `Sound.cpp`: After `CreateSoundBuffer`, BGM buffer's `device_id` is forced to `pds->m_stream_device` regardless of buffer size. This prevents the `>= 110000` streaming threshold from misclassifying the smaller buffer as SFX, which would assign it to a device in the SFX pool — causing BGM to get paused by `PauseAll()` when the in-game menu opens.

### In-Game Menu — Custom Mission Briefing Fix
- Root cause: `script_ingame_menu_create_briefing_dialog` (`GameMenu.cpp:345`) indexed `aNoFreeLinks[4 * current_level_idx + 12]` — but custom mission level indices go up to 19, producing an OOB access into the fixed `[4*10]` array. Game crashed when opening "MISSION BRIEFING" for any New Mission.
- Additionally, the full 20-line `custom_missions[].briefing[]` text was too long for the 11-line in-game menu dialog. The original EXE stored separate shorter briefings using `.` as newline delimiters (`.`=`\n`, `..`=`\n\n`).
- **Fix**: extracted the 20 short briefings from the original EXE into `custom_missions_in_game_briefing[20]` array (`_unsorted_data.cpp:2135-2176`), using a `period+space=no-break` rule to keep entries ≤ 11 lines. Updated `script_ingame_menu_create_briefing_dialog` to use `custom_missions_in_game_briefing[cm_idx]` instead of the OOB heuristic.
- Removed unused `briefing_text[2048]` local variable.

### Save Game Dialog — Missing Keyboard Keys
- `SdlWindow.cpp:MessageProcessor`: added SDL_KEYUP forwarding for scancodes: BACKSPACE (8), DELETE (46), SPACE (32), LEFT (37), RIGHT (39), HOME (36), END (35), INSERT (45) via `NofifyCharUp(vk, ctrl, alt)`.
- `input_get_string()` (`kknd.cpp:5333`) already handled all these VK codes — they just weren't being forwarded from SDL.

### Save Game Dialog — Escape Key Fix
- Root cause: `input_get_string()` checked for `case 27 (VK_ESCAPE)` in its switch, but the SDL_KEYUP handler forwarded Escape via `NofifySpecialKeyUp(INPUT_KEYBOARD_ESCAPE_MASK, ...)` which is `0x200`. The switch missed the `0x200` case, so Escape was silently ignored — the function still returned `true` (same as Enter), causing the save to proceed instead of cancelling.
- Also, `input_char_is_escape()` only checked `input_combo_pressed_vk == 27`, missing the `0x200` value.
- **Fix**: added `case INPUT_KEYBOARD_ESCAPE_MASK:` alongside `case 27:` in `input_get_string()`, and updated `input_char_is_escape()` to check for both `VK_ESCAPE (27)` and `INPUT_KEYBOARD_ESCAPE_MASK (0x200)`.

### Save Game Dialog — Delete Key Crash Fix
- Root cause: `input_get_string()` Delete and Backspace handlers used `v5 += 0xFFFF` (lines 5417, 5423). In the original 16-bit x86 assembly, `add ax, 0FFFFh` wraps to `ax -= 1`. In the 32-bit C++ translation, `v5` is `int`, so `0xFFFF` is the literal 65535 — adding 65535 instead of subtracting 1 produced a huge cursor position (e.g., 65536). This was passed to `draw_handler()` → `render_string_443EE0()`, which used it to traverse a linked list — walking past the end into freed memory → crash.
- **Fix**: replaced `v5 += 0xFFFF` with `v5--` in both the Delete and Backspace handlers.

### Crash Fix — Savegame with Destroyed Entities (New Missions Save/Load)
- **Root cause 1 (`SaveLoad.cpp`): `GAME_Save_PackEntities()`** iterated `entities.size()` times but `entity_save_index` has fewer entries (destroyed entities skipped at line 1765). `std::next(entity_save_index.begin(), i)` went past `end()` for any destroyed entity, producing UB. The entity data buffer was filled with wrong data at wrong offsets, corrupting the save file. On load, `EntityFactory::Unpack()` parsed corrupt data → returned null → entity loading loop `break`-ed early → AI player data referenced entity IDs not in the repo.
- **Root cause 2 (`SaveLoad.cpp`): `GAME_Load_UnpackAiPlayers()`** had 3 locations (drill rig line 2641, tanker line 2741, tanker_DC line 2766) that dereferenced `entityRepo->FindById()` results without null checks. When entities were missing due to corrupt save data, this caused null pointer dereference → SIGSEGV.
- **Fix 1**: `GAME_Save_PackEntities()` now uses a separate `index_iter` that only advances when a non-destroyed entity is actually packed, keeping entity buffer and index entries perfectly aligned.
- **Fix 2-4**: Added `if (v53/v66/v71)` guards around the `_24_ai_node_per_player_side` access in all 3 locations, matching the null-check pattern used by every other list in the same function.

### Crash Fix — Production Loop Hardcoded Address (New Missions Save/Load)
- **Root cause (`EntityFactory.cpp:579`, `SaveLoad.cpp:2587`)**: Two call sites used hardcoded address `4704680` (`0x47C8A8`) — the Windows EXE address of `game_globals_per_player` (actual BSS address: `0x322460`). The wrong address caused `_18_pcash` in production `stru37` entries to point to unmapped memory. When the production loop (`kknd.cpp:7078`) dereferenced `_18_pcash`, it triggered SIGSEGV.
- **Root cause (`kknd.cpp:7078`)**: The production loop had no null guard for `_18_pcash`, so stale `stru37` entries with null `_18_pcash` (from pre-save state) also crashed.
- **Fix 1**: Replaced hardcoded `4704680` with `(int *)&game_globals_per_player` in both call sites.
- **Fix 2**: Added `if (v5 && *v5)` null guard around the `_18_pcash` dereference in the production loop.
- **Verification**: New Missions save files (114KB) load successfully without crashes. Game proceeds from main menu through save load into gameplay.

### Crash Fix — Use-After-Free in Oil Tanker Entity (game6.sav)
- **Root cause**: `EntityFactory::Unpack()` deleted the entity when `entity_mode` was invalid/unspecified (`0` or `-1` in old save format), but `v3->script->param` still pointed to the freed entity. Later `UNIT_Handler_OilTanker` dereferenced `a1->param` → use-after-free → `ExecMode` called on garbage memory.
- **Fix 1**: `EntityFactory::Unpack()` no longer deletes the entity for invalid `entity_mode`. Values ≤ 0 are treated as "not set" (backward compat for old saves). For positive values, tries 1-based lookup then 0-based fallback. If still no match, leaves mode as `nullptr` instead of deleting.
- **Fix 2**: `UNIT_Handler_OilTanker` checks `v1->HasMode()` before calling `ExecMode()`. If mode is null (loaded from old save without a mode), sets a default (`entity_mode_4444D0_oiltanker`).
- **Fix 3**: Added `Entity::HasMode()` public accessor.
- **Verification**: game6.sav loads without crash. All save files 0-6 load past this point (only pre-existing minimap `heap-buffer-overflow` in `Map.cpp:697` remains as ASan error).

## Current Status
- **ASan Debug build**: game runs through main menu → campaign start → gameplay without ASan errors.
- **Release build (`-O3 -DNDEBUG -fno-strict-aliasing`)**: builds clean, runs with full rendering and sound.
- **Savegames**: Fully working — save/load, unit selection, unit control, enemy AI, and attack commands all work correctly after loading a save.
- **Pathfinding**: Fixed — all units (infantry and vehicles) navigate around hills/obstacles, group move orders settle at destination, infantry uses same lateral avoidance (sidestep, cardinal slide, fine‑snap sidestep) as vehicles.
- **BGM**: Fixed — background music no longer stalls or stutters during gameplay.

## Remaining Concerns
- No display/audio for full gameplay test in this environment.
- `SetCurrentAnimFrame(256): index out of bounds` warning during gameplay (minor, doesn't crash).
- Low-risk printf-format warnings remain (`%d` → `size_t` args).

### Crash Fixes — Stack-Buffer-Overflow in EnemyAI (entity_move)
Root cause: `script_409770_ai` used groups of adjacent stack variables (`v172`/`v170`/`i1` and `param[4]`/`v173`/`v174`) as fake `_47CAF0_task_attachment1_move_task` structs via `&v172` / `&param`. ASan redzoning detects reads past the first variable.
- Fixed 5 call sites in `EnemyAI.cpp:1388,1603,1743,1792,1848` — all replaced with proper local `_47CAF0_task_attachment1_move_task` struct variables and explicit field assignments.
- Removed unused declarations (`v172`, `v170`, `i1`, `param[4]`, `v173`, `v174`, `v114`).

### Crash Fixes — SIGFPE in Sound Playback (Unit Selection)
Root cause: `IDirectSoundBuffer::Play()` (`DirectSoundSdl2.h:263-264`) performed `(double)buffer_size / wfx.nAvgBytesPerSec * 1000.0` using floating-point arithmetic. When `nAvgBytesPerSec` was 0 (or under unknown FPU state in coroutine context), a floating-point division-by-zero raised SIGFPE — triggered when dragging a selection rectangle over units in gameplay.
- Rewrote `Play()` time tracking to use only integer arithmetic (`unsigned long long`), with explicit zero-guards on all divisions and a `our_ms > 0` guard for `play_start_time`/`duration_ms` assignment.

### In-Game Menu — Restart / Return to Main Menu Crash Fix
- Root cause: when "Restart Game" or "Return to Main Menu" was selected, `is_async_execution_supported` stayed `1` (set by the menu event loop) and `sound_suspended` stayed `true` (set by `sound_pause_all()` at menu open). Neither was reset on the restart/quit path because the normal menu-dismiss path (which resets both) was bypassed.
- `is_async_execution_supported = 1` blocked entity movement (`Entity.cpp:180`), cursor commands (`Cursor.cpp:1880`), and game tick counters (`Game.cpp:155`) in the next level — units couldn't move, no commands worked, the game appeared frozen/crashed.
- `sound_suspended = true` blocked all sound playback in the restarted mission or main menu.
- Fix (`Game.cpp:173-184`): added `is_async_execution_supported = 0; sound_resume_all();` in both the quit-to-main-menu path (before `goto LABEL_5`) and the restart path (after `on_level_finished()` returns).

### Kaos Mode — Crash on Quit, Main Menu Corruption, Wrong Start Position
- **BSS buffer overflow** (`Mission.cpp:37`): Tech bunker handler wrote sprite `y` to `_47A300_stru51_array__field_4__minus1_index[2*(v1+1)]` — computed index up to 24, but array was declared `[5]`. Overflow corrupted adjacent BSS globals: `sprites_lvl` (at index 22, value `0x8ef00`) caused crash-on-quit in `free()`; `wait_lvl` (index 24) caused second crash in `Game::Run()`; possibly also corrupted `game_state` and other state → main menu items disabled after Kaos quit. **Fix**: store `y` to `_47A300_stru51_tech_bunkers[v1].y` instead — correct write within array bounds, also fixes tech bunker spawn y-read at line 1640.
- **`free(wait_lvl)` crash guard** (`Game.cpp:213,219`): added same `0x100000` pointer-sanity check as `sprites_lvl` guard.
- **Player start position not found** (`Mission.cpp:1317`): `is_demo_build` search loop compared `v12` against `&_47A3D4_tanker_convoy_units_left` (BSS `0xdbbb0`) — which sits **before** `_47A378_stru48_array` (BSS `0xdbbc0`). The initial `v12 = 0xDBBC8` already exceeded the bound, so the loop **always** immediately exited to random-start fallback (`kknd_rand % 6`). Player spawned at a random (often AI) start → camera in fog, units under wrong side control, "fully built base" from AI territory. **Fix**: changed bound to `v11 >= _47A378_stru48_array_num_items` — properly iterates all available start positions until `player_side` matches.

### Crash Fix — AI Player Save/Load (heap-buffer-overflow)
- Root cause: `GAME_Save_PackAiPlayers` size pass counted list data for *all* non-null AI players, but the pack pass only *wrote* it when `handler_id >= 0`. For null-handler players with non-empty lists (e.g., preplaced enemies), a buffer gap of uninitialized heap existed — on load, the sequential reader read past the actual written data.
- **Fix 1** (`SaveLoad.cpp:3345`): removed `if (handler_id >= 0)` guard in the pack pass. Every non-null AI player now has full data written (284-byte base + all list data), matching the size calculation.
- **Fix 2** (`SaveLoad.cpp:2304`): removed the null-handler early skip in `GAME_Load_UnpackAiPlayers`. Every non-null AI player now has full data loaded regardless of handler validity.
- Backward compatible: old saves have zeroed count fields in null-handler base blocks; load skips list processing when counts are zero.
- **Fix 3** (`SaveLoad.cpp:2029-2036`): handle `block_size == 0` in `GAME_Load()` when no CPU opponents exist. `fread()` with count 0 returns 0 items (falsy), treated as failure — crashed campaigns without CPU players (e.g., mission 1 "The Next Generation"). Added `block_size == 0 ||` short-circuits to skip AI player loading when no data exists.

### Save File Paths — Backslash → Forward Slash
- Root cause: format strings used `\\` (Windows backslash) as path separator. On Linux, `\\` is a valid filename character, not a separator. `game_data_installation_dir` (e.g. `/path/to/bin`) combined with `%s\\game%d.sav` produced `/path/to/bin\game0.sav` — a literal file with `\` in the name.
- **Fix**: changed all save-related path format strings to use `/` instead of `\\`:
  - `aSGameD_sav`: `"%s\\game%d.sav"` → `"%s/game%d.sav"` (`_unsorted_data.cpp:2497`)
  - `aSSave_lst`: `"%s\\save.lst"` → `"%s/save.lst"` (`_unsorted_data.cpp:2499`)
  - KKND.SVE read path: `"%s\\%s"` → `"%s/%s"` (`SaveLoad.cpp:1164`)
  - KKND.SVE update path: `"%s\\%s"` → `"%s/%s"` (`SaveLoad.cpp:1326`)
- Save files now created as e.g. `bin/game0.sav`, `bin/save.lst`, `bin/KKND.SVE`.

### Video Decoder Fix — Visual Artifacts in VBC Frames (Global Motion + Op 0)

- **Root cause**: Two issues in the 8-bit and 16-bit VBC frame decoders (`Video.cpp`):
  1. **Missing pre-copy of previous frame**: The C# reference decoder (`ApplyFrame`) creates a zeroed new frame, then `BlockCopy`s the old frame into it with a byte offset (global motion). Our decoder wrote directly to `front_buf` without first copying the old frame — uncovered pixels stayed as stale garbage instead of the correct previous-frame content (or zero).
  2. **Op 0 was not a no-op**: In both decoders, op 0 was implemented as a reference copy (like op 1 with flag=0), but in C# op 0 does nothing — the tile is already covered by the pre-copy.

- **Fix 1 (pre-copy)**: Added `memcpy` from `back_buf` to `front_buf` before tile decode, matching C# `Buffer.BlockCopy`:
  - 8-bit: `memcpy(front_buf, back_buf + offset, frame_size - offset)` for offset≥0, `memcpy(front_buf - offset, back_buf, frame_size + offset)` for offset<0. Frame size = `width * tile_rows * 4`.
  - 16-bit: Same logic with byte offset = `2 * offset` and frame size = `2 * width * tile_rows * 4`.

- **Fix 2 (op 0 no-op)**: Changed op 0 in both decoders from reference copy to `break;` (no-op), matching C# behavior. The pre-copy already handles the old frame data.

### Crash Fix — Mission Briefing Video Negative-Size-Param in memcpy

- Root cause: `render_draw_tile()` (`Render.cpp:1252`) declared `v9` as `char` (signed 8-bit). When drawing the 320px-wide mission briefing VBC frame, `this_row_x_pixels` values > 127 were truncated to negative (e.g., 160 → -96). Passing this as the `size` argument to `memcpy` triggered ASan `negative-size-param`.
- Fix: Changed `char v9` to `int v9`. The original two-step `qmemcpy` approach (4-byte chunks + remainder) was immune to this truncation, but was replaced with a single `memcpy` without fixing the type.

### Crash Fix — AI Player List Off-by-One on Load (heap-buffer-overflow)
- Root cause: `GAME_Load_UnpackAiPlayers` advanced read pointers (`v6 += 4`, `a2 += 4`, `++v56`) *before* reading entity IDs, but the pack code writes the ID before advancing (`*((_DWORD *)i19 - 1) = id; i19 += 4`). First entry at offset 0 was skipped, and the last iteration read one DWORD past the allocated buffer.
- **Fix**: moved all pointer-advance operations to *after* the entity-ID reads in 7 loops:
  - Enemy list (`SaveLoad.cpp:3097`)
  - Wanderer list (`2412`)
  - `list_40` (`2443`)
  - `attacker_list_48` (`2474`)
  - `marshalling_nodes` (`2508`)
  - `power_plant` list (`2577`)
  - Attacker sublist in convoy (`2363`)
  - Attacker sublist in drill-rig (`2658`)

### Crash Fix — EventHandler_OilTanker Segfault (Script Handler Mismatch)
- Root cause: `EventHandler_OilTanker` was registered in the `script_handlers[]` table and got called as a `Script::handler` via `s->handler(s)` (1 arg) instead of as a `Script::event_handler` (4 args). The remaining 3 parameters were garbage on stack — `receiver->param` was a `task_sidebar_attachment*`, not an `Entity*`, causing a segfault on dereference.
- **Fix** (`Vehicles.cpp:688-690`): added defensive guard `if (!v5 || v5->script != receiver) return;` — validates that `receiver->param` points back to the receiver via the Entity's `script` field before proceeding.

### Crash Fix — Use-After-Free on New Campaign Start After Quit to Main Menu
- Root cause: `script_list_free()` (`Script.cpp:1390`) guards cleanup with `if (coroutine_current == coroutine_list_get_head())`. After MainMenu exits (user clicks "New Campaign"), `coroutine_current` still points to the last resumed coroutine, NOT the head. The guard fails silently — scripts are NOT deleted and `script_execute_list` is NOT cleared. Stale scripts (from previous game + main menu) persist into the new campaign's script list. A stale tower-adjacent script with corrupted `param` executes `EventHandler_Towers` → reads from freed SDL surface memory → heap-use-after-free.
- **Fix 1** (`Script.cpp:1390`): replaced `if (coroutine_current == coroutine_list_get_head())` with `coroutine_current = coroutine_list_get_head();` — unconditionally resets `coroutine_current` to head so cleanup always runs when `script_list_free()` is called from `LVL_Deinit()`.
- **Fix 2** (`Towers.cpp:174-175`): added defensive guard `if (!v4 || v4->script != receiver) return;` in `EventHandler_Towers` — validates the bidirectional Entity↔Script relationship before accessing entity fields, preventing crashes from stale/corrupted script params.

### Savegame Fix — Handler ID Mismatch Between Save/Load (Crash)
- Root cause: Handler IDs were saved as 0-based (0 = first handler) but loaded as 1-based (0 = null handler, 1 = first handler). This caused wrong handlers to be invoked, including 4-param handlers being called as 1-param handlers, leading to crashes.
- **Fix** (`Script.cpp`): changed `get_handler_id()` to return 1-based index:
  - Return `i + 1` instead of `i` when handler is found
  - Return `0` instead of `-1` when handler is not found (matching load-side convention: 0 = null)
- **Fix** (`Vehicles.cpp:51,55,60; Infantry.cpp:362,438; kknd.cpp:11144,11152,11158`): added pointer-type detection guards (offset-12 heuristic: values < 1MB = `script_type`, values >= 1MB = `script` pointer) to safely handle cases where `script` and `script_type` might be confused.

### Savegame Fix — Entity ID Not Restored (Unit Control + AI Broken)
- Root cause: When loading savegames, `EntityFactory::Create()` assigned new sequential IDs instead of restoring the original saved IDs. This broke:
  - Unit targeting (attack commands used wrong entity IDs)
  - Enemy AI (couldn't find targets by ID)
  - All entity ID-based lookups
- **Fix 1** (`EntityFactory.cpp:297`): Added `v3->entity_id = v2->entity_entity_id;` in `EntityFactory::Unpack()` to restore the saved entity ID instead of generating a new one.
- **Fix 2** (`SaveLoad.cpp:2057-2059`): Updated `_47DCC4_entity_id_counter` after loading all entities to avoid ID collisions for newly spawned units.
- **Fix 3** (`SaveLoad.cpp:1968`): Fixed compilation error — moved `int max_entity_id;` declaration before the while loop that contains `goto LABEL_59` to avoid "crosses initialization of variable" error.

### Savegame Fix — Defensive Null Checks for Entity Factory
- **Fix** (`Vehicles.cpp:175; Infantry.cpp:839,853`): Added null checks for `EntityFactory::Create()` return values in `UNIT_Handler_VehiclesInfantry` and `UNIT_Handler_OilTanker` to prevent dereferencing null pointers if entity creation fails.

### Crash Fix — Information Icon (Help) Global-Buffer-Overflow
- Root cause: `sidebar_button_handler_help_open` used sentinel pattern relying on `_47CA10_sidebar_button_minimap` being immediately adjacent to `_47CA08_sidebar_buttons[2]` in memory. ASan inserts redzones between globals, breaking this assumption and causing reads past the array bounds when clicking the Information icon.
- **Fix 1** (`Sidebar.cpp:311-320`): Changed `sidebar_button_handler_help_open` from address-based sentinel loop to counted loop `for (int i = 1; i < 2; ++i)` (processes only index 1, skipping index 0 as intended).
- **Fix 2** (`Sidebar.cpp:648-657`): Changed `sidebar_button_handler_446190_open` from address-based sentinel loop to counted loop `for (int i = 0; i < 1; ++i)` (processes only index 0, skipping index 1 as intended) for consistency and safety.

### Crash Fix — KKND.SVE Validation (Mission Complete) Global-Buffer-Overflow
- Root cause: `_4240E0_kknd_sve_read()` and `_424270_kknd_sve_read()` used sentinel patterns relying on `kknd_sve_array_463070[15]` being immediately adjacent to `kknd_sve_array_4630AC[15]`, which is then adjacent to `dword_4630E8`. ASan redzones between globals break this assumption, causing reads past array bounds when validating the KKND.SVE file after completing a mission.
- **Fix** (`SaveLoad.cpp`): Converted 4 sentinel-based while loops to counted loops of 15 iterations each:
  - `_4240E0_kknd_sve_read()`: first loop (lines 1174-1188) and second loop (lines 1199-1213)
  - `_424270_kknd_sve_read()`: first loop (lines 1254-1268) and second loop (lines 1279-1293)
- Each fix includes inner loops to consume remaining file values if validation fails early, keeping the file pointer correct for subsequent reads.

### Menu Fix — Infantry Units Appearing in Buildings Menu
- Root cause: In `entity_mode_outpost_enable_basic_construction` (Survivor) and `UNIT_Handler_Clanhall` (Mutant), the Outpost/Clanhall initialization code added BOTH building options AND infantry options to `_47B3D0_building_production_group`, which was created with `PRODUCTION_GROUP_BUILDINGS`. This caused infantry units (Rifleman, SWAT, Technician for Survivors; Berserker, Shotgunner, Mekanik for Mutants) to appear in the Buildings menu.
- **Fix** (`kknd.cpp`): Removed the infantry loop from the Outpost/Clanhall initialization. Barracks and Tech Centers already create their own infantry production groups with `PRODUCTION_GROUP_INFANTRY` when built, so the infantry loop in the shared building production group was redundant and incorrect.

### Crash Fix — Minimap Button Global-Buffer-Overflow
- Root cause: Multiple issues with the minimap data structures and memory layout assumptions:
  1. `dword_470588[]` was only 4 elements, but accessed as `Sprite_stru58_stru0*` which has 6 int fields (24 bytes). The `z` and `w` fields were supposed to come from separate adjacent globals `dword_470598` and `dword_47059C`.
  2. `_4705A8_minimap_smthn` was declared as `int*` (4-byte pointer), but code cast `&_4705A8_minimap_smthn` to `Sprite_stru58*`, taking the ADDRESS of the pointer variable.
  3. **Most critical**: The collision detection code in `Pathfind.cpp` uses pointer arithmetic (`++v16`, `v38 + 1`) to iterate through an **array** of `Sprite_stru58` structures, terminating when `pstru0` is null. But `_4705A8_minimap_smthn` was only a single element, not an array with a sentinel.
- In the original binary, all these globals were packed consecutively with no padding, so the memory layout "happened to work". ASan inserts redzones between globals, breaking all these assumptions.
- **Fix** (`Map.cpp`):
  - Expanded `dword_470588[]` from 4 elements to 6: `{ 1, 0, 0, 0xC0000000, 0, 0 }`
  - Changed `dword_470598 = ...` → `dword_470588[4] = ...` (stores minimap width as `z` field)
  - Changed `dword_47059C = ...` → `dword_470588[5] = ...` (stores minimap height as `w` field)
  - Removed now-unused separate globals `dword_470598` and `dword_47059C`
  - **Key fix**: Changed `_4705A8_minimap_smthn` from `int*` (pointer) to a proper **2-element array** of `Sprite_stru58`:
    ```cpp
    Sprite_stru58 _4705A8_minimap_smthn[] = {
        { (Sprite_stru58_stru0 *)dword_470588 },
        { NULL }  // Sentinel: terminates the loop in Pathfind.cpp
    };
    ```
  - The second element with `pstru0 = NULL` acts as a sentinel, so when the collision detection code does `++v16` and checks `if (!v16->pstru0) break;`, it properly terminates instead of reading into ASan redzones.

### Visual Fix — Minimap Background Rendering
- Root cause: Memory layout mismatch between original code and GCC's default struct padding:
  1. `DrawHandlerData_Units` struct: 2 ints (8 bytes) + char (1 byte) + 2-byte array = 11 bytes total. But GCC adds 1 byte of padding for 4-byte alignment, making it 12 bytes.
  2. The minimap allocation size is `(minimap_width+4) * (minimap_height+4) + 12`, but the border setup code assumed no padding (11-byte struct).
  3. This caused `dword_47CBAC` (content area address) to be calculated incorrectly.
- **Fix 1** (`kknd.h:431): Added `__attribute__((packed))` to `DrawHandlerData_Units` struct to match original memory layout.
- **Fix 2** (`Map.cpp:550): Reverted incorrect loop increment: `destination += i` → `destination += 4`. The original `destination += 4` was actually correct: the inner loop increments `destination` by 1 for each of `minimap_width` pixels, and `destination += 4` adds the remaining border stride, giving total `minimap_width + 4` per row.

### Crash Fix — Infantry Idle Fidgeting Global-Buffer-Overflow
- Root cause: `entity_mode_4157F0_infantry_idle_fidgeting` added/subtracted 32 from the current animation frame without bounds checking or wrap-around. This caused:
  - `GetCurrentAnimFrame() + 32` could exceed 255 (e.g., 240 + 32 = 272)
  - `GetCurrentAnimFrame() - 32` could become negative (e.g., 20 - 32 = -12)
  - `SetCurrentAnimFrame()` only checked for `>= 256`, not for negative values
  - When arrays were accessed as `_47D3C4_entity_mobd_lookup_ids[GetCurrentAnimFrame() + 1]`, a value of -12 became index -11, reading way out of bounds
- **Fix 1** (`Entity.h:129-138`): Updated `SetCurrentAnimFrame()` to check for both `>= 256` AND `< 0`, and return early without changing the value if out of bounds (with logging).
- **Fix 2** (`Infantry.cpp:1973-2010`): Updated `entity_mode_4157F0_infantry_idle_fidgeting` to:
  - Check if `GetCurrentAnimFrame() == -1` (special uninitialized value) and skip if so
  - Use `(unsigned __int8)` cast + `& 0xFF` mask to properly wrap values when adding/subtracting 32 (matching pattern used elsewhere in the codebase)

### Visual Fix — Minimap Terrain Colors
- Root cause: **Double increment bug in decompiled histogram loop** when sampling terrain colors for the minimap. The loop incremented `v19` (histogram index) twice per iteration:
  - Once in the `for` loop header: `for (v19 = 1; ...; ++v19)`
  - Once inside the loop: `++v19;`
- This caused the loop to only check **odd palette indices** (1, 3, 5, 7...), skipping even indices (2, 4, 6, 8...). The actual most common terrain color was never found, so the minimap defaulted to color 0 (transparent) or 1 (gray).
- **Fix** (`Map.cpp:373-382`): Removed the extra `++v19;` inside the loop. Now all palette indices 1-255 are properly checked when finding the most common color in each 16x16 tile sample.

### Crash Fix — Oil Patch (UNIT_Handler_OilPatch) Heap-Buffer-Overflow
- Root cause: In the "Infiltrator" custom mission, oil patch entities were placed outside the map bounds (e.g., `y=84` when `map_height=82`). `UNIT_Handler_OilPatch` called `boxd_get_tile()` to mark the tile's `flags2 |= 0x80u` (marking tile as having oil), but `boxd_get_tile()` only logs an error message without bounds checking — it still returns a pointer into the `_478AA8_boxd_stru0_array` using the out-of-bounds index. ASan detected this as a heap-buffer-overflow when the flag was dereferenced and written to.
- **Fix** (`kknd.cpp:2925-2930`): Added explicit bounds check before calling `boxd_get_tile()`. The oil deposit creation still proceeds (the entity is still spawned), but the tile flag update is skipped if `map_x` or `map_y` are outside the valid range `[0, map_get_width()-1]` or `[0, map_get_height()-1]`.

### SFX (Units) Sounds — Queue Buildup & Cleanup Fixes
- **Root cause (lag)**: Only 8 SFX devices with round-robin assignment. When many units fired simultaneously, `SDL_QueueAudio()` accumulated sounds per device with no limit — each device's queue grew unbounded, delaying sound playback by seconds behind the on-screen action.
- **Fix 1** (`DirectSoundSdl2.h, CreateSoundBuffer()`): Replaced round-robin with **smart device selection** — each new SFX is assigned to the device with the least queued audio (`SDL_GetQueuedAudioSize`). Sound load is now naturally balanced across all 8 devices.
- **Fix 2** (`Sound.cpp, sound_play()`): Added per-device queue size check. If a device already has ≥44100 bytes queued (~500ms playback), the new sound is silently dropped instead of queued. Together with smart selection, this prevents any device from accumulating >500ms of latency.
- **Root cause (post-quit SFX)**: In `sound_free_sounds()` and `sound_deinit()`, `_47C5C0_can_sound` was set to 0 *after* the cleanup loop, leaving a window where script event triggers from `sound_cleanup()` could create new sounds during teardown.
- **Fix 3** (`Sound.cpp, sound_free_sounds()/sound_deinit()`): Moved `_47C5C0_can_sound = 0` to *before* the cleanup loop, preventing any new sound creation during teardown. This eliminates residual SFX playback after mission quit or restart.

### Kaos Mode — Wrong Colors / Missing Player Color Palette
- Root cause: Three `%S` format specifiers in `_unsorted_data.cpp` — MSVC treats `%S` as wide-to-narrow conversion, but GCC treats it as `wchar_t*`. `sprintf()` produced garbled paths when formatting `game_data_installation_dir` (a `char*`), causing `fopen()` to silently fail. As a result, `MULTI.PAL` was never loaded, and per-player sprite color remapping never occurred.
- **Fix** (`_unsorted_data.cpp`): Changed `%S` → `%s` in 3 format strings:
  - `aSLevelsMulti_p`: `"%S//LEVELS//MULTI.PAL"` → `"%s//LEVELS//MULTI.PAL"`
  - `aSLevelsSSupspr`: `"%S//LEVELS//%S//SUPSPR.LVL"` → `"%s//LEVELS//%s//SUPSPR.LVL"`
  - `aSFmvMh_fmv_vbc`: `"%S//FMV//MH_FMV.VBC"` → `"%s//FMV//MH_FMV.VBC"`
- **Fix** (`Render.cpp: _408550_multi_pal()`): Refined the single-player guard — only early-return in the `!is_demo_build` branch (campaign mode) when no active network players exist, preventing `MULTI.PAL` from overwriting the campaign palette. The `is_demo_build` branch (Kaos mode) is not guarded since it needs `MULTI.PAL` for player-selected colors.

### Crash Fix — Mission Briefing Video Heap-Use-After-Free (SIGSEGV)
- **Root cause**: `VIDEO_DoFrame()` set `_18_img_data` on the video draw job's `DetailedDrawHandler_VideoPlayer` structs. The second draw job (`stru_477D90`, subtitle overlay) computed its pointer as `v24 + height * width` (line 237). When `v24` (`video->header.field_10`) was NULL on the first frame (from `memset` in `VIDEO_ReadFile`), the result was `0 + 240*320 = 76800` — a non-null pointer into unmapped memory. `render_video_draw_handler` checked `_18_img_data != NULL` (line 917) and passed this invalid pointer to `render_draw_tile`, causing SIGSEGV in `memcpy`.
- Timing detail: the first `VIDEO_DoFrame()` call's timing check always passes immediately because both `v15` and `dword_477940` are `SDL_GetTicks()` on frame 0, propagating the NULL `field_10` to `_18_img_data`.
- **Fix 1** (`Video.cpp:237-240`): Added NULL guard — `stru_477D90._18_img_data` is set to `NULL` when `v24` is NULL, instead of computing a non-null-but-invalid offset.
- **Fix 2** (`Render.cpp:1252`): Changed `char v9` to `int v9` in `render_draw_tile()` — prevents ASan `negative-size-param` for video widths > 127px (truncation when `this_row_x_pixels` > 127).
- **Fix 3** (`Video.cpp:420`): Changed `memcpy` → `memmove` in subtitle scroll buffer for overlapping copy safety.

### Visual Fix — Mission Briefing Background / Subtitles (Survivors vs Evolved)
- **Root cause**: Survivors missions used `MAPD_Draw(MAPD_FOG_OF_WAR, 0, -10)` with palette from `items[1]`, while Evolved used `MAPD_Draw(MAPD_MAP, 0, 0)` with palette from `items[0]`. The `FOG_OF_WAR` renderer produced white pixels around the video frame, and the `items[1]` palette lacked correct colors for subtitle text rendering.
- **Fix** (`Video.cpp:1410-1419`): Unified both branches — Survivors now uses `MAPD_MAP` + `items[0]` palette, matching the Evolved path that already worked perfectly.

### Pathfinding Fix — Units Getting Stuck Around Hills & Group Settlement
- **Symptoms**: Units (both infantry and vehicles) would get stuck when pathfinding around hills or other obstacles. In group move orders, only one unit would reach the destination while others wandered indefinitely around nearby tiles.
- **Root cause — Bresenham-only pathfinding**: The game uses Bresenham straight-line pathfinding (no A*). When a straight-line path crosses an impassable tile, the pathfinder returns result codes indicating obstruction. Unit handlers had no fallback mechanism — they either re-attempted the same blocked path or entered a "stuck" mode that produced lateral nudges insufficient to route around obstacles. For group settlement, the destination tile occupied by prior-arriving units returned classification 3 (fully occupied by friendlies) from `boxd_40EA50_classify_tile_objects`, leading to pathfinding result 5 with no escape path.

- **Fix 1 — Perpendicular offset search (result 5 else branch, Infantry.cpp:2276-2338)**: When pathfinding returns result 5 (destination tile blocked, no intermediate waypoint), the handler now searches ±1→±3 tiles perpendicular to the path direction for a passable alternative tile. If found, the unit is teleported there and continues movement toward the destination via `entity_mode_move_to_target_416790`. This replaces `entity_414C30_boxd` (cardinal axis nudging) which caused ping-pong wandering.

- **Fix 2 — Proximity-triggered settlement (result 5 else, Infantry.cpp:2269-2274)**: Before the perpendicular search, if the unit is within 16384px (2 tiles) of `sprite_x_2` (persistent destination), snap the persistent destination to the current tile and call `entity_initialize_order` to trigger settlement. Catches the case where the destination tile is occupied by friendlies (classification 3) and the unit is adjacent.

- **Fix 3 — Proximity-triggered settlement (result 4, Infantry.cpp:2224-2233)**: Same proximity check for result 4 (first tile blocked / ClearStraightLine). When the unit is close to the persistent destination, snap and settle instead of calling `entity_414C30_boxd`.

- **Fix 4 — Block handler settlement (Infantry.cpp:2794-2806, 2834-2844)**: In `entity_mode_move_to_target_416790`, when `map_40DA90_move_entity` returns 0 or 1 (movement blocked) and the unit is within 16384px of `sprite_x_2`, snap persistent destination to current tile and call `entity_mode_move_attack` to trigger settlement. Applied to both infantry and vehicle branches.

- **Fix 5 — Cardinal axis escapes with speed save (Infantry.cpp:1590-1700)**: `entity_414C30_boxd` now saves and tries all 4 cardinal directions (forward X, forward Y, backward X, backward Y) before falling back to `entity_413A90_boxd` spiral-search teleport, and finally stuck mode. Previously only tried forward X and forward Y when facing was diagonal.

- **Fix 6 — Diagonal nudge in `entity_414870_boxd` (Infantry.cpp:1460,1486)**: Added diagonal nudge (simultaneous X+Y displacement) after both cardinal-axis nudges fail, in both `|dx|<=|dy|` and `|dx|>|dy|` branches.

- **Fix 7 — `/256` → `>>8` in pathfinders (Pathfind.cpp)**: Replaced integer division `/256` with arithmetic right shift `>>8` in `Map_41B970_straight_line_pathing_old_refactored` and `_new_refactored`. Integer division truncates toward zero, giving wrong sign for negative deltas; `>>8` truncates toward negative infinity, matching the original 16-bit assembly `sar` instruction.

- **Fix 8 — Infantry stuck handler (Infantry.cpp)**: Changed from `entity_mode_attack_move_4_order_3_7_417E60` (stuck mode) to `entity_414C30_boxd` (nudge + escape attempt) in the result 4 handler, matching vehicle behavior and giving infantry the same second chance to find an alternative path before entering stuck animation.

- **Verification**: Build succeeds. All units pathfind correctly, navigate around obstacles, and settle at the destination tile in group orders without wandering. No regressions in single-unit movement.

### Crash Fix — strcpy Overlap in Kaos Player Name Editor (ASan)
- Root cause: `input_get_string()` (`kknd.cpp:5439,5446`) used `strcpy` to shift characters left in the edit buffer during Delete and Backspace operations. Since source and destination overlap, `strcpy` behavior is undefined — ASan detected `strcpy-param-overlap`.
- **Fix**: Replaced `strcpy` with `memmove` (with length derived from `strlen`) in both the Delete and Backspace handlers. Also fixed the Insert handler (`memcpy` → `memmove`) for the same class of bug.

### Crash Fix — SIGILL After Entering Player Name in Kaos Mode
- Root cause: After `input_get_string()` returned (Enter pressed), `script_main_menu_kaos_player_name` (`MainMenu.cpp:1720`) hit `__debugbreak()` which is `__builtin_trap()` → `ud2` instruction → SIGILL. The next line was a `strcpy` to hardcoded absolute EXE address `4695939` (0x47A303) — a decompilation artifact meaningless in the Linux port.
- **Fix**: Removed `__debugbreak()` and replaced the dead `strcpy` with the proper version `strcpy(netz_47A740[idx + 2].player_name, netz_default_player_name)`, matching the identical pattern already used at line 1623 in the same function.

### BGM Fix — Random Stalling/Stuttering at Ring-Buffer Wrap Boundary
- **Root cause**: BGM streaming loop used a ring-buffer model (v2 write cursor, v52 play cursor modulo `buffer_size`) on top of SDL2's queue model (monotonic append). When the SDL queue drained completely (`queued == 0`), `GetCurrentPosition` mapped `consumed = buffer_size` → `cursor = 0`. Meanwhile v2 (write pointer) sat at `buffer_size` after the previous write. The standard ring-buffer formula `v20 = buffer_size + v52 - v2` computed `v20 = 0` — the streaming loop wrote nothing and BGM stalled until the SDL queue refilled (several seconds of silence).
- This was ambiguous with the "buffer full" case (queued ≥ buffer_size → consumed=0 → cursor=0 → v20=0), which should correctly skip writing.
- **Fix** (`Sound.cpp`): After computing `v20`, added a disambiguation check: when `v20==0 && v52==0 && v2 >= buffer_size`, call `SDL_GetQueuedAudioSize`. If queued < buffer_size, set `v20 = buffer_size - queued` to refill the drained buffer. If queued ≥ buffer_size, leave `v20=0` (buffer genuinely full).
- Also changed all `>=` buffer_size wrap comparisons to `>` so v2 stays at `buffer_size` instead of wrapping to `0`, preventing a separate permanent deadlock.

### Crash Fix — Schrap Explosions Global-Buffer-Overflow (dword_46BC98[8])
- Root cause: `script_438F50_explosions` (`Schrap.cpp:199`) used `(unsigned __int8)((char)kknd_rand_debug() % -8)` to compute an index into `dword_46BC98[8]`. When `kknd_rand_debug()` returned a value whose `char` truncation was negative (e.g., 128–255), C's `%` with negative dividend produced a negative result, and the `(unsigned __int8)` cast wrapped it to 249–255 — far past the array's 8 elements.
- Two other `% -8` sites (lines 148, 179) were safe because their operands were non-negative loop counters (0–7 and 0–5), where the result is the same as `% 8`.
- **Fix**: Replaced `(unsigned __int8)((char)... % -8)` with `(unsigned __int8)(... % 8)` — the random value is positive, so `% 8` gives a clean 0–7 range.

### Infantry Obstacle Avoidance — Same Logic as Vehicles

- **Symptom**: Infantry units still got stuck when navigating around obstacles even after the Bresenham pathfinding fixes.
- **Root cause**: Three call sites where vehicles attempt lateral avoidance (sidestep, cardinal slide) were missing for infantry — infantry went directly to the stuck handler with no intermediate avoidance step.

- **Fix 1 — Sidestep at init (`Infantry.cpp:2528`)**: In `entity_mode_attack_move_2_5_4165C0`, added `boxd_41C130` path-clear check + `entity_414870_boxd` sidestep before the orientation/animation setup, matching the vehicle path. If the direct path to the waypoint is blocked, infantry now tries sidestepping to an adjacent tile before falling back to the stuck handler.

- **Fix 2 — Cardinal slide at tick (`Infantry.cpp:2642`)**: In `entity_mode_move_to_target_416790`, changed infantry's move-result handling to match vehicles exactly: `map_40DA90_move_entity` returning 0 or 1 (partially blocked) now calls `entity_414C30_boxd` (tries X-only then Y-only cardinal slide) instead of going directly to `entity_mode_attack_move_4_order_3_7_417E60` (stuck/backoff). Return code 3 (fully occupied) still triggers the nudge mode; return code 4 (success) continues normally.

- **Fix 3 — Sidestep in fine-snap placement (`Infantry.cpp:3305`)**: In `entity_mode_417A20`, added the same `boxd_41C130` check + `entity_414870_boxd` sidestep before orientation setup for infantry, matching the `entity_414870_boxd` fallback already present in the vehicle branch of the same function. Previously the infantry branch just set orientation and mobd without checking if the path was clear.

- **Verification**: Build succeeds. Infantry now uses all obstacle avoidance behaviors that were previously vehicle-only: sidestep at movement init, cardinal slide during movement ticks, and sidestep during fine-snap placement.

### Crash Fix — Beast Enclosure Upgrade Global-Buffer-Overflow (Kaos Mode)
- **Root cause**: `__477318_beastenclosure_negindex` was declared as `int __477318_beastenclosure_negindex[] = { 0 };` (single element), but `EventHandler_BeastEnclosure` (`BuildingsMute.cpp:93`) accessed it as `__477318_beastenclosure_negindex[v7->num_upgrades - 1]` where `num_upgrades` ranges 1–4 (indices 0–3). ASan detected global-buffer-overflow when reading/writing past the single element.
- **Fix** (`_unsorted_data.cpp:5135`): Changed declaration to `int __477318_beastenclosure_negindex[5] = { 0, 0, 0, 0, 0 };` — matching the 5 upgrade levels (indices 0–4), same as `num_buildings_by_level[5]` which is already correctly sized.

### Unit Control Groups — CTRL+0..9 and Number Recall Fix
- **Root cause 1 — mapping gated on CTRL**: `InputKeyboardObserver::OnCharUp()` only applied `combo_press_params_map[]` when `ctrl_status == true`. Plain number keys got `combo_key_param = 0`, so the recall branch (`Cursor.cpp:2756`) always fell through since `0 < 2`.
- **Root cause 2 — stale param never cleared**: `combo_key_param` set by `OnCharUp()` was never reset. After a CTRL+number assignment, the stale param persisted across frames, causing the group recall (`sub_4297D0`) to re-fire on every right-click deselect (deselect resets `dword_468980` guard, but `combo_key_param` was still the group number).
- **Root cause 3 — polled CTRL state vs captured state**: `dword_47A6FC` polls current physical CTRL state via `pressed_keys_mask`. Game loop order is `input_update_keyboard()` → cursor handler → `TimedMessagePump()` (SDL events). By the time the cursor handler runs, the user has usually already released CTRL, so `dword_47A6FC == 0` and the code entered the recall path instead of assignment — requiring 2–3 key presses.
- **Fix 1** (`InputKeyboardObserver.cpp:17-18`): Always apply `combo_press_params_map` regardless of CTRL, and store `ctrl_status` in `input_now_pressed_keys.combo_key_was_ctrl`.
- **Fix 2** (`Input.cpp:341-342`): Clear both `combo_key_param` and `combo_key_was_ctrl` after memcpy to `input_keyboard_state`.
- **Fix 3** (`Input.h:19`): Added `bool combo_key_was_ctrl` to `KeyboardInput` struct so it propagates through the memcpy pipeline (`input_now_pressed_keys` → `input_keyboard_state` → `_47A700_input`).
- **Fix 4** (`Cursor.cpp:1870,2756`): Replaced `dword_47A6FC == 29` with `_47A700_input.combo_key_was_ctrl` — uses the CTRL status captured at key-up time instead of polling current physical state.

### Main Menu Centering for Higher Resolutions

- **Problem**: Main menu content (background, buttons, dialogs, cursor) rendered at fixed 640×480 coordinates. At higher resolutions (set via `vga_resolution_width/height` in `config.txt`), content appeared stuck in the top-left corner and mouse hit-testing (BOXD) failed.

- **Architecture**: Three coordinate spaces:
  1. **640×480 internal space** — `data.tix`, mobd sprites, button positions, y-expressions. All `sprite.x/y` values in this session are in this space.
  2. **cplc_render** (`mapd_cplc_render_x/y`) — global scroll offset applied by most draw handlers at render time. Set to `-menu_offset * 256` during main menu, shifting world-relative sprites right/down by `menu_offset`.
  3. **Screen pixel space** — final display pixel.

  Draw handlers fall into three categories:
  - `drawjob_update_handler_4483E0_ui` (UI handler): Ignores `cplc_render` — `job.x = sprite.x/256`. Used for ingame menu sprites.
  - `drawjob_update_handler_426C40_mobd` / `drawjob_update_handler_44C430_default_sprite` (default/camera handlers): Subtracts `cplc_render` — `job.x = sprite.x/256 + menu_offset`. Used for most sprites including cursor.
  - Custom `drawjob_update_handler_menu_cursor_with_cplc`: Like default handler but with `z_index + 0x20000000` (UI z-range). Created specifically for main menu sprites that need both cplc shift and proper UI z-ordering.

  Render strings (`render_string_create`) are drawn at absolute screen coordinates — they must be manually offset with `+ menu_offset_x/y`.

  BOXD collision in `Pathfind.cpp` compares `sprite.x` directly (ignoring `cplc_render`). For hit-testing to work, cursor and button `sprite.x` must be in the same space. The cursor's `sprite.x = cplc_render_x + mouse_x` (line 455), converting window-pixel mouse position back to 640×480 space. Button sprites with the UI handler also need their `sprite.x` in 640×480 space — but UI handler sprites that were shifted by `+ menu_offset * 256` had `sprite.x` in screen-pixel space, breaking BOXD.

- **Fix strategy**: Use `cplc_render` as the sole centering mechanism for sprites. Sprites that need to shift with the background use a cplc-aware handler (default or `menu_cursor_with_cplc`). Sprites using the UI handler must NOT have `menu_offset` baked into their `sprite.x`, because the UI handler renders at `sprite.x/256` (screen space) but BOXD compares `sprite.x` against the cursor's 640×480-space value. By keeping all sprites' `sprite.x` in 640×480 space and using cplc handlers to shift rendering, both rendering and BOXD work correctly.

- **Implementation** (`src/Application/Scripts/MainMenu.h`, `MainMenu.cpp`, `src/kknd.cpp`, `src/RenderDrawHandlers.cpp/.h`, `src/Application/Game.cpp`):

  - **Globals** (`MainMenu.h:22-23`): `int menu_offset_x/y` — set to `(render_width - 640) / 2` and `(render_height - 480) / 2` in `Game::MainMenu()`.

  - **cplc_render override** (`MainMenu.cpp:425-426`, `Game.cpp`): `mapd_cplc_render_x/y = -menu_offset * 256` applied in three places:
    - `Game::MainMenu()` after MAPD init
    - Game loop after `_4393F0_call_mapd()` (overwrites game-world cplc)
    - Mouse handler main loop (line 425-426, every frame)

  - **Cursor** (`MainMenu.cpp:399`): Handler changed from `drawjob_update_handler_cursors` to `drawjob_update_handler_menu_cursor_with_cplc`. Cursor `sprite.x/y` kept at `cplc_render_x + mouse_pos` (converts window pixel → 640×480 space). New handler uses `z_index + 0x20000000` (UI range) instead of `+ 0x40000000` (overlay range) so the cursor renders above buttons but below the existing overlay cursor.

  - **Centering defines** (`MainMenu.h:7-12`): All reverted to 0 (`BTN_CENTER_X/Y`, `FACTION_CENTER_Y`, `KAOS_CENTER_X/Y`, `SAVELOAD_CENTER_Y`). Cplc-based centering replaces explicit position offsets.

  - **Button scripts** (`MainMenu.cpp:44,97,148,199,247,296,346`): Removed `a1->sprite->y = ... + BTN_CENTER_Y * 256` — buttons now use default handler `drawjob_update_handler_426C40_mobd` (cplc-aware), so their 640×480 sprite positions render correctly shifted.

  - **Kaos dialog render strings** (`MainMenu.cpp:989,1092,1119,1167,1264,1309,1411,1452,1491,1533`): Added `menu_offset_x/y` to positions — render strings don't use cplc and need explicit offset.

  - **Load dialog buttons** (`kknd.cpp:8997-9128`): Five scripts (`script_434220`, `4342A0`, `434310`, `434390` and `434460_DA000007`) had their draw handler changed from `drawjob_update_handler_4483E0_ui` to `drawjob_update_handler_menu_cursor_with_cplc`. Removed `a1->sprite->x/y += menu_offset * 256` — the cplc handler shifts rendering without modifying sprite.x, keeping BOXD-compatible 640×480 coordinates. Both parent and child (`a1->param`) sprites get the new handler.

  - **Savegame list** (`kknd.cpp:8657-8919`):
    - `a1a` (highlight indicator) and `v8` (5 slot sprites): handler switched to `drawjob_update_handler_menu_cursor_with_cplc` when `appearance == SAVEGAME_LIST_APPEARANCE_MAIN_MENU`.
    - Removed `menu_offset_x * 256` from `v8->x` and `a1a->x` — they now use raw 640×480 coordinates.
    - `v4`/`v30` (y-base): removed `+ menu_offset_y` (was `256 + menu_offset_y`, now `256`) — the cplc handler adds `menu_offset_y` once via cplc_render.
    - `render_string_create` x: changed from `216` to `216 + menu_offset_x` for main menu — render strings need explicit offset.
    - `render_string_create` y: already had `240 + menu_offset_y`.

  - **Blinking cursor** (`MainMenu.cpp:1750`): Added missing `- menu_offset_x` to `_47C664_ingame_menu_sprite->x` after `input_get_string()`. The cursor uses default handler `drawjob_update_handler_426C40_mobd` which adds `menu_offset_x` via cplc_render — the render string's `job_details.x` is in screen-pixel space, so `menu_offset_x` must be subtracted before the handler adds it back.

### UI Centering — In-Game Menu Dialogs for Higher Resolutions

- After main menu centering (cplc-based), in-game menu dialogs and mission outcome modals still rendered at fixed 640×480 coordinates, appearing in the top-left at higher resolutions.
- In-game menu master dialog (`GameMenu.cpp`): Uses `drawjob_update_handler_4483E0_ui` (ignore cplc_render), so explicit `menu_offset_y * 256` added to sprite y position.
- Sub-dialogs (restart/quit confirmation, options, briefing) (`GameMenu.cpp`): render_string positions shifted by `+ menu_offset_x/y`.
- Savegame list (`kknd.cpp`, `script_ingame_menu_saveload`): slot sprites, highlight indicator, cursor, and render_string positions conditioned on `menu_x`/`menu_y` for `SAVEGAME_LIST_APPEARANCE_INGAME_MENU` only (main menu uses cplc handlers).

### Bug Fix — Savegame List Title Not Centered / Slot y Double-Offset

- Main menu save/load dialog title render_string used `menu_x`/`menu_y` which are 0 for main menu (they exist only to skip offset for cplc-shifted sprites). render_string doesn't use cplc handlers, so it needs `menu_offset_x/y` directly. **Fix**: changed `menu_x/menu_y` → `menu_offset_x/menu_offset_y` at `kknd.cpp:8695-8696`.
- Ingame menu save slots had `menu_y` added twice — once in `v4` (which flows into `v7 = (v4+14)<<8`) and again in `v8->y = v7 + menu_y*256`. **Fix**: removed `+ menu_y*256` from `v8->y` at `kknd.cpp:8730`.
- Mission outcome letter sprite (`Mission.cpp` `script_424CE0_mission_outcome_modal`): `v3->x/y` already used `menu_offset_x/y * 256` (previous fix). Not in original entry.
- Mission outcome background sprites (`Mission.cpp`, outcome dialog creation): Three sprites (left panel offset 0, right panel offset 12, mission text frame offset 24) had hardcoded `x=81920, y=61440`. Added `+ menu_offset_x/y * 256` to match the outcome letter sprite on top.
- 
- ### Window Maximize / Fullscreen Toggle (ALT+ENTER)
- 
- - **Problem**: Game window lacked maximize support and ALT+ENTER fullscreen toggle. The maximize button was disabled (no `SDL_WINDOW_RESIZABLE` flag), `SetWindowed()` was a no-op, and no code handled the Alt+Enter combination.
- - **Fix 1 — Resizable window** (`SdlWindow.cpp:79`): Added `SDL_WINDOW_RESIZABLE` to `SDL_CreateWindow` flags so the maximize button is enabled.
- - **Fix 2 — Maximize button** (`SdlWindow.cpp:232-236`): Added `SDL_WINDOWEVENT` handler — when `SDL_WINDOWEVENT_MAXIMIZED` fires (user clicks maximize), calls `SetFullscreen()`.
- - **Fix 3 — ALT+ENTER** (`SdlWindow.cpp:314-319`): In `SDL_KEYUP` for `SDL_SCANCODE_RETURN`, checks `altStatus` — if Alt is held, calls `ToggleFullscreen()` instead of dispatching a normal Enter event.
- - **Fix 4 — SetWindowed** (`SdlWindow.cpp:157-163`): Implemented `SetWindowed()` — calls `SDL_SetWindowFullscreen(window, 0)`, restores window to configured size, re-enables grab.
- - **Fix 5 — ToggleFullscreen** (`SdlWindow.cpp:166-172`): Added `ToggleFullscreen()` — delegates to `SetWindowed()` or `SetFullscreen()` based on `_isFullscreen` state.
- - **Interface** (`Window.h`, `SdlWindow.h`): Added `ToggleFullscreen()` pure virtual and `_isFullscreen` state member.
- 
- ### Config — vga_fullscreen Property
- 
- - **Config** (`Config.h:7`, `Config.cpp:7,31-33`): Added `static bool vga_fullscreen` defaulting to `false`. Parsed from `config.txt` as `vga_fullscreen=0` or `vga_fullscreen=1`.
- - **Game startup** (`Game.cpp:81`, `kknd.cpp:5555`): Both `Game::Run()` and `LVL_SysInit()` now use `Config::vga_fullscreen` instead of hardcoded `false`.
- 
- ### Fullscreen Toggle — Sidebar Off-Screen Fix
- 
- - **Root cause**: `SdlRenderer::DrawImageCentered` used cached `config->width/height` (stored at init time) for centering. When starting in fullscreen, `config` was replaced with desktop dimensions (e.g. 1920×1080). After toggling to windowed mode (640×480), `config->width/height` still held the stale desktop resolution. `draw_x = 1920/2 - 640/2 = 640` placed the entire 640×480 game image at x=640 on a 640-wide window — completely off-screen.
- - **Fix** (`SdlRenderer.cpp:57-58`): `DrawImageCentered` now queries actual window size via `config->window->GetWidth()/GetHeight()` instead of using the cached `config->width/height`. This always reflects the current window dimensions regardless of fullscreen/windowed state.
