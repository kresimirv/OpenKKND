#pragma once

#include "src/Script.h"


/* 331 */
struct SidebarButton
{
    SidebarButton *next;
    SidebarButton *prev;
    Script *task;
    void(*open_handler)(SidebarButton *);
    void(*close_handler)(SidebarButton *);
    int mobd_lookup_table_offset;
    int field_18;
    int ptr_1C;
    void *param;
    Sprite *sprite;
};

/* 330 */
struct Sidebar
{
    Sidebar *next;
    Sidebar *prev;
    Script *script;
    int num_buttons;
    int x;
    int w;
    int sprite_width_step;
    int sprite_height_step;
    Sprite *sprite;
    SidebarButton *button_list_free_pool;
    SidebarButton *button_list_head;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
};

extern const char *sidebar_category_labels[11];
extern int _4701A8_sidebar_mask_per_44_mobd_lookup_idx[]; // idb
extern void(*_470478_sidebar_button_open_handlers[5])(SidebarButton *);
extern void(*_470490_sidebar_button_close_handlers[5])(SidebarButton *);
extern int _4704A8_per_sidebar_button_mobd_lookup_table_offsets[2][11];
extern Sidebar *stru22_list_479548;
extern Sidebar *stru22_list_47954C;
extern Sidebar *sidebar_list;
extern Sidebar *sidebar_list_head;
extern SidebarButton *sidebar_button_list_4795A0;
extern SidebarButton *sidebar_button_list_4795A4;
extern SidebarButton *sidebar_button_list;
extern SidebarButton *sidebar_button_list_head;
extern int sidebar_button_list_sidebar_height; // weak
extern int sidebar_button_list_item_width; // weak
extern Script *_47A734_sidebar_tooltips_task; // idb
extern Sidebar *_47C914_sidebar;
extern Sprite *_47C96C_mobd_1F_sidebar_empty_blocks; // idb
extern Script *_47C970_sidebar_task; // idb
extern SidebarButton *_47CA08_sidebar_buttons[2];
extern SidebarButton *_47CA10_sidebar_button_minimap;
extern SidebarButton *_47CA18_sidebar_production_buttons[5];
extern int _47CA2C_should_airstrike_mess_with_sidebar; // weak

bool sidebar_initialize();
void sidebar_button_handler_cash_open(SidebarButton *a1);
void sidebar_button_handler_cash_close(SidebarButton *a1);
void sidebar_button_handler_minimap_open(SidebarButton *a1);
void sidebar_button_handler_minimap_close(SidebarButton *a1);
void sidebar_button_handler_options_open(SidebarButton *a1);
void sidebar_button_handler_help_open(SidebarButton *a1);
void sidebar_button_handler_help_close(SidebarButton *a1);
void sidebar_button_handler_infantry_open(SidebarButton *a1);
void sidebar_button_handler_infantry_close(SidebarButton *a1);
void sidebar_button_handler_order_unit_click(SidebarButton *a1);
void sidebar_button_handler_vehicles_open(SidebarButton *a1);
void sidebar_button_handler_vehicles_close(SidebarButton *a1);
void sidebar_button_handler_buildings_open(SidebarButton *a1);
void sidebar_button_handler_buildings_close(SidebarButton *a1);
void sidebar_button_handler_order_building_click(SidebarButton *a1);
void sidebar_button_handler_towers_open(SidebarButton *a1);
void sidebar_button_handler_towers_close(SidebarButton *a1);
void sidebar_button_handler_airstrike_open(SidebarButton *a1);
void sidebar_button_handler_airstrike_close(SidebarButton *a1);
void sidebar_button_handler_446190_open(SidebarButton *a1);
void sidebar_button_handler_4461E0_close(SidebarButton *a1);
void sidebar_close_all();
Sidebar *sidebar_list_create(Sprite *sprite, Script *script, int width, int height, int sidebar_horizontal);
bool sidebar_button_list_alloc();
void script_40F5D0_sidebar_button_1_2(Script *a1);
void script_40F8F0_sidebar_button_3(Script *a1);
void script_40FC10_sidebar_button_4(Script *a1);
SidebarButton *sidebar_add_button_1(Sidebar *sidebar, int mobd_lookup_table_offset, void(*open_handler)(SidebarButton *), void *param, void *task_context);
SidebarButton *sidebar_add_button_2(Sidebar *sidebar, int mobd_lookup_table_offset, void(*button_open_handler)(SidebarButton *), void(*button_close_handler)(SidebarButton *), void *task_context);
SidebarButton *sidebar_add_button_3(Sidebar *sidebar, int mobd_lookup_table_offset, void(*button_open_handler)(SidebarButton *), void(*button_close_handler)(SidebarButton *), void *task_context);
SidebarButton *sidebar_add_button_4(Sidebar *sidebar, int mobd_lookup_table_offset, void(*button_open_handler)(SidebarButton *), int a4, int a5, void *param, enum UNIT_ID a7);
SidebarButton *sidebar_add_buttton_internal(
    Sidebar *a1, int mobd_lookup_table_offset, void(*task_routine)(Script *),
    void(*open_handler)(SidebarButton *), void(*close_handler)(SidebarButton *), void *param,
    void *task_context, enum SCRIPT_TYPE event
);
void script_sidebar(Script *a1); // idb
void script_446ED0_sidebar_buttons(Script *a1);
void script_401C30_sidebar(Script *a1);
void script_42D030_sidebar_tooltips(Script *a1);
void _4471E0_send_sidebar_buttons_message();
void _447340_send_sidebar_buttons_message(int excluding_button_id);
bool _44CDC0_sidebar_is_units_limit();
Sprite *_447310_minimap();
void sidebar_remove_button(Sidebar *a1, SidebarButton *a2);
void sidebar_list_remove(Sidebar *a1);
void sidebar_list_free();
void sidebar_deinit();