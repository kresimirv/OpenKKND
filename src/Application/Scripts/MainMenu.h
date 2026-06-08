#pragma once
#include "src/Script.h"


// Global offset to center the 640x480 main menu content within the render viewport.
// Set based on (render_width - 640) / 2 and (render_height - 480) / 2.
// When cplc_render_x/y are set to -menu_offset * 256, all sprites that use cplc_render
// in their draw handlers (background via drawjob_update_handler_mapd_menu, buttons via
// drawjob_update_handler_44C430_default_sprite) automatically shift to the correct position.
// The cursor uses drawjob_update_handler_menu_cursor_with_cplc which subtracts cplc_render
// from its position, so its sprite.x/y is in the shifted 640x480 space (cplc_render + cursor).
// This ensures the cursor's sprite.x matches button sprite.x values for BOXD hit-testing.
// =========== MAIN MENU dialog scripts ===================
void script_main_menu_new_campaign(Script *a1);
void script_main_menu_load(Script *a1);
void script_main_menu_play_mission(Script *a1);
void script_main_menu_new_missions(Script *a1);
void script_main_menu_kaos_mode(Script *a1);
void script_main_menu_multiplayer(Script *a1);
void script_main_menu_quit(Script *a1);
// =======



// =========== MAIN MENU NEW CAMPAIGN dialog scripts ===================
void script_main_menu_new_campaign_surv(Script *a1);
void script_main_menu_new_campaign_mute(Script *a1);
void script_main_menu_new_campaign_cancel(Script *a1);
// =======



// =========== MAIN MENU LOAD dialog scripts ===================
void script_main_menu_create_load_dialog(Script *a1);   // dialog initialization script
// =======



// =========== MAIN MENU PLAY MISSION dialog scripts ===================
void script_main_menu_play_mission_faction_toggle(Script *a1);
void script_main_menu_play_mission_ok(Script *a1);
void script_main_menu_play_mission_cancel(Script *a1);
// =======


// =========== MAIN MENU MULTIPLAYER dialog scripts ===================
void script_main_menu_multiplayer_ipx(Script *a1);
void script_main_menu_multiplayer_serial(Script *a1);
void script_main_menu_multiplayer_modem(Script *a1);
void script_main_menu_multiplayer_cancel(Script *a1);
// =======



// =========== MAIN MENU KAOS dialog scripts ===================
void script_main_menu_create_kaos_dialog(Script *a1);
void script_main_menu_kaos_player_name(Script *a1); // idb
void script_main_menu_kaos_starting_cash(Script *a1);
void script_main_menu_kaos_color(Script *a1);
void script_main_menu_kaos_map(Script *a1);
void script_main_menu_kaos_max_tech_level(Script *a1);
void script_main_menu_kaos_bunkers_toggle(Script *a1);
void script_main_menu_kaos_team(Script *a1); // idb
void script_main_menu_kaos_allies(Script *a1);
void script_main_menu_kaos_enemies(Script *a1);
void script_main_menu_kaos_difficulty(Script *a1);
void script_main_menu_kaos_start(Script *a1); // idb
void script_main_menu_kaos_cancel(Script *a1);
// =======