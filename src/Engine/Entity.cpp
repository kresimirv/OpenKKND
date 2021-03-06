#include "Engine/Entity.h"

#include "src/_unsorted_functions.h"
#include "src/_unsorted_data.h"
#include "src/kknd.h"
#include "src/Map.h"
#include "src/Pathfind.h"
#include "src/Script.h"
#include "src/Sprite.h"

#include "Engine/Infrastructure/EntityRepository.h"

using Engine::Infrastructure::EntityRepository;




void Entity::SetMode(EntityMode mode) {
if (unit_id == UNIT_STATS_SURV_MOBILE_DERRICK)
log("SetMode: %s", get_handler_name(mode));
    this->mode = mode;
}
bool Entity::IsMode(EntityMode mode) const {
    return this->mode == mode;
}
void Entity::ExecMode() {
if (unit_id == UNIT_STATS_SURV_MOBILE_DERRICK)
log("ExecMode: %s", get_handler_name(mode));
    (mode)(this);
}
int Entity::ModeHandlerId() const {
    extern int get_handler_id(void *function);
    return get_handler_id(mode);
}

void Entity::SetReturnModeFromMode() {
    mode_return = mode;
}


int entity_get_mobd_speed_x(Entity *entity)
{
    if (entity->GetCurrentAnimFrame() != -1)
    {
        return entity->stats->speed * _4731A8_speeds[__47CFC4_mobd_lookup_speeds[entity->GetCurrentAnimFrame() + 1]] / 64;
    }
    return 0;
}

int entity_get_mobd_speed_y(Entity *entity)
{
    if (entity->GetCurrentAnimFrame() != -1)
    {
        return entity->stats->speed * _4731A8_speeds[8 + __47CFC4_mobd_lookup_speeds[entity->GetCurrentAnimFrame() + 1]] / 64;
    }
    return 0;
}

// XL vehicles: mobile outpost, autocannon tank & missile crab
bool entity_is_xl_vehicle(Entity *entity) {
    return entity->stats->field_4C == 4096;
}

bool entity_is_regular_vehicle(Entity *entity) {
    return entity->stats->field_4C == 128;
}

bool entity_is_infantry(Entity *entity) {
    return entity->stats->field_4C == 512;
}


bool entity_is_tower(Entity *entity) {
    return is_tower(entity->unit_id);
}

bool entity_is_bomber(Entity *entity) {
    return is_bomber(entity->unit_id);
}

bool entity_is_21st_century(Entity *entity) {
    return is_21st_century(entity->unit_id);
}

bool entity_is_moveable(Entity *entity) {
    return entity->unit_id <= UNIT_STATS_MUTE_MISSILE_CRAB || entity->unit_id >= UNIT_STATS_GORT;
}

bool entity_is_attacker(Entity *entity)
{
    return
        entity_is_moveable(entity)
        && (entity->unit_id < UNIT_STATS_SURV_MOBILE_DERRICK || entity->unit_id > UNIT_STATS_TANKER_CONVOY)
        && (entity->unit_id < UNIT_STATS_SURV_TECHNICIAN || entity->unit_id > UNIT_STATS_MUTE_MEKANIK)
        || entity_is_tower(entity);
}



bool is_21st_century(UNIT_ID unit_id) {
    return unit_id >= UNIT_STATS_GORT && unit_id <= UNIT_STATS_MECH;
}

bool is_tower(UNIT_ID unit_id) {
    return unit_id >= UNIT_STATS_SURV_GUARD_TOWER && unit_id <= UNIT_STATS_MUTE_ROTARY_CANNON;
}

bool is_bomber(UNIT_ID unitId) {
    return unitId == UNIT_STATS_SURV_BOMBER || unitId == UNIT_STATS_MUTE_WASP;
}



void entity_load_mobd_4(Entity *entity)
{
    if (entity->stats->mobd_lookup_offset_4 != -1)
    {
        sprite_4272E0_load_mobd_item(
            entity->sprite,
            entity->stats->mobd_lookup_offset_4,
            _47D3C4_entity_mobd_lookup_ids[entity->GetCurrentAnimFrame() + 1]);
    }
}

void entity_load_idle_mobd(Entity *entity, int idx)
{
    entity->SetCurrentAnimFrame(idx);
    entity_load_idle_mobd(entity);
}

void entity_load_idle_mobd(Entity *entity)
{
    if (entity->stats->mobd_lookup_offset_idle != -1)
    {
        sprite_4272E0_load_mobd_item(
            entity->sprite,
            entity->stats->mobd_lookup_offset_idle,
            _47D3C4_entity_mobd_lookup_ids[entity->GetCurrentAnimFrame() + 1]);
    }
}

void entity_load_move_mobd(Entity *entity, int idx)
{
    entity->SetCurrentAnimFrame(idx);
    entity_load_move_mobd(entity);
}

void entity_load_move_mobd(Entity *entity)
{
    if (entity->stats->mobd_lookup_offset_move != -1)
    {
        sprite_4272E0_load_mobd_item(
            entity->sprite,
            entity->stats->mobd_lookup_offset_move,
            _47D3C4_entity_mobd_lookup_ids[entity->GetCurrentAnimFrame() + 1]);
    }
}

void entity_load_attack_mobd(Entity *entity)
{
    if (entity->stats->mobd_lookup_offset_attack != -1)
    {
        sprite_4272E0_load_mobd_item(
            entity->sprite,
            entity->stats->mobd_lookup_offset_attack,
            _47D3C4_entity_mobd_lookup_ids[entity->GetCurrentAnimFrame() + 1]);
    }
}


