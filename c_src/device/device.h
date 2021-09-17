/*
#  Created by Boyd Multerer on 2021/09/07
#  Copyright 2018-2021 Kry10 Limited
#
*/


NVGcontext* device_init( device_info_t* p_info );
void device_close( device_info_t* p_info );
void device_swap_buffers();