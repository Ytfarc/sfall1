#pragma once

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

#define PID_ACTIVE_GEIGER_COUNTER 207
#define PID_ACTIVE_STEALTH_BOY    210
#define PID_SUPER_STIMPAK         144

// Perk Trait Types
#define PERK_bonus_awareness     (0)
#define PERK_bonus_hth_attacks   (1)
#define PERK_bonus_hth_damage    (2)
#define PERK_bonus_move          (3)
#define PERK_bonus_ranged_damage (4)
#define PERK_bonus_rate_of_fire  (5)
#define PERK_earlier_sequence    (6)
#define PERK_faster_healing      (7)
#define PERK_more_criticals      (8)
#define PERK_night_vision        (9)
#define PERK_presence            (10)
#define PERK_rad_resistance      (11)
#define PERK_toughness           (12)
#define PERK_strong_back         (13)
#define PERK_sharpshooter        (14)
#define PERK_silent_running      (15)
#define PERK_survivalist         (16)
#define PERK_master_trader       (17)
#define PERK_educated            (18)
#define PERK_healer              (19)
#define PERK_fortune_finder      (20)
#define PERK_better_criticals    (21)
#define PERK_empathy             (22)
#define PERK_slayer              (23)
#define PERK_sniper              (24)
#define PERK_silent_death        (25)
#define PERK_action_boy          (26)
#define PERK_mental_block        (27)
#define PERK_lifegiver           (28)
#define PERK_dodger              (29)
#define PERK_snakeater           (30)
#define PERK_mr_fixit            (31)
#define PERK_medic               (32)
#define PERK_master_thief        (33)
#define PERK_speaker             (34)
#define PERK_heave_ho            (35)
#define PERK_friendly_foe        (36)
#define PERK_pickpocket          (37)
#define PERK_ghost               (38)
#define PERK_cult_of_personality (39)
#define PERK_scrounger           (40)
#define PERK_explorer            (41)
#define PERK_flower_child        (42)
#define PERK_pathfinder          (43)
#define PERK_animal_friend       (44)
#define PERK_scout               (45)
#define PERK_mysterious_stranger (46)
#define PERK_ranger              (47)
#define PERK_quick_pockets       (48)
#define PERK_smooth_talker       (49)
#define PERK_swift_learner       (50)
#define PERK_tag                 (51)
#define PERK_mutate              (52)
#define PERK_add_nuka            (53)
#define PERK_add_buffout         (54)
#define PERK_add_mentats         (55)
#define PERK_add_psycho          (56)
#define PERK_add_radaway         (57)
#define PERK_weapon_long_range   (58)
#define PERK_weapon_accurate     (59)
#define PERK_weapon_penetrate    (60)
#define PERK_weapon_knockback    (61)
#define PERK_armor_powered       (62)
#define PERK_armor_combat        (63)
#define PERK_count               (64)

// Trait Types
#define TRAIT_fast_metabolism (0)
#define TRAIT_bruiser         (1)
#define TRAIT_small_frame     (2)
#define TRAIT_one_hander      (3)
#define TRAIT_finesse         (4)
#define TRAIT_kamikaze        (5)
#define TRAIT_heavy_handed    (6)
#define TRAIT_fast_shot       (7)
#define TRAIT_bloody_mess     (8)
#define TRAIT_jinxed          (9)
#define TRAIT_good_natured    (10)
#define TRAIT_drug_addict     (11)
#define TRAIT_drug_resistant  (12)
#define TRAIT_sex_appeal      (13)
#define TRAIT_night_person    (14)
#define TRAIT_gifted          (15)
#define TRAIT_count           (16)

// proto.h: stats //
// SPECIAL System stats
#define STAT_st                    (0)
#define STAT_pe                    (1)
#define STAT_en                    (2)
#define STAT_ch                    (3)
#define STAT_iq                    (4)
#define STAT_ag                    (5)
#define STAT_lu                    (6)
#define STAT_max_hit_points        (7)
#define STAT_max_move_points       (8)
#define STAT_ac                    (9)
#define STAT_unused                (10)
#define STAT_melee_dmg             (11)
#define STAT_carry_amt             (12)
#define STAT_sequence              (13)
#define STAT_heal_rate             (14)
#define STAT_crit_chance           (15)
#define STAT_better_crit           (16)
#define STAT_dmg_thresh            (17)
#define STAT_dmg_thresh_laser      (18)
#define STAT_dmg_thresh_fire       (19)
#define STAT_dmg_thresh_plasma     (20)
#define STAT_dmg_thresh_electrical (21)
#define STAT_dmg_thresh_emp        (22)
#define STAT_dmg_thresh_explosion  (23)
#define STAT_dmg_resist            (24)
#define STAT_dmg_resist_laser      (25)
#define STAT_dmg_resist_fire       (26)
#define STAT_dmg_resist_plasma     (27)
#define STAT_dmg_resist_electrical (28)
#define STAT_dmg_resist_emp        (29)
#define STAT_dmg_resist_explosion  (30)
#define STAT_rad_resist            (31)
#define STAT_poison_resist         (32)
#define STAT_max_derived           STAT_poison_resist
#define STAT_age                   (33)
#define STAT_gender                (34)
#define STAT_current_hp            (35)
#define STAT_current_poison        (36)
#define STAT_current_rad           (37)
#define STAT_real_max_stat         (38)
// extra stat-like values that are treated specially
#define STAT_max_stat              STAT_current_hp

#define PCSTAT_unspent_skill_points (0)
#define PCSTAT_level                (1)
#define PCSTAT_experience           (2)
#define PCSTAT_reputation           (3)
#define PCSTAT_karma                (4)
#define PCSTAT_max_pc_stat          (5)

#define SKILL_SMALL_GUNS     (0)
#define SKILL_BIG_GUNS       (1)
#define SKILL_ENERGY_WEAPONS (2)
#define SKILL_UNARMED_COMBAT (3)
#define SKILL_MELEE          (4)
#define SKILL_THROWING       (5)
#define SKILL_FIRST_AID      (6)
#define SKILL_DOCTOR         (7)
#define SKILL_SNEAK          (8)
#define SKILL_LOCKPICK       (9)
#define SKILL_STEAL          (10)
#define SKILL_TRAPS          (11)
#define SKILL_SCIENCE        (12)
#define SKILL_REPAIR         (13)
#define SKILL_CONVERSANT     (14)
#define SKILL_BARTER         (15)
#define SKILL_GAMBLING       (16)
#define SKILL_OUTDOORSMAN    (17)
#define SKILL_count          (18)

#define item_type_armor     (0)
#define item_type_container (1)
#define item_type_drug      (2)
#define item_type_weapon    (3)
#define item_type_ammo      (4)
#define item_type_misc_item (5)
#define item_type_key       (6)

#define ObjType_Item     (0)
#define ObjType_Critter  (1)
#define ObjType_Scenery  (2)
#define ObjType_Wall     (3)
#define ObjType_Tile     (4)
#define ObjType_Misc     (5)
#define ObjType_Intrface (6)

#define hit_left_weapon_primary    (0)
#define hit_left_weapon_secondary  (1)
#define hit_right_weapon_primary   (2)
#define hit_right_weapon_secondary (3)
#define hit_punch                  (4)
#define hit_kick                   (5)
#define hit_left_weapon_reload     (6)
#define hit_right_weapon_reload    (7)

#define ROLL_CRITICAL_FAILURE (0)
#define ROLL_FAILURE          (1)
#define ROLL_SUCCESS          (2)
#define ROLL_CRITICAL_SUCCESS (3)
