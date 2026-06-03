#pragma once

#include "Engine/Entity.h"

int stru31_list_alloc();
void stru31_list_free();
void entity_401070_stru31(Entity *a1);

extern stru31 stru31_sentinel;
extern stru31 *stru31_list;
extern stru31 *stru31_list_free_pool;