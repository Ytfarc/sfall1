/*
 *    sfall
 *    Copyright (C) 2008-2015  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

#include "FalloutEngine.h"

const DWORD action_get_an_object_ = 0x411F98;
const DWORD action_loot_container_ = 0x412254;
const DWORD action_use_an_item_on_object_ = 0x411DA8;
const DWORD AddHotLines_ = 0x4896CC;
const DWORD art_exists_ = 0x4190B4;
const DWORD art_id_ = 0x41944C;
const DWORD art_ptr_lock_data_ = 0x4189B8;
const DWORD art_ptr_unlock_ = 0x418A90;
const DWORD buf_to_buf_ = 0x4BE494;
const DWORD combat_ai_ = 0x425E9C;
const DWORD combat_is_shot_blocked_ = 0x424518;
const DWORD combat_should_end_ = 0x420BD4;
const DWORD combat_turn_ = 0x420918;
const DWORD config_set_value_ = 0x426B50;
const DWORD credits_ = 0x427250;
const DWORD credits_get_next_line_ = 0x4279EC;
const DWORD critter_body_type_ = 0x4287C8;
const DWORD critter_is_dead_ = 0x42871C;
const DWORD critter_name_ = 0x427C28;
const DWORD critter_pc_set_name_ = 0x427CB8;
const DWORD db_fclose_ = 0x4B2B94;
const DWORD debug_register_env_ = 0x4B3380;
const DWORD dialog_out_ = 0x41BFF0;
const DWORD display_inventory_ = 0x463BB8;
const DWORD display_print_ = 0x42C0AC;
const DWORD display_table_inventories_ = 0x468598;
const DWORD display_target_inventory_ = 0x464060;
const DWORD DOSCmdLineDestroy_ = 0x4CE77C;
const DWORD endgame_slideshow_ = 0x438910;
const DWORD game_get_global_var_ = 0x43C998;
const DWORD game_set_global_var_ = 0x43C9C8;
const DWORD get_input_ = 0x4B38F8;
const DWORD gmouse_is_scrolling_ = 0x44359C;
const DWORD gmovie_play_ = 0x446520;
const DWORD gsnd_build_weapon_sfx_name_ = 0x4494F0;
const DWORD gsound_play_sfx_file_ = 0x449734;
const DWORD intface_redraw_ = 0x454988;
const DWORD intface_toggle_item_state_ = 0x455088;
const DWORD intface_update_hit_points_ = 0x454A08;
const DWORD intface_update_items_ = 0x454D78;
const DWORD intface_update_move_points_ = 0x454C34;
const DWORD intface_use_item_ = 0x455180;
const DWORD inven_display_msg_ = 0x4663D4;
const DWORD inven_left_hand_ = 0x465588;
const DWORD inven_right_hand_ = 0x465548;
const DWORD inven_worn_ = 0x4655C8;
const DWORD isPartyMember_ = 0x485F4C;
const DWORD item_add_force_ = 0x46A244;
const DWORD item_c_curr_size_ = 0x46C220;
const DWORD item_c_max_size_ = 0x46C200;
const DWORD item_d_check_addict_ = 0x46CBFC;
const DWORD item_d_take_drug_ = 0x46C4FC;
const DWORD item_get_type_ = 0x46A8B8;
const DWORD item_m_turn_off_ = 0x46C0A0;
const DWORD item_move_all_ = 0x46A64C;
const DWORD item_mp_cost_ = 0x46AD18;
const DWORD item_remove_mult_ = 0x46A418;
const DWORD item_total_weight_ = 0x46AB88;
const DWORD item_w_anim_code_ = 0x46B7FC;
const DWORD item_w_anim_weap_ = 0x46B23C;
const DWORD item_w_is_2handed_ = 0x46B1CC;
const DWORD item_w_try_reload_ = 0x46B38C;
const DWORD item_weight_ = 0x46A90C;
const DWORD ListSkills_ = 0x43042C;
const DWORD LoadSlot_ = 0x47012C;
const DWORD main_game_loop_ = 0x472E98;
const DWORD main_menu_loop_ = 0x47385C;
const DWORD mem_free_ = 0x4AF2A8;
const DWORD message_search_ = 0x476E28;
const DWORD move_inventory_ = 0x467B20;
const DWORD obj_change_fid_ = 0x47CAB4;
const DWORD obj_connect_ = 0x47C020;
const DWORD obj_destroy_ = 0x48B1D0;
const DWORD obj_dist_ = 0x47D8F8;
const DWORD obj_find_first_at_ = 0x47D47C;
const DWORD obj_find_next_at_ = 0x47D500;
const DWORD obj_outline_object_ = 0x47DE34;
const DWORD obj_remove_outline_ = 0x47DE70;
const DWORD obj_turn_off_outline_ = 0x47CF78;
const DWORD obj_turn_on_outline_ = 0x47CF5C;
const DWORD obj_use_book_ = 0x48B220;
const DWORD pc_flag_off_ = 0x428BCC;
const DWORD pc_flag_on_ = 0x428C18;
const DWORD pc_flag_toggle_ = 0x428C8C;
const DWORD perk_level_ = 0x486CF4;
const DWORD perks_dialog_ = 0x436420;
const DWORD PipStatus_ = 0x487A9C;
const DWORD process_bk_ = 0x4B395C;
const DWORD proto_ptr_ = 0x49094C;
const DWORD queue_remove_this_ = 0x490E00;
const DWORD register_begin_ = 0x413584;
const DWORD register_clear_ = 0x413708;
const DWORD register_end_ = 0x413788;
const DWORD register_object_animate_ = 0x414380;
const DWORD RestorePlayer_ = 0x434B40;
const DWORD SaveGame_ = 0x46DE24;
const DWORD SavePlayer_ = 0x434A68;
const DWORD scr_exec_map_update_scripts_ = 0x494F10;
const DWORD scr_write_ScriptNode_ = 0x493D40;
const DWORD skill_dec_point_ = 0x498A98;
const DWORD skill_get_tags_ = 0x498874;
const DWORD skill_inc_point_ = 0x4989F4;
const DWORD skill_level_ = 0x498898;
const DWORD skill_set_tags_ = 0x498850;
const DWORD sprintf_ = 0x4D2B82;
const DWORD stat_get_bonus_ = 0x49CB5C;
const DWORD stat_level_ = 0x49C9D8;
const DWORD stat_pc_add_experience_ = 0x49D14C;
const DWORD stat_pc_get_ = 0x49CFE4;
const DWORD stat_pc_min_exp_ = 0x49D06C;
const DWORD switch_hand_ = 0x46505C;
const DWORD text_font_ = 0x4C1ECC;
const DWORD tile_refresh_display_ = 0x49E3CC;
const DWORD tile_scroll_to_ = 0x4A08D4;
const DWORD trait_get_ = 0x4A0B04;
const DWORD trait_set_ = 0x4A0AF8;
const DWORD win_draw_ = 0x4C3548;
const DWORD win_draw_rect_ = 0x4C356C;
const DWORD win_get_buf_ = 0x4C3E98;
const DWORD win_print_ = 0x4C2E38;
const DWORD win_register_button_ = 0x4C4850;
