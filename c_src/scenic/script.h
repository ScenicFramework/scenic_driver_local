/*
#  Created by Boyd Multerer on 2021-03-09.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "scenic_types.h"
#include "tommyds/src/tommyhashlin.h"


void init_scripts(void);

void put_script(int* p_msg_length);
void delete_script(int* p_msg_length);

// script_t* get_script(uint32_t id);
void reset_scripts();
void render_script(void* v_ctx, sid_t id);
