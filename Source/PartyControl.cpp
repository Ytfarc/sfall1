/*
 *    sfall
 *    Copyright (C) 2013  The sfall team
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

#include "Define.h"
#include "FalloutEngine.h"
#include "Logging.h"
#include "PartyControl.h"
#include <vector>

DWORD IsControllingNPC = 0;
DWORD HiddenArmor = 0;

static DWORD Mode;
static std::vector<WORD> Chars;

static char real_pc_name[32];
static DWORD real_last_level;
static DWORD real_Level;
static DWORD real_Experience;
static DWORD real_perk_lev[PERK_count];
static DWORD real_free_perk;
static DWORD real_unspent_skill_points;
static DWORD real_map_elevation;
static DWORD real_sneak_working;
static DWORD real_dude;
static DWORD real_hand;
static DWORD real_tag_skill[4];
static DWORD real_trait;
static DWORD real_trait2;
static DWORD real_itemButtonItems[(6*4)*2];
static DWORD real_drug_gvar[6];

static void __declspec(naked) CanUseWeapon() {
 __asm {
  push edi
  push esi
  push edx
  push ecx
  push ebx
  mov  edi, eax
  call item_get_type_
  cmp  eax, 3                               // Это item_type_weapon?
  jne  canUse                               // Нет
  mov  eax, edi                             // eax=item
  mov  edx, 2                               // hit_right_weapon_primary
  call item_w_anim_code_
  xchg ecx, eax                             // ecx=ID1=Weapon code
  xchg edi, eax                             // eax=item
  call item_w_anim_weap_
  xchg ebx, eax                             // ebx=ID2=Animation code
  mov  esi, dword ptr ds:[_inven_dude]      // _inven_dude
  mov  edx, dword ptr [esi+0x20]            // fid
  and  edx, 0xFFF                           // edx=Index
  mov  eax, dword ptr [esi+0x1C]            // cur_rot
  inc  eax
  push eax                                  // ID3=Direction code
  mov  eax, 1                               // ObjType_Critter
  call art_id_
  call art_exists_
  test eax, eax
  jz   end
canUse:
  xor  eax, eax
  inc  eax
end:
  pop  ebx
  pop  ecx
  pop  edx
  pop  esi
  pop  edi
  retn
 }
}

static bool _stdcall IsInPidList(DWORD* npc) {
 if (Chars.size() == 0) return true;
 int pid = npc[0x64/4] & 0xFFFFFF;
 for (std::vector<WORD>::iterator it = Chars.begin(); it != Chars.end(); it++) {
  if (*it == pid) {
   return true;
  }
 }
 return false;
}

// save "real" dude state
static void __declspec(naked) SaveDudeState() {
 __asm {
  push edi
  push esi
  push edx
  push ecx
  push ebx
  mov  esi, _pc_name
  mov  edi, offset real_pc_name
  mov  ecx, 32/4
  rep  movsd
  mov  eax, ebx
  call critter_name_
  call critter_pc_set_name_
  mov  eax, dword ptr ds:[_last_level]
  mov  real_last_level, eax
  mov  eax, dword ptr ds:[_Level_]
  mov  real_Level, eax
  mov  eax, dword ptr ds:[_Experience_]
  mov  real_Experience, eax
  movzx eax, byte ptr ds:[_free_perk]
  mov  real_free_perk, eax
  mov  eax, dword ptr ds:[_curr_pc_stat]
  mov  real_unspent_skill_points, eax
  mov  eax, dword ptr ds:[_map_elevation]
  mov  real_map_elevation, eax
  mov  eax, dword ptr ds:[_obj_dude]
  mov  real_dude, eax
  mov  eax, dword ptr [ebx+0x64]
  mov  dword ptr ds:[_inven_pid], eax
  mov  dword ptr ds:[_obj_dude], ebx
  mov  dword ptr ds:[_inven_dude], ebx
  mov  eax, dword ptr ds:[_itemCurrentItem]
  mov  real_hand, eax
  mov  eax, dword ptr ds:[_sneak_working]
  mov  real_sneak_working, eax
  mov  edx, offset real_trait2
  mov  eax, offset real_trait
  call trait_get_
  mov  esi, _itemButtonItems
  mov  edi, offset real_itemButtonItems
  mov  ecx, (6*4)*2
  rep  movsd
  mov  esi, _perk_lev
  push esi
  mov  edi, offset real_perk_lev
  mov  ecx, PERK_count
  push ecx
  rep  movsd
  xchg ecx, eax
  pop  ecx
  pop  edi
  rep  stosd
  mov  dword ptr ds:[_last_level], eax
  mov  dword ptr ds:[_Level_], eax
  mov  byte ptr ds:[_free_perk], al
  mov  dword ptr ds:[_curr_pc_stat], eax
  mov  dword ptr ds:[_sneak_working], eax
  mov  esi, dword ptr ds:[_game_global_vars]
  add  esi, 189*4                           // esi->NUKA_COLA_ADDICT
  push esi
  mov  edi, offset real_drug_gvar
  mov  ecx, 6
  push ecx
  rep  movsd
  pop  ecx
  pop  edi
  mov  edx, _drug_pid
  mov  esi, ebx                             // _obj_dude
loopDrug:
  mov  eax, dword ptr [edx]                 // eax = drug_pid
  call item_d_check_addict_
  test eax, eax                             // Есть зависимость?
  jz   noAddict                             // Нет
  xor  eax, eax
  inc  eax
noAddict:
  mov  dword ptr [edi], eax
  add  edx, 4
  add  edi, 4
  loop loopDrug
  test eax, eax                             // Есть зависимость к алкоголю (пиво)?
  jnz  skipBooze                            // Да
  mov  eax, dword ptr [edx]                 // PID_BOOZE
  call item_d_check_addict_
  mov  dword ptr [edi-4], eax
skipBooze:
  mov  ecx, ebx                              // eax = _obj_dude
  xor  eax, eax
  dec  eax
  call item_d_check_addict_
  mov  edx, 4
  test eax, eax
  mov  eax, edx
  jz   unsetAddict
  call pc_flag_on_
  jmp  setAddict
unsetAddict:
  call pc_flag_off_
setAddict:
  push edx
  mov  eax, offset real_tag_skill
  call skill_get_tags_
  mov  edi, _tag_skill
  pop  ecx
  xor  eax, eax
  dec  eax
  rep  stosd
  mov  edx, eax
  call trait_set_
  mov  dword ptr ds:[_Experience_], eax
// get active hand by weapon anim code
  mov  edx, dword ptr [ebx+0x20]            // fid
  and  edx, 0x0F000
  sar  edx, 0xC                             // edx = current weapon anim code as seen in hands
  xor  ecx, ecx                             // Левая рука
  mov  eax, ebx
  call inven_right_hand_
  test eax, eax                             // Есть вещь в правой руке?
  jz   setActiveHand                        // Нет
  push eax
  call item_get_type_
  cmp  eax, item_type_weapon                // Это оружие?
  pop  eax
  jne  setActiveHand                        // Нет
  call item_w_anim_code_
  cmp  eax, edx                             // Анимация одинаковая?
  jne  setActiveHand                        // Нет
  inc  ecx                                  // Правая рука
setActiveHand:
  mov  dword ptr ds:[_itemCurrentItem], ecx
  mov  eax, ebx
  call inven_right_hand_
  test eax, eax                             // Есть вещь в правой руке?
  jz   noRightHand                          // Нет
  push eax
  call CanUseWeapon
  test eax, eax
  pop  eax
  jnz  noRightHand
  and  byte ptr [eax+0x27], 0xFD            // Сбрасываем флаг вещи в правой руке
noRightHand:
  xchg ebx, eax
  call inven_left_hand_
  test eax, eax                             // Есть вещь в левой руке?
  jz   noLeftHand                           // Нет
  push eax
  call CanUseWeapon
  test eax, eax
  pop  eax
  jnz  noLeftHand
  and  byte ptr [eax+0x27], 0xFE            // Сбрасываем флаг вещи в левой руке
noLeftHand:
  pop  ebx
  pop  ecx
  pop  edx
  pop  esi
  pop  edi
  retn
 }
}

// restore dude state
static void __declspec(naked) RestoreDudeState() {
 __asm {
  push edi
  push esi
  push edx
  push ecx
  push eax
  mov  eax, offset real_pc_name
  call critter_pc_set_name_
  mov  eax, real_last_level
  mov  dword ptr ds:[_last_level], eax
  mov  eax, real_Level
  mov  dword ptr ds:[_Level_], eax
  mov  eax, real_map_elevation
  mov  dword ptr ds:[_map_elevation], eax
  mov  eax, real_Experience
  mov  dword ptr ds:[_Experience_], eax
  mov  esi, offset real_drug_gvar
  mov  edi, dword ptr ds:[_game_global_vars]
  add  edi, 189*4                           // esi->NUKA_COLA_ADDICT
  mov  ecx, 6
  rep  movsd
  mov  esi, offset real_perk_lev
  mov  edi, _perk_lev
  mov  ecx, PERK_count
  rep  movsd
  mov  esi, offset real_itemButtonItems
  mov  edi, _itemButtonItems
  mov  ecx, (6*4)*2
  rep  movsd
  mov  eax, real_free_perk
  mov  byte ptr ds:[_free_perk], al
  mov  eax, real_unspent_skill_points
  mov  dword ptr ds:[_curr_pc_stat], eax
  mov  edx, 4
  mov  eax, offset real_tag_skill
  call skill_set_tags_
  mov  edx, real_trait2
  mov  eax, real_trait
  call trait_set_
  mov  ecx, real_dude
  mov  dword ptr ds:[_obj_dude], ecx
  mov  dword ptr ds:[_inven_dude], ecx
  mov  eax, dword ptr [ecx+0x64]
  mov  dword ptr ds:[_inven_pid], eax
  mov  eax, real_hand
  mov  dword ptr ds:[_itemCurrentItem], eax
  mov  eax, real_sneak_working
  mov  dword ptr ds:[_sneak_working], eax
  xor  eax, eax
  mov  IsControllingNPC, eax
  dec  eax
  call item_d_check_addict_
  test eax, eax
  mov  eax, 4
  jz   unsetAddict
  call pc_flag_on_
  jmp  setAddict
unsetAddict:
  call pc_flag_off_
setAddict:
  pop  eax
  pop  ecx
  pop  edx
  pop  esi
  pop  edi
  retn
 }
}

static void _declspec(naked) CombatWrapper_v2() {
 __asm {
  pushad
  cmp  eax, dword ptr ds:[_obj_dude]        // _obj_dude
  jne  skip
  xor  edx, edx
  cmp  _combatNumTurns, edx
  je   skipControl                          // Это первый ход
  mov  eax, dword ptr [eax+0x4]             // tile_num
  add  edx, 2
  call tile_scroll_to_
  jmp  skipControl
skip:
  push eax
  call IsInPidList
  and  eax, 0xFF
  test eax, eax
  jz   skipControl
  cmp  Mode, eax                            // control all critters?
  je   npcControl
  popad
  pushad
  call isPartyMember_
  test eax, eax
  jnz  npcControl
skipControl:
  popad
  jmp  combat_turn_
npcControl:
  mov  IsControllingNPC, eax
  popad
  pushad
  xchg ebx, eax                             // ebx = npc
  call SaveDudeState
  call intface_redraw_
  mov  eax, dword ptr [ebx+0x4]             // tile_num
  mov  edx, 2
  call tile_scroll_to_
  xchg ebx, eax                             // eax = npc
  call combat_turn_
  xchg ecx, eax
  cmp  IsControllingNPC, 0                  // if game was loaded during turn, PartyControlReset()
  je   skipRestore                          // was called and already restored state
  call RestoreDudeState
  call intface_redraw_
skipRestore:
  test ecx, ecx                             // Нормальное завершение хода?
  popad
  jz   end                                  // Да
// выход/загрузка/побег/смерть
  test byte ptr [eax+0x44], 0x80            // DAM_DEAD
  jnz  end
  xor  eax, eax
  dec  eax
  retn
end:
  xor  eax, eax
  retn
 }
}

// hack to exit from this function safely when you load game during NPC turn
static void _declspec(naked) combat_add_noncoms_hook() {
 __asm {
  call CombatWrapper_v2
  inc  eax
  test eax, eax
  jnz  end
  mov  dword ptr ds:[_list_com], eax
  mov  ecx, ebp
end:
  retn
 }
}

static void _declspec(naked) stat_pc_min_exp_hook() {
 __asm {
  xor  eax, eax
  cmp  IsControllingNPC, eax
  je   end
  dec  eax
  retn
end:  
  jmp  stat_pc_min_exp_
 }
}

static void _declspec(naked) inven_pickup_hook() {
 __asm {
  call item_get_type_
  test eax, eax                             // Это item_type_armor?
  jnz  end                                  // Нет
  cmp  IsControllingNPC, eax
  je   end
  dec  eax
end:
  retn
 }
}

static void _declspec(naked) handle_inventory_hook() {
 __asm {
  mov  edx, eax                             // edx=_inven_dude
  call inven_worn_
  test eax, eax
  jz   end
  cmp  IsControllingNPC, 0
  je   end
  mov  HiddenArmor, eax
  pushad
  push edx
  mov  ebx, 1
  xchg edx, eax
  call item_remove_mult_
  pop  edx
nextArmor:
  mov  eax, edx
  call inven_worn_
  test eax, eax
  jz   noArmor
  and  byte ptr [eax+0x27], 0xFB            // Сбрасываем флаг одетой брони
  jmp  nextArmor
noArmor:
  popad
end:
  retn
 }
}

static void _declspec(naked) handle_inventory_hook1() {
 __asm {
  cmp  IsControllingNPC, 0
  je   end
  pushad
  mov  edx, HiddenArmor
  test edx, edx
  jz   skip
  or   byte ptr [edx+0x27], 4               // Устанавливаем флаг одетой брони
  mov  ebx, 1
  call item_add_force_
  xor  edx, edx
skip:
  mov  HiddenArmor, edx
  popad
end:
  jmp  inven_worn_
 }
}

static const DWORD switch_hand_hook_End = 0x46516F;
static void _declspec(naked) switch_hand_hook() {
 __asm {
  cmp  IsControllingNPC, 0
  je   end
  call CanUseWeapon
  test eax, eax
  jnz  end
  pop  eax                                  // Уничтожаем адрес возврата
  jmp  switch_hand_hook_End
end:  
  mov  esi, ebx
  cmp  dword ptr [edx], 0
  retn
 }
}

static void _declspec(naked) combat_input_hook() {
 __asm {
  xor  ebx, ebx
  cmp  IsControllingNPC, ebx
  je   end
  cmp  eax, 0xD                             // Enter (завершение боя)?
  jne  end                                  // Нет
  mov  eax, 0x20                            // Space (окончание хода)
end:
  mov  ebx, eax
  cmp  eax, 0x20                            // Space (окончание хода)?
  retn
 }
}

static const DWORD action_skill_use_hook_End = 0x412365;
static void __declspec(naked) action_skill_use_hook() {
 __asm {
  cmp  eax, SKILL_SNEAK
  jne  end
  xor  eax, eax
  cmp  IsControllingNPC, eax
  jne  end
  retn
end:
  pop  eax                                  // Уничтожаем адрес возврата
  jmp  action_skill_use_hook_End
 }
}

static void __declspec(naked) action_use_skill_on_hook() {
 __asm {
  cmp  IsControllingNPC, eax
  jne  end
  call pc_flag_toggle_
end:
  retn
 }
}

void PartyControlInit() {
 Mode = GetPrivateProfileIntA("Misc", "ControlCombat", 0, ini);
 if (Mode == 1 || Mode == 2) {
  char pidbuf[512];
  pidbuf[511]=0;
  if (GetPrivateProfileStringA("Misc", "ControlCombatPIDList", "", pidbuf, 511, ini)) {
   char* ptr = pidbuf;
   char* comma;
   while (true) {
    comma = strchr(ptr, ',');
    if (!comma) 
     break;
    *comma = 0;
    if (strlen(ptr) > 0)
     Chars.push_back((WORD)strtoul(ptr, 0, 0));
    ptr = comma + 1;
   }
   if (strlen(ptr) > 0)
    Chars.push_back((WORD)strtoul(ptr, 0, 0));
  }
  dlog_f(" Mode %d, Chars read: %d.", DL_INIT, Mode, Chars.size());
  HookCall(0x4203A1, &combat_add_noncoms_hook);
  HookCall(0x420DB7, &CombatWrapper_v2);
  HookCall(0x42EDF7, &stat_pc_min_exp_hook);// PrintLevelWin_
  HookCall(0x433A4B, &stat_pc_min_exp_hook);// Save_as_ASCII_
  HookCall(0x464F4D, &inven_pickup_hook);
  HookCall(0x4629E3, &handle_inventory_hook);
  HookCall(0x462BFB, &handle_inventory_hook1);
  MakeCall(0x465063, &switch_hand_hook, false);
  SafeWrite32(0x4657B0, 152);               // Ширина текста 152, а не 80 
  MakeCall(0x420883, &combat_input_hook, false);
  MakeCall(0x41234C, &action_skill_use_hook, false);
  HookCall(0x412603, &action_use_skill_on_hook);
 } else dlog(" Disabled.", DL_INIT);
}

void __stdcall PartyControlReset() {
 __asm {
  cmp  IsControllingNPC, 0
  je   end
  call RestoreDudeState
end:
 }
}
