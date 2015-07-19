/*
 *    sfall
 *    Copyright (C) 2011  Timeslip
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

#include "Bugs.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "Input.h"
#include "Inventory.h"
#include "PartyControl.h"

static DWORD ReloadWeaponKey = 0;

static void __declspec(naked) ReloadActiveHand() {
 __asm {
// esi=-1 если не перезарядили неактивную руку или смещение неактивной руки
  push ebx
  push ecx
  push edx
  mov  eax, ds:[_itemCurrentItem]
  imul ebx, eax, 24
  mov  eax, ds:[_obj_dude]
  xor  ecx, ecx
reloadItem:
  push eax
  mov  edx, ds:[_itemButtonItems][ebx]
  call item_w_try_reload_
  test eax, eax
  pop  eax
  jnz  endReloadItem
  inc  ecx
  jmp  reloadItem
endReloadItem:
  cmp  dword ptr ds:[_itemButtonItems + 0x10][ebx], 5// mode
  jne  skip_toggle_item_state
  call intface_toggle_item_state_
skip_toggle_item_state:
  test ecx, ecx
  jnz  useActiveHand
  xchg esi, ebx
useActiveHand:
  push ebx
  xor  esi, esi
  dec  esi
  xor  eax, eax
  call intface_update_items_
  pop  eax
  cmp  eax, esi
  je   end
  mov  ebx, 2
  xor  ecx, ecx
  mov  edx, ds:[_itemButtonItems][eax]
  mov  eax, 0x4565A2
  jmp  eax
end:
  pop  edx
  pop  ecx
  pop  ebx
  retn
 }
}

static void __declspec(naked) ReloadWeaponHotKey() {
 __asm {
  call gmouse_is_scrolling_
  test eax, eax
  jnz  end
  pushad
  xchg ebx, eax
  push ReloadWeaponKey
  call KeyDown
  test eax, eax
  jnz  ourKey
  popad
  retn
ourKey:
  cmp  dword ptr ds:[_intfaceEnabled], ebx
  je   endReload
  xor  esi, esi
  dec  esi
  cmp  dword ptr ds:[_interfaceWindow], esi
  je   endReload
  mov  edx, ds:[_itemCurrentItem]
  imul eax, edx, 24
  cmp  byte ptr ds:[_itemButtonItems + 0x5][eax], bl// itsWeapon
  jne  itsWeapon                            // Да
  call intface_use_item_
  jmp  endReload
itsWeapon:
  test byte ptr ds:[_combat_state], 1
  jnz  inCombat
  call ReloadActiveHand
  jmp  endReload
inCombat:
//  xor  ebx, ebx                             // is_secondary
  add  edx, hit_left_weapon_reload          // edx = 6/7 - перезарядка оружия в левой/правой руке
  mov  eax, ds:[_obj_dude]
  push eax
  call item_mp_cost_
  xchg ecx, eax
  pop  eax                                  // _obj_dude
  mov  edx, [eax+0x40]                      // curr_mp
  cmp  ecx, edx
  jg   endReload
  push eax
  call ReloadActiveHand
  test eax, eax
  pop  eax                                  // _obj_dude
  jnz  endReload
  sub  edx, ecx
  mov  [eax+0x40], edx                      // curr_mp
  xchg edx, eax
  call intface_update_move_points_
endReload:
  popad
  inc  eax
end:
  retn
 }
}

static void __declspec(naked) AutoReloadWeapon() {
 __asm {
  call scr_exec_map_update_scripts_
  pushad
  cmp  dword ptr ds:[_game_user_wants_to_quit], 0
  jnz  end
  mov  eax, ds:[_obj_dude]
  call critter_is_dead_                     // Дополнительная проверка не помешает
  test eax, eax
  jnz  end
  cmp  dword ptr ds:[_intfaceEnabled], eax
  je   end
  xor  esi, esi
  dec  esi
  cmp  dword ptr ds:[_interfaceWindow], esi
  je   end
  inc  eax
  mov  ebx, ds:[_itemCurrentItem]
  push ebx
  sub  eax, ebx
  mov  ds:[_itemCurrentItem], eax           // Устанавливаем неактивную руку
  imul ebx, eax, 24
  mov  eax, ds:[_obj_dude]
  xor  ecx, ecx
reloadOffhand:
  push eax
  mov  edx, ds:[_itemButtonItems][ebx]
  call item_w_try_reload_
  test eax, eax
  pop  eax
  jnz  endReloadOffhand
  inc  ecx
  jmp  reloadOffhand
endReloadOffhand:
  cmp  dword ptr ds:[_itemButtonItems + 0x10][ebx], 5// mode
  jne  skip_toggle_item_state
  call intface_toggle_item_state_
skip_toggle_item_state:
  test ecx, ecx
  jnz  useOffhand
  xchg ebx, esi
useOffhand:
  xchg esi, ebx                             // esi=-1 если не перезарядили или смещению неактивной руки
  pop  eax
  mov  ds:[_itemCurrentItem], eax           // Восстанавливаем активную руку
  call ReloadActiveHand
end:
  popad
  retn
 }
}

static void __declspec(naked) SetDefaultAmmo() {
 __asm {
  push eax
  push ebx
  push edx
  xchg edx, eax
  mov  ebx, eax
  call item_get_type_
  cmp  eax, item_type_weapon
  jne  end                                  // Нет
  cmp  dword ptr [ebx+0x3C], 0              // Есть патроны в оружии?
  jne  end                                  // Да
  sub  esp, 4
  mov  edx, esp
  mov  eax, [ebx+0x64]                      // eax = pid оружия
  call proto_ptr_
  mov  edx, [esp]
  mov  eax, [edx+0x5C]                      // eax = идентификатор прототипа патронов по умолчанию
  mov  [ebx+0x40], eax                      // прототип используемых патронов
  add  esp, 4
end:
  pop  edx
  pop  ebx
  pop  eax
  retn
 }
}

static void __declspec(naked) inven_action_cursor_hook() {
 __asm {
  mov  edx, [esp+0x1C]
  call SetDefaultAmmo
  cmp  dword ptr [esp+0x18], 0
  mov  eax, 0x466D43
  jmp  eax
 }
}

static void __declspec(naked) item_add_mult_hook() {
 __asm {
  call SetDefaultAmmo
  jmp  item_add_force_
 }
}

static void __declspec(naked) fontHeight() {
 __asm {
  push ebx
  mov  ebx, ds:[_curr_font_num]
  mov  eax, 101
  call text_font_
  call ds:[_text_height]
  xchg ebx, eax
  call text_font_
  xchg ebx, eax
  pop  ebx
  retn
 }
}

static char SizeMsgBuf[32];
static void __declspec(naked) printFreeMaxWeight() {
 __asm {
// ebx = who, ecx = ToWidth, edi = posOffset, esi = extraWeight
  mov  eax, ds:[_curr_font_num]
  push eax
  mov  eax, 101
  call text_font_
  mov  eax, ds:[_i_wid]
  call win_get_buf_                         // eax=ToSurface
  add  edi, eax                             // ToSurface+posOffset (Ypos*ToWidth+Xpos)
  mov  eax, [ebx+0x20]
  and  eax, 0x0F000000
  sar  eax, 0x18
  test eax, eax                             // Это ObjType_Item?
  jz   itsItem                              // Да
  cmp  eax, ObjType_Critter
  jne  noWeight                             // Нет
  mov  eax, ebx
  call item_total_weight_                   // eax = общий вес груза
  xchg ebx, eax                             // ebx = общий вес груза, eax = кто
  mov  edx, STAT_carry_amt
  call stat_level_                          // eax = макс. вес груза
  jmp  print
itsItem:
  mov  eax, ebx
  call item_get_type_
  cmp  eax, item_type_container
  jne  noWeight                             // Нет
  mov  eax, ebx
  call item_c_curr_size_
  xchg ebx, eax
  call item_c_max_size_
print:
  push eax                                  // eax = макс. вес/объём груза
  add  ebx, esi
  sub  eax, ebx                             // eax = свободный вес/размер
  push eax
  xchg ebx, eax
  push 0x4F9288                             // '%d/%d'
  lea  eax, SizeMsgBuf
  push eax
  call sprintf_
  add  esp, 0x10
  movzx eax, byte ptr ds:[_GreenColor]
  cmp  ebx, 0
  jge  noRed
  mov  al, ds:[_RedColor]
noRed:
  push eax
  lea  esi, SizeMsgBuf
  push esi
  xor  edx, edx
nextChar:
  xor  eax, eax
  mov  al, [esi]
  call ds:[_text_char_width]
  inc  eax
  add  edx, eax
  inc  esi
  cmp  byte ptr [esi-1], '/'
  jne  nextChar
  sub  edi, edx
  xchg edi, eax                             // ToSurface+posOffset (Ypos*ToWidth+Xpos)
  mov  ebx, 64                              // TxtWidth
  pop  edx                                  // DisplayText
  call ds:[_text_to_buf]
noWeight:
  pop  eax
  call text_font_
  retn
 }
}

static void __declspec(naked) display_inventory_hook() {
 __asm {
  call fontHeight
  inc  eax
  inc  eax
  add  [esp+0x8], eax                       // height = height + text_height + 1
  call buf_to_buf_
  add  esp, 0x18
  mov  eax, [esp+0x4]
  call art_ptr_unlock_
  pushad
  mov  ebx, ds:[_stack]
  mov  ecx, 537
  mov  edi, 324*537+44+32                   // Xpos=44, Ypos=324, max text width/2=32
  xor  esi, esi
  call printFreeMaxWeight
  popad
  mov  eax, 0x463E88
  jmp  eax
 }
}

static void __declspec(naked) display_target_inventory_hook() {
 __asm {
  call fontHeight
  inc  eax
  inc  eax
  add  [esp+0x8], eax                       // height = height + text_height + 1
  call buf_to_buf_
  add  esp, 0x18
  mov  eax, [esp]
  call art_ptr_unlock_
  pushad
  mov  ebx, ds:[_target_stack]
  mov  ecx, 537
  mov  edi, 324*537+426+32                  // Xpos=426, Ypos=324, max text width/2=32
  mov  esi, WeightOnBody                    // Учитываем вес одетой на цели брони и оружия
  call printFreeMaxWeight
  popad
  mov  eax, 0x464174
  jmp  eax
 }
}

static void __declspec(naked) display_table_inventories_hook() {
 __asm {
  call win_get_buf_
  mov  edi, ds:[_btable]
  mov  [esp+0x6C+4], edi
  mov  edi, ds:[_ptable]
  mov  [esp+0x74+4], edi
  retn
 }
}

static void __declspec(naked) display_table_inventories_hook1() {
 __asm {
  add  dword ptr [esp+8], 20
  sub  dword ptr [esp+16], 20*480
  call buf_to_buf_
  add  esp, 0x18
  pushad
  mov  eax, ds:[_btable]
  call item_total_weight_                   // eax = вес вещей цели в окне бартера
  xchg esi, eax
  mov  ebx, ds:[_stack]
  mov  ecx, 480
  mov  edi, 10*480+169+32                   // Xpos=169, Ypos=10, max text width/2=32
  call printFreeMaxWeight
  popad
  mov  eax, 0x468657
  jmp  eax
 }
}

// Рисуем участок окна
static void __declspec(naked) display_table_inventories_hook2() {
 __asm {
  mov  dword ptr [edx+4], 4                 // WinRect.y_start = 4
  jmp  win_draw_rect_
 }
}

#ifdef TRACE
static void __declspec(naked) display_table_inventories_hook3() {
 __asm {
  add  dword ptr [esp+8], 20
  sub  dword ptr [esp+16], 20*480
  call buf_to_buf_
  add  esp, 0x18
  pushad
  mov  eax, ds:[_ptable]
  call item_total_weight_                   // eax = вес вещей игрока в окне бартера
  xchg esi, eax
  add  esi, WeightOnBody                    // Учитываем вес одетой на цели брони и оружия
  mov  ebx, ds:[_target_stack]
  mov  ecx, 480
  mov  edi, 10*480+254+32                   // Xpos=254, Ypos=10, max text width/2=32
  call printFreeMaxWeight
  popad
  mov  eax, 0x4687E8
  jmp  eax
 }
}
#endif

static void __declspec(naked) barter_inventory_hook() {
 __asm {
  call win_draw_
  xor  ecx, ecx
  dec  ecx
//  mov  ebx, ds:[_btable]
//  mov  edx, ds:[_ptable]
  mov  eax, ds:[_barter_back_win]
  jmp  display_table_inventories_
 }
}

static void __declspec(naked) inven_pickup_hook2() {
 __asm {
  mov  eax, 246                             // x_start
  mov  ebx, 306                             // x_end
//  mov  edx, 37                              // y_start
//  mov  ecx, 137                             // y_end
  push edi
  mov  edi, cs:[0x464D01]
  add  edi, 0x464D05
  call edi                                  // mouse_click_in или процедура из F1_RES
  pop  edi
  test eax, eax
  jz   end
  mov  eax, ds:[_curr_stack]
  xchg edi, eax
  test edi, edi
  jz   useOnPlayer
  mov  ecx, 0x46501B
  jmp  ecx
useOnPlayer:
  cmp  eax, 1006                            // Руки?
  jge  end                                  // Да
  mov  edx, [esp+0x18]                      // item
  mov  eax, edx
  call item_get_type_
  cmp  eax, item_type_drug
  jne  end
  mov  eax, ds:[_stack]
  push eax
  push edx
  call item_d_take_drug_
  pop  edx
  pop  ebx
  cmp  eax, 1
  jne  notUsed
  xchg ebx, eax
  push edx
  push eax
  call item_remove_mult_
  pop  eax
  xor  ecx, ecx
  mov  ebx, [eax+0x28]
  mov  edx, [eax+0x4]
  pop  eax
  push eax
  call obj_connect_
  pop  eax
  call obj_destroy_
notUsed:
  mov  eax, 1
  call intface_update_hit_points_
end:
  mov  eax, 0x465032
  jmp  eax
 }
}

static void __declspec(naked) display_stats_hook() {
 __asm {
  push eax
  mov  edx, STAT_carry_amt
  call stat_level_                          // Макс. груз
  pop  edx
  push eax
  xchg edx, eax                             // edx=Макс. груз
  call item_total_weight_
  xchg edi, eax                             // edi=вес вещей
  xor  eax, eax
  cmp  IsControllingNPC, eax                // Контролируемый персонаж?
  je   skip                                 // Нет
  mov  eax, HiddenArmor
  test eax, eax                             // У него есть броня?
  jz   skip                                 // Нет
  call item_weight_
skip:
  add  edi, eax
  push edi
  sub  edx, edi
  mov  edi, [esp+0x94+4]
  push edi
  push 0x4F92C4                             // '%s %d/%d'
  lea  eax, [esp+0xC+4]
  push eax
  call sprintf_
  add  esp, 5*4
  movzx eax, byte ptr ds:[_GreenColor]
  cmp  edx, 0
  jge  noRed
  mov  al, ds:[_RedColor]
noRed:
  mov  ecx, 499
  mov  ebx, 120
  mov  edx, 0x465F25
  jmp  edx
 }
}

static void __declspec(naked) make_loot_drop_button() {
 __asm {
  cmp  dword ptr [esp+0x4+0x4], 2
  jne  end
  cmp  dword ptr ds:[_gIsSteal], 0
  jne  end
  pushad
  mov  eax, dword ptr ds:[_inven_dude]
  call critter_body_type_
  test eax, eax                             // Это Body_Type_Biped?
  jnz  noDropButton                         // Нет
  xchg ebp, eax
  push ebp
  mov  edx, 265                             // USEGETN.FRM (Action menu use/get normal)
  mov  eax, ObjType_Intrface
  xor  ecx, ecx
  xor  ebx, ebx
  call art_id_
  mov  ecx, 0x59CD84
  xor  ebx, ebx
  xor  edx, edx
  call art_ptr_lock_data_
  test eax, eax
  jz   noLootButton
  xchg esi, eax
  push ebp
  mov  edx, 264                             // USEGETH.FRM (Action menu use/get highlighted)
  mov  eax, ObjType_Intrface
  xor  ecx, ecx
  xor  ebx, ebx
  call art_id_
  mov  ecx, 0x59CD88
  xor  ebx, ebx
  xor  edx, edx
  call art_ptr_lock_data_
  test eax, eax
  jz   noLootButton
  push ebp                                  // ButType
  push ebp
  push eax                                  // PicDown
  push esi                                  // PicUp
  dec  ebp
  push ebp                                  // ButtUp
  push 65                                   // ButtDown
  push ebp                                  // HovOff
  push ebp                                  // HovOn
  mov  ecx, 40                              // Width
  push ecx                                  // Height
  mov  edx, 354                             // Xpos
  mov  ebx, 154                             // Ypos
  mov  eax, ds:[_i_wid]                     // WinRef
  call win_register_button_
noLootButton:
  mov  ebx, [esp+0x18+0x4+0x20]
  mov  eax, [ebx+0x20]
  and  eax, 0xF000000
  sar  eax, 0x18
  test eax, eax                             // Это ObjType_Item?
  jnz  skip                                 // Нет
  xchg ebx, eax
  call item_get_type_
  cmp  eax, item_type_container
  je   goodTarget                           // Да
  jmp  noDropButton
skip:
  cmp  eax, ObjType_Critter
  jne  noDropButton                         // Нет
  xchg ebx, eax
  call critter_body_type_
  test eax, eax                             // Это Body_Type_Biped?
  jnz  noDropButton                         // Нет
goodTarget:
  xor  ebp, ebp
  push ebp
  mov  edx, 255                             // DROPN.FRM (Action menu drop normal)
  mov  eax, ObjType_Intrface
  xor  ecx, ecx
  xor  ebx, ebx
  call art_id_
  mov  ecx, 0x59CD8C
  xor  ebx, ebx
  xor  edx, edx
  call art_ptr_lock_data_
  test eax, eax
  jz   noDropButton
  xchg esi, eax
  push ebp
  mov  edx, 254                             // DROPH.FRM (Action menu drop highlighted )
  mov  eax, ObjType_Intrface
  xor  ecx, ecx
  xor  ebx, ebx
  call art_id_
  mov  ecx, 0x59CD90
  xor  ebx, ebx
  xor  edx, edx
  call art_ptr_lock_data_
  test eax, eax
  jz   noDropButton
  push ebp                                  // ButType
  push ebp
  push eax                                  // PicDown
  push esi                                  // PicUp
  dec  ebp
  push ebp                                  // ButtUp
  push 68                                   // ButtDown
  push ebp                                  // HovOff
  push ebp                                  // HovOn
  mov  ecx, 40                              // Width
  push ecx                                  // Height
  mov  edx, 140                             // Xpos
  mov  ebx, 154                             // Ypos
  mov  eax, ds:[_i_wid]                     // WinRef
  call win_register_button_
noDropButton:
  popad
end:
  mov  ecx, 0x2000000
  retn
 }
}

static char OverloadedLoot[48];
static char OverloadedDrop[48];
static void __declspec(naked) loot_drop_all_() {
 __asm {
  cmp  eax, 'A'
  je   lootKey
  cmp  eax, 'a'
  je   lootKey
  cmp  eax, 'D'
  je   dropKey
  cmp  eax, 'd'
  je   dropKey
  cmp  eax, 0x148
  mov  ebx, 0x467496
  jmp  ebx
lootKey:
  pushad
  cmp  dword ptr ds:[_gIsSteal], 0
  jne  end
  mov  ecx, [esp+0x10C+0x20]
  mov  eax, ecx
  call critter_body_type_
  test eax, eax                             // Это Body_Type_Biped?
  jnz  end                                  // Нет
  mov  edx, STAT_carry_amt
  mov  eax, ecx
  call stat_level_
  xchg edx, eax
  mov  eax, ecx
  call item_total_weight_
  sub  edx, eax
  mov  eax, ebp
  call item_total_weight_
  cmp  eax, edx
  jg   cantLoot
  mov  edx, ecx
  mov  eax, ebp
  jmp  moveAll
cantLoot:
  lea  edx, OverloadedLoot
  jmp  printError
dropKey:
  pushad
  cmp  dword ptr ds:[_gIsSteal], 0
  jne  end
  mov  ecx, [esp+0x10C+0x20]
  mov  eax, ecx
  call critter_body_type_
  test eax, eax                             // Это Body_Type_Biped?
  jnz  end                                  // Нет
  mov  eax, [ebp+0x20]
  and  eax, 0xF000000
  sar  eax, 0x18
  test eax, eax                             // Это ObjType_Item?
  jz   itsItem                              // Да
  cmp  eax, ObjType_Critter
  jne  end                                  // Нет
  mov  eax, ebp
  call critter_body_type_
  test eax, eax                             // Это Body_Type_Biped?
  jnz  end                                  // Нет
  mov  edx, STAT_carry_amt
  mov  eax, ebp
  call stat_level_
  xchg edx, eax                             // edx = макс. вес груза цели
  sub  edx, WeightOnBody                    // Учитываем вес одетой на цели брони и оружия
  mov  eax, ebp
  call item_total_weight_                   // eax = общий вес груза цели
  sub  edx, eax
  mov  eax, ecx
  call item_total_weight_
  jmp  compareSizeWeight
itsItem:
  mov  eax, ebp
  call item_get_type_
  cmp  eax, item_type_container
  jne  end                                  // Нет
  mov  eax, ebp
  call item_c_max_size_
  xchg edx, eax
  mov  eax, ebp
  call item_c_curr_size_
  sub  edx, eax
  mov  eax, ecx
  call item_c_curr_size_
compareSizeWeight:
  cmp  eax, edx
  jg   cantDrop
  mov  edx, ebp
  mov  eax, ecx
moveAll:
  push eax
  mov  eax, 0x4F3564                        // 'ib1p1xx1'
  call gsound_play_sfx_file_
  pop  eax
  call item_move_all_
  mov  ecx, 2
  push ecx
  mov  eax, ds:[_target_curr_stack]
  mov  edx, -1
  push edx
  mov  ebx, ds:[_target_pud]
  mov  eax, ds:[_target_stack_offset][eax*4]
  call display_target_inventory_
  mov  eax, ds:[_curr_stack]
  pop  edx                                  // -1
  pop  ebx                                  // 2
  mov  eax, ds:[_stack_offset][eax*4]
  call display_inventory_
  jmp  end
cantDrop:
  lea  edx, OverloadedDrop
printError:
  mov  eax, 0x4F3564                        // 'ib1p1xx1'
  call gsound_play_sfx_file_
  xor  eax, eax
  push eax
  mov  al, ds:[0x6AB968]                    // color
  push eax
  xor  ebx, ebx
  push ebx
  push eax
  mov  ecx, 169
  push 117
  xor  eax, eax
  xchg edx, eax
  call dialog_out_
end:
  popad
  mov  ebx, 0x4678DE
  jmp  ebx
 }
}

static char SuperStimMsg[128];
static void __declspec(naked) protinst_default_use_item_hook() {
 __asm {
  mov  ecx, ebx                             // ecx = item
  mov  edx, [edi+0x20]                      // edx = target fid
  and  edx, 0x0F000000
  sar  edx, 0x18
  cmp  edx, ObjType_Critter
  mov  edx, [ebx+0x64]                      // edx = item pid
  jne  end
  cmp  edx, PID_SUPER_STIMPAK
  jne  end
  push edx
  push eax
  mov  eax, edi                             // eax = target
  mov  edx, STAT_max_hit_points
  call stat_level_
  cmp  eax, [edi+0x58]                      // max_hp == curr_hp?
  pop  eax
  pop  edx
  jne  end                                  // Нет
  pop  eax                                  // Уничтожаем адрес возврата
  mov  eax, offset SuperStimMsg
  mov  esi, 0x48B875
  jmp  esi
end:
  retn
 }
}

void InventoryInit() {

 ReloadWeaponKey = GetPrivateProfileIntA("Input", "ReloadWeaponKey", 0, ini);
 if (ReloadWeaponKey) HookCall(0x43BB37, &ReloadWeaponHotKey);

 if (GetPrivateProfileInt("Misc", "AutoReloadWeapon", 0, ini)) {
  HookCall(0x420DA6, &AutoReloadWeapon);
 }

// Не вызывать окошко выбора количества при перетаскивании патронов в оружие
 int ReloadReserve = GetPrivateProfileIntA("Misc", "ReloadReserve", 1, ini);
 if (ReloadReserve >= 0) {
  SafeWrite8(0x4695DD, 0xB8);
  SafeWrite32(0x4695DE, ReloadReserve);      // mov  eax, ReloadReserve
  SafeWrite32(0x4695E2, 0x057FC139);         // cmp  ecx, eax; jg   0x4695EB
  SafeWrite32(0x4695E6, 0xEB40C031);         // xor  eax, eax; inc  eax; jmps 0x4695EE
  SafeWrite32(0x4695EA, 0x91C12903);         // sub  ecx, eax; xchg ecx, eax
 };

 if (GetPrivateProfileIntA("Misc", "StackEmptyWeapons", 0, ini)) {
  MakeCall(0x466D3E, &inven_action_cursor_hook, true);
  HookCall(0x46A235, &item_add_mult_hook);
 }

 if (GetPrivateProfileInt("Misc", "FreeWeight", 0, ini)) {
  MakeCall(0x463DF0, &display_inventory_hook, true);
  MakeCall(0x4640EB, &display_target_inventory_hook, true);

  HookCall(0x4685B6, &display_table_inventories_hook);

  SafeWrite16(0x46861A, 0xD231);
  MakeCall(0x46864F, &display_table_inventories_hook1, true);
  HookCall(0x468772, &display_table_inventories_hook2);

#ifdef TRACE
  SafeWrite16(0x468798, 0xD231);
  MakeCall(0x4687E0, &display_table_inventories_hook3, true);
  HookCall(0x468929, &display_table_inventories_hook2);
#endif

  HookCall(0x468A8B, &barter_inventory_hook);
 }

// Использование химии из инвентаря на картинке игрока
 MakeCall(0x465008, &inven_pickup_hook2, true);

// Показывать в инвентаре макс.вес
 MakeCall(0x465EF4, &display_stats_hook, true);

// Кнопки "Взять всё" и "Положить всё"
 MakeCall(0x4638D3, &make_loot_drop_button, false);
 MakeCall(0x467491, &loot_drop_all_, true);
 GetPrivateProfileString("sfall", "OverloadedLoot", "Sorry, you cannot carry that much.", OverloadedLoot, 48, translationIni);
 GetPrivateProfileString("sfall", "OverloadedDrop", "Sorry, there is no space left.", OverloadedDrop, 48, translationIni);

 if (GetPrivateProfileInt("Misc", "SuperStimExploitFix", 0, ini)) {
  GetPrivateProfileString("sfall", "SuperStimExploitMsg", "You cannot use a super stim on someone who is not injured!", SuperStimMsg, 128, translationIni);
  MakeCall(0x48B837, &protinst_default_use_item_hook, false);
 }

}