void entity_load_attack_mobd(Entity *entity, int idx)
{
    entity->SetCurrentAnimFrame(idx);
    entity_load_attack_mobd(entity);
}

//----- (0041A270) --------------------------------------------------------
void entity_move(Entity *a1, _47CAF0_task_attachment1_move_task *a2)
{
    int v4; // eax@5
    int v5; // eax@9
    int v6; // eax@11

    if (a1->player_side == a2->player_side)
    {
        v4 = a1->_DC_order == ENTITY_ORDER_MOVE
            && map_is_same_tile(a2->dst_x, a1->sprite_x_2)
            && map_is_same_tile(a2->dst_y, a1->sprite_y_2);
        if (!v4 || a1->entity_8)
        {
            v5 = a2->dst_x;
            if (v5 >= 0 && v5 < map2global(map_get_width()))
            {
                v6 = a2->dst_y;
                if (v6 >= 0 && v6 < map2global(map_get_height()))
                {
                    a1->pathing.field_54 = 0;
                    a1->pathing.field_50 = 0;
                    script_sleep(a1->script, 1);

                    a1->_DC_order = ENTITY_ORDER_MOVE;
                    a1->_E4_prev_attack_target = a1->retaliation_target;
                    a1->_E4_prev_attack_target_entity_id = a1->retaliation_target_id;
                    a1->retaliation_target = nullptr;
                    a1->_134_param__unitstats_after_mobile_outpost_plant = 600;

                    a1->sprite_x_2 = map_adjust_entity_in_tile_x(a1, a2->dst_x);
                    a1->sprite_y_2 = map_adjust_entity_in_tile_y(a1, a2->dst_y);
                    entity_414440_boxd(a1, &a1->sprite_x_2, &a1->sprite_y_2);

                    a1->entity_8 = 0;
                    Map_40DF50_update_tile(a1, 1);

                    a1->SetMode(entity_mode_move_attack);
                }
            }
        }
    }
}


//----- (00419F00) --------------------------------------------------------
void entity_attack(Entity *a1, _47CAF0_task_attachment1_attack_task *param)
{
    Entity *v3; // esi@1
    enum PLAYER_SIDE v4; // ecx@1
    Entity *v5; // edx@2
    enum PLAYER_SIDE v6; // eax@3
    int v7; // eax@7

    v3 = a1;
    v4 = a1->player_side;
    if (v4 == param->player_side)
    {
        v5 = param->target;
        if (v5 == v3->retaliation_target)
        {
            v3->_E4_prev_attack_target = 0;
            v3->_134_param__unitstats_after_mobile_outpost_plant = 600;
            if (v3->entity_8 != v3->retaliation_target)
                v3->entity_8 = 0;
        }
        else
        {
            v6 = v5->player_side;
            if (v6 && v6 != v4 && !is_enemy(v4, v5))
            {
                if (!single_player_game)
                {
                    v7 = param->target->player_side;
                    if (player_side == v7 || player_side == v3->player_side)
                    {
                        sprintf(
                            byte_479B00,
                            aAllegianceBroken,
                            netz_47A740[v7 + 1].player_name,
                            netz_47A740[v3->player_side + 1].player_name);
                        show_message_ex(0, byte_479B00);
                    }
                }
                game_globals_cpu[param->target->player_side].cash[v3->player_side] = 0;
                game_globals_cpu[v3->player_side].cash[param->target->player_side] = 0;
            }
            v3->pathing.field_54 = 0;
            v3->pathing.field_50 = 0;
            script_sleep(v3->script, 1);
            v3->_DC_order = ENTITY_ORDER_ATTACK;
            v3->retaliation_target = param->target;
            v3->retaliation_target_id = param->target->entity_id;
            v3->_E4_prev_attack_target = 0;
            v3->_134_param__unitstats_after_mobile_outpost_plant = 600;
            v3->entity_8 = 0;
            v3->SetMode(entity_mode_move_attack);
        }
    }
}


//----- (00425820) --------------------------------------------------------
Entity *entity_find_player_entity_in_radius(Entity *a1, int max_distance_squared)
{
    for (auto i : entityRepo->FindAll())
    {
        if (!i->destroyed && i->player_side == player_side)
        {
            int dx = abs(a1->sprite->x - i->sprite->x);
            int dy = abs(a1->sprite->y - i->sprite->y);
            if (dx + dy < max_distance_squared)
                return i;
        }
    }
    return nullptr;
}


//----- (004078B0) --------------------------------------------------------
Entity *entity_find_any_entity_in_radius(Entity *a1, int max_distance_squared)
{
    for (auto i : entityRepo->FindAll())
    {
        if (!i->destroyed && i->player_side != PLAYER_SIDE_UNSPECIFIED)
        {
            int dx = abs(a1->sprite->x - i->sprite->x);
            int dy = abs(a1->sprite->y - i->sprite->y);
            if (dx + dy < max_distance_squared)
                return i;
        }
    }
    return nullptr;
}


int entity_advance_rotation(Entity *entity, int dst, int step) {
    auto frame = entity->GetCurrentAnimFrame();
    int res = mobd_advance_anim(&frame, dst, step);
    entity->SetCurrentAnimFrame(frame);

    return res;
}