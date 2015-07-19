/*
* sfall
* Copyright (C) 2008-2015 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

extern const DWORD action_get_an_object_;
extern const DWORD action_loot_container_;
extern const DWORD action_use_an_item_on_object_;
extern const DWORD AddHotLines_;
extern const DWORD art_exists_;
extern const DWORD art_id_;
extern const DWORD art_ptr_lock_data_;
extern const DWORD art_ptr_unlock_;
extern const DWORD buf_to_buf_;
extern const DWORD combat_ai_;
extern const DWORD combat_is_shot_blocked_;
extern const DWORD combat_should_end_;
extern const DWORD combat_turn_;
extern const DWORD config_set_value_;
extern const DWORD credits_;
extern const DWORD credits_get_next_line_;
extern const DWORD critter_body_type_;
extern const DWORD critter_is_dead_;
extern const DWORD critter_name_;
extern const DWORD critter_pc_set_name_;
extern const DWORD db_fclose_;
extern const DWORD debug_register_env_;
extern const DWORD dialog_out_;
extern const DWORD display_inventory_;
extern const DWORD display_print_;
extern const DWORD display_table_inventories_;
extern const DWORD display_target_inventory_;
extern const DWORD DOSCmdLineDestroy_;
extern const DWORD endgame_slideshow_;
extern const DWORD game_get_global_var_;
extern const DWORD game_set_global_var_;
extern const DWORD get_input_;
extern const DWORD gmouse_is_scrolling_;
extern const DWORD gmovie_play_;
extern const DWORD gsnd_build_weapon_sfx_name_;
extern const DWORD gsound_play_sfx_file_;
extern const DWORD intface_redraw_;
extern const DWORD intface_toggle_item_state_;
extern const DWORD intface_update_hit_points_;
extern const DWORD intface_update_items_;
extern const DWORD intface_update_move_points_;
extern const DWORD intface_use_item_;
extern const DWORD inven_display_msg_;
extern const DWORD inven_left_hand_;
extern const DWORD inven_right_hand_;
extern const DWORD inven_worn_;
extern const DWORD isPartyMember_;
extern const DWORD item_add_force_;
extern const DWORD item_c_curr_size_;
extern const DWORD item_c_max_size_;
extern const DWORD item_d_check_addict_;
extern const DWORD item_d_take_drug_;
extern const DWORD item_get_type_;
extern const DWORD item_m_turn_off_;
extern const DWORD item_move_all_;
extern const DWORD item_mp_cost_;
extern const DWORD item_remove_mult_;
extern const DWORD item_total_weight_;
extern const DWORD item_w_anim_code_;
extern const DWORD item_w_anim_weap_;
extern const DWORD item_w_is_2handed_;
extern const DWORD item_w_try_reload_;
extern const DWORD item_weight_;
extern const DWORD ListSkills_;
extern const DWORD LoadSlot_;
extern const DWORD main_game_loop_;
extern const DWORD main_menu_loop_;
extern const DWORD mem_free_;
extern const DWORD message_search_;
extern const DWORD move_inventory_;
extern const DWORD obj_change_fid_;
extern const DWORD obj_connect_;
extern const DWORD obj_destroy_;
extern const DWORD obj_dist_;
extern const DWORD obj_find_first_at_;
extern const DWORD obj_find_next_at_;
extern const DWORD obj_outline_object_;
extern const DWORD obj_remove_outline_;
extern const DWORD obj_turn_off_outline_;
extern const DWORD obj_turn_on_outline_;
extern const DWORD obj_use_book_;
extern const DWORD pc_flag_off_;
extern const DWORD pc_flag_on_;
extern const DWORD pc_flag_toggle_;
extern const DWORD perk_level_;
extern const DWORD perks_dialog_;
extern const DWORD PipStatus_;
extern const DWORD process_bk_;
extern const DWORD proto_ptr_;
extern const DWORD queue_clear_type_;
extern const DWORD queue_find_;
extern const DWORD queue_remove_this_;
extern const DWORD register_begin_;
extern const DWORD register_clear_;
extern const DWORD register_end_;
extern const DWORD register_object_animate_;
extern const DWORD RestorePlayer_;
extern const DWORD SaveGame_;
extern const DWORD SavePlayer_;
extern const DWORD scr_exec_map_update_scripts_;
extern const DWORD scr_write_ScriptNode_;
extern const DWORD skill_dec_point_;
extern const DWORD skill_get_tags_;
extern const DWORD skill_inc_point_;
extern const DWORD skill_level_;
extern const DWORD skill_set_tags_;
extern const DWORD sprintf_;
extern const DWORD stat_get_bonus_;
extern const DWORD stat_level_;
extern const DWORD stat_pc_add_experience_;
extern const DWORD stat_pc_get_;
extern const DWORD stat_pc_min_exp_;
extern const DWORD switch_hand_;
extern const DWORD text_font_;
extern const DWORD tile_refresh_display_;
extern const DWORD tile_scroll_to_;
extern const DWORD trait_get_;
extern const DWORD trait_set_;
extern const DWORD win_draw_;
extern const DWORD win_draw_rect_;
extern const DWORD win_get_buf_;
extern const DWORD win_print_;
extern const DWORD win_register_button_;

#define _barter_back_win         0x59CF04
#define _btable                  0x59CEF8
#define _combat_free_move        0x56BCBC
#define _combat_highlight        0x56BCA4
#define _combat_list             0x56BCAC
#define _combat_state            0x4FED78
#define _combat_turn_running     0x4FED74
#define _crit_succ_eff           0x4FEDA8
#define _crnt_func               0x662C34
#define _curr_font_num           0x53A2F4
#define _curr_pc_stat            0x66521C
#define _curr_stack              0x59CEE8
#define _drug_pid                0x5058B4
#define _Educated                0x56E7D8
#define _Experience_             0x665224
#define _flptr                   0x612D70
#define _free_perk               0x56ED4D
#define _game_global_vars        0x5050BC
#define _game_user_wants_to_quit 0x5050C8
#define _gIsSteal                0x507EAC
#define _GreenColor              0x6A3F00
#define _holo_flag               0x662CC5
#define _hot_line_count          0x662C20
#define _i_wid                   0x59CF18
#define _intfaceEnabled          0x505604
#define _interfaceWindow         0x505700
#define _inven_dude              0x505734
#define _inven_pid               0x505738
#define _itemButtonItems         0x5956A0
#define _itemCurrentItem         0x50566C
#define _kb_lock_flags           0x539F0A
#define _last_level              0x56ECAC
#define _Level_                  0x665220
#define _Lifegiver               0x56E800
#define _list_com                0x56BCB0
#define _list_total              0x56BCA8
#define _LSData                  0x6122D0
#define _main_window             0x505B78
#define _map_elevation           0x505BEC
#define _Mutate_                 0x56E860
#define _name_color              0x56BF04
#define _name_font               0x56BF0C
#define _obj_dude                0x65F638
#define _outlined_object         0x5054E4
#define _pc_name                 0x56BF1C
#define _perk_lev                0x662984
#define _proto_main_msg_file     0x662DB8
#define _ptable                  0x59CEE4
#define _pud                     0x59CE50
#define _queue                   0x662F6C
#define _quick_done              0x505A54
#define _RedColor                0x6AB720
#define _slot_cursor             0x505A50
#define _sneak_working           0x56BF3C
#define _stack                   0x59CDEC
#define _stack_offset            0x59CD94
#define _sthreads                0x507318
#define _Tag_                    0x56E85C
#define _tag_skill               0x665010
#define _target_curr_stack       0x59CEFC
#define _target_pud              0x59CEF4
#define _target_stack            0x59CE14
#define _target_stack_offset     0x59CDBC
#define _text_char_width         0x53A308
#define _text_height             0x53A300
#define _text_to_buf             0x53A2FC
#define _title_color             0x56BF10
#define _title_font              0x56BF08
