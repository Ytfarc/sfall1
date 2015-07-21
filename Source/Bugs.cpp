#include "main.h"

#include "Bugs.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "Logging.h"

DWORD WeightOnBody = 0;

static void __declspec(naked) determine_to_hit_func_hook() {
 __asm {
  call stat_level_                          // Perception|Восприятие
  cmp  edi, ds:[_obj_dude]
  jne  end
  mov  ecx, PERK_sharpshooter
  xchg ecx, eax
  call perk_level_
  shl  eax, 1
  add  eax, ecx
end:
  retn
 }
}

static void __declspec(naked) perform_withdrawal_start_hook() {
 __asm {
  test eax, eax
  jnz  end
  retn
end:
  jmp  display_print_
 }
}

static void __declspec(naked) pipboy_hook() {
 __asm {
  cmp  ebx, 0x20E                           // Кнопка НАЗАД?
  je   end
  cmp  byte ptr ds:[_holo_flag], 0
  jne  end
  xor  ebx, ebx                             // Нет человека - нет проблемы (c) :-p
end:
  mov  eax, ds:[_crnt_func]
  retn
 }
}

static void __declspec(naked) PipAlarm_hook() {
 __asm {
  mov  ds:[_crnt_func], eax
  mov  eax, 0x400
  call PipStatus_
  mov  eax, 0x4FB9D0                        // 'iisxxxx1'
  retn
 }
}

static void __declspec(naked) scr_save_hook() {
 __asm {
  mov  ecx, 16
  cmp  dword ptr [esp+0xDC+4], ecx          // number_of_scripts
  jg   skip
  mov  ecx, dword ptr [esp+0xDC+4]
  cmp  ecx, 0
  jg   skip
  xor  eax, eax
  retn
skip:
  sub  dword ptr [esp+0xDC+4], ecx          // number_of_scripts
  push dword ptr [ebp+0xD00]                // num
  mov  dword ptr [ebp+0xD00], ecx           // num
  xor  ecx, ecx
  xchg dword ptr [ebp+0xD04], ecx           // NextBlock
  call scr_write_ScriptNode_
  xchg dword ptr [ebp+0xD04], ecx           // NextBlock
  pop  dword ptr [ebp+0xD00]                // num
  retn
 }
}

static void __declspec(naked) item_d_check_addict_hook() {
 __asm {
  mov  edx, 2                               // type = зависимость
  inc  eax
  test eax, eax                             // Есть drug_pid?
  jz   skip                                 // Нет
  dec  eax
  xchg ebx, eax                             // ebx = drug_pid
  mov  eax, esi                             // eax = who
  call queue_find_first_
loopQueue:
  test eax, eax                             // Есть что в списке?
  jz   end                                  // Нет
  cmp  ebx, dword ptr [eax+0x4]             // drug_pid == queue_addict.drug_pid?
  je   end                                  // Есть конкретная зависимость
  mov  eax, esi                             // eax = who
  call queue_find_next_
  jmp  loopQueue
skip:
  xchg ecx, eax                             // eax = _obj_dude
  call queue_find_first_
end:
  mov  esi, 0x46CC5D
  jmp  esi
 }
}

static void __declspec(naked) queue_clear_type_hook() {
 __asm {
  mov  ebx, [esi]
  jmp  mem_free_
 }
}

static void __declspec(naked) invenWieldFunc_hook() {
 __asm {
  pushad
  mov  edi, ecx
  mov  edx, esi
  xor  ebx, ebx
  inc  ebx
  push ebx
  mov  cl, byte ptr [edi+0x27]
  and  cl, 0x3
  xchg edx, eax                             // eax=who, edx=item
  call item_remove_mult_
nextWeapon:
  mov  eax, esi
  test cl, 0x2                              // Правая рука?
  jz   leftHand                             // Нет
  call inven_right_hand_
  jmp  removeFlag
leftHand:
  call inven_left_hand_
removeFlag:
  test eax, eax
  jz   noWeapon
  and  byte ptr [eax+0x27], 0xFC            // Сбрасываем флаг оружия в руке
  jmp  nextWeapon
noWeapon:
  or   byte ptr [edi+0x27], cl              // Устанавливаем флаг оружия в руке
  xchg esi, eax
  mov  edx, edi
  pop  ebx
  call item_add_force_
  popad
  jmp  item_get_type_
 }
}

static void __declspec(naked) inven_right_hand_hook() {
 __asm {
  mov  esi, 0x46555D
  cmp  eax, ds:[_inven_dude]
  jne  end
  xchg edx, eax
  mov  esi, 0x465572
end:
  jmp  esi
 }
}

static void __declspec(naked) inven_left_hand_hook() {
 __asm {
  mov  esi, 0x46559D
  cmp  eax, ds:[_inven_dude]
  jne  end
  xchg edx, eax
  mov  esi, 0x4655B2
end:
  jmp  esi
 }
}

static void __declspec(naked) inven_worn_hook() {
 __asm {
  mov  esi, 0x4655DD
  cmp  eax, ds:[_inven_dude]
  jne  end
  xchg edx, eax
  mov  esi, 0x4655F2
end:
  jmp  esi
 }
}

static void __declspec(naked) loot_container_hook() {
 __asm {
  mov  eax, [esp+0x110+0x4]
  test eax, eax
  jz   noArmor
  call item_weight_
noArmor:
  mov  WeightOnBody, eax
  mov  eax, [esp+0x114+0x4]
  test eax, eax
  jz   noLeftWeapon
  call item_weight_
noLeftWeapon:
  add  WeightOnBody, eax
  mov  eax, [esp+0x118+0x4]
  test eax, eax
  jz   noRightWeapon
  call item_weight_
noRightWeapon:
  add  WeightOnBody, eax
  xor  eax, eax
  inc  eax
  inc  eax
  retn
 }
}

#ifdef TRACE
static void __declspec(naked) barter_inventory_hook() {
 __asm {
  mov  eax, [esp+0x18+0x4]
  test eax, eax
  jz   noArmor
  call item_weight_
noArmor:
  mov  WeightOnBody, eax
  mov  eax, [esp+0x1C+0x4]
  test eax, eax
  jnz  RightWeapon
  mov  eax, [esp+0x14+0x4]
  test eax, eax
  jz   end
RightWeapon:
  call item_weight_
end:
  add  WeightOnBody, eax
  mov  eax, ds:[_inven_dude]
  retn
 }
}
#endif

static DWORD Looting = 0;
static void __declspec(naked) move_inventory_hook() {
 __asm {
  inc  Looting
  call move_inventory_
  dec  Looting
  retn
 }
}

static void __declspec(naked) correctWeight() {
 __asm {
  call stat_level_                          // eax = Макс. груз
  cmp  Looting, 0
  je   end
  sub  eax, WeightOnBody                    // Учитываем вес одетой на цели брони и оружия
end:
  retn
 }
}

static DWORD inven_pickup_loop=-1;
static const DWORD inven_pickup_hook_Loop = 0x464D00;
static void __declspec(naked) inven_pickup_hook() {
 __asm {
  cmp  inven_pickup_loop, -1
  jne  inLoop
  test eax, eax
  jnz  startLoop
  mov  eax, 0x464E16
  jmp  eax
startLoop:
  xor  edx, edx
  mov  inven_pickup_loop, edx
nextLoop:
  mov  eax, 124                             // x_start
  mov  ebx, 188                             // x_end
  add  edx, 35                              // y_start
  mov  ecx, edx
  add  ecx, 48                              // y_end
  jmp  inven_pickup_hook_Loop
inLoop:
  test eax, eax
  mov  eax, inven_pickup_loop
  jnz  foundRect
  inc  eax
  mov  inven_pickup_loop, eax
  imul edx, eax, 48
  jmp  nextLoop
foundRect:
  mov  inven_pickup_loop, -1
  mov  edx, [esp+0x3C]                      // inventory_offset
  add  edx, eax
  mov  eax, ds:[_pud]
  push eax
  mov  eax, [eax]                           // itemsCount
  test eax, eax
  jz   skip
  dec  eax
  cmp  edx, eax
  jle  inRange
skip:
  pop  eax
  mov  ebx, 0x464D9A
  jmp  ebx
inRange:
  pop  eax
  mov  ecx, 0x464D3C
  jmp  ecx
 }
}

static void __declspec(naked) drop_ammo_into_weapon_hook() {
 __asm {
  push ecx
  mov  esi, ecx
  dec  esi
  test esi, esi                             // Одна коробка патронов?
  jz   skip                                 // Да
  xor  esi, esi
// Лишняя проверка на from_slot, но пусть будет
  cmp  edi, 1006                            // Руки?
  jge  skip                                 // Да
  lea  edx, [eax+0x2C]                      // Inventory
  mov  ecx, [edx]                           // itemsCount
  jcxz skip                                 // инвентарь пустой (ещё лишняя проверка, но пусть будет)
  mov  edx, [edx+8]                         // FirstItem
nextItem:
  cmp  ebp, [edx]                           // Наше оружие?
  je   foundItem                            // Да
  add  edx, 8                               // К следующему
  loop nextItem
  jmp  skip                                 // Нашего оружия нет в инвентаре
foundItem:
  cmp  dword ptr [edx+4], 1                 // Оружие в единственном экземпляре?
  jg   skip                                 // Нет
  lea  edx, [eax+0x2C]                      // Inventory
  mov  edx, [edx]                           // itemsCount
  sub  edx, ecx                             // edx=порядковый номер слота с оружием
  lea  ecx, [edi-1000]                      // from_slot
  add  ecx, [esp+0x3C+4+0x24+8]             // ecx=порядковый номер слота с патронами
  cmp  edx, ecx                             // Оружие после патронов?
  jg   skip                                 // Да
  inc  esi                                  // Нет, нужно менять from_slot
skip:
  pop  ecx
  mov  edx, ebp
  call item_remove_mult_
  test eax, eax                             // Удалили оружие из инвентаря?
  jnz  end                                  // Нет
  sub  [esp+0x24+4], esi                    // Да, корректируем from_slot
end:
  retn
 }
}

static void __declspec(naked) PipStatus_hook() {
 __asm {
  call AddHotLines_
  xor  eax, eax
  mov  dword ptr ds:[_hot_line_count], eax
  retn
 }
}

//checks if an attacked object is a critter before attempting dodge animation
static void __declspec(naked) action_melee_hook() {
 __asm {
  mov  edx, 0x4112EC
  mov  ebx, [eax+0x20]                      // pobj.fid
  and  ebx, 0x0F000000
  sar  ebx, 0x18
  cmp  ebx, ObjType_Critter                 // check if object FID type flag is set to critter
  jne  end                                  // if object not a critter skip dodge animation
  test byte ptr [eax+0x44], 0x3             // (DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN)?
  jnz  end
  mov  edx, 0x41130E
end:
  jmp  edx
 }
}

static void __declspec(naked) action_ranged_hook() {
 __asm {
  mov  edx, 0x4119F1
  mov  ebx, [eax+0x20]                      // pobj.fid
  and  ebx, 0x0F000000
  sar  ebx, 0x18
  cmp  ebx, ObjType_Critter                 // check if object FID type flag is set to critter
  jne  end                                  // if object not a critter skip dodge animation
  test byte ptr [eax+0x44], 0x3             // (DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN)?
  jnz  end
  mov  edx, 0x411A56
end:
  jmp  edx
 }
}


static DWORD XPWithSwiftLearner;
static void __declspec(naked) stat_pc_add_experience_hook() {
 __asm {
  mov  XPWithSwiftLearner, esi
  mov  eax, ds:[_Experience_]
  retn
 }
}

static void __declspec(naked) combat_give_exps_hook() {
 __asm {
  call stat_pc_add_experience_
  mov  ebx, XPWithSwiftLearner
  retn
 }
}

static void __declspec(naked) loot_container_hook1() {
 __asm {
  xchg edi, eax
  call stat_pc_add_experience_
  cmp  edi, 1
  jne  skip
  push XPWithSwiftLearner
  mov  ebx, [esp+0xF4]
  push ebx
  lea  eax, [esp+0x8]
  push eax
  call sprintf_
  add  esp, 0xC
  mov  eax, esp
  call display_print_
skip:
  mov  ebx, 0x4679FA
  jmp  ebx
 }
}

static DWORD critterObj;
static void __declspec(naked) critterClearObj() {
 __asm {
  cmp  eax, critterObj
  setz al
  and  eax, 0xFF
  retn
 }
}

static void __declspec(naked) set_new_results_hook() {
 __asm {
  test ah, 0x1                              // DAM_KNOCKED_OUT?
  jz   end                                  // Нет
  mov  critterObj, esi
  mov  edx, offset critterClearObj
  xor  eax, eax
  inc  eax                                  // type = отключка
  call queue_clear_type_                    // Удаляем отключку из очереди (если отключка там есть)
  retn
end:
  pop  eax                                  // Уничтожаем адрес возврата
  mov  eax, 0x422871
  jmp  eax
 }
}

static void __declspec(naked) critter_wake_clear_hook() {
 __asm {
  test dl, 0x80                             // DAM_DEAD?
  jnz  end                                  // Это трупик
  and  dl, 0xFE                             // Сбрасываем DAM_KNOCKED_OUT
  or   dl, 0x2                              // Устанавливаем DAM_KNOCKED_DOWN
  mov  [esi+0x44], dl
end:
  xor  eax, eax
  inc  eax
  pop  esi
  pop  ecx
  pop  ebx
  retn
 }
}

static void __declspec(naked) obj_load_func_hook() {
 __asm {
  mov  edi, 0x47B187
  test byte ptr [eax+0x25], 0x4             // Temp_
  jnz  end
  mov  edi, [eax+0x64]
  shr  edi, 0x18
  cmp  edi, ObjType_Critter
  jne  skip
  test byte ptr [eax+0x44], 0x2             // DAM_KNOCKED_DOWN?
  jz   clear                                // Нет
  pushad
  xor  ecx, ecx
  inc  ecx
  xor  ebx, ebx
  xor  edx, edx
  xchg edx, eax
  call queue_add_
  popad
clear:
  and  word ptr [eax+0x44], 0x7FFD          // not (DAM_LOSE_TURN or DAM_KNOCKED_DOWN)
skip:
  mov  edi, 0x47B1A2
end:
  jmp  edi
 }
}

static void __declspec(naked) partyMemberPrepLoad_hook() {
 __asm {
  test byte ptr [ecx+0x44], 0x2             // DAM_KNOCKED_DOWN
  jz   skip
  mov  eax, ecx
  mov  edx, [ecx+0x1C]
  xor  ebx, ebx
  dec  ebx
  call dude_stand_
skip:
  and  word ptr [ecx+0x44], 0x7FFD          // not (DAM_LOSE_TURN or DAM_KNOCKED_DOWN)
  xor  edx, edx
  mov  ebx, [ecx+0x2C]
  retn
 }
}

void BugsInit() {

 dlog("Applying sharpshooter patch.", DL_INIT);
 HookCall(0x42205D, &determine_to_hit_func_hook);
 SafeWrite8(0x422094, 0xEB);
 dlogr(" Done", DL_INIT);

 dlog("Applying withdrawal perk description crash fix. ", DL_INIT);
 HookCall(0x46CA87, &perform_withdrawal_start_hook);
 dlogr(" Done", DL_INIT);

// Исправление багов кликабельности в пипбое
 MakeCall(0x48709E, &pipboy_hook, false);
 MakeCall(0x48934C, &PipAlarm_hook, false);

// Исправление ошибки "Too Many Items Bug"
 HookCall(0x493FA6, &scr_save_hook);
 HookCall(0x493FFD, &scr_save_hook);

// Исправление обработки наркотической зависимости
 MakeCall(0x46CC00, &item_d_check_addict_hook, true);

// Исправление краша (которого тут нет) при использовании стимпаков на жертве с последующим
// выходом с карты
 HookCall(0x490F9B, &queue_clear_type_hook);

// Исправление "Unlimited Ammo bug"
 HookCall(0x4660DB, &invenWieldFunc_hook);

// Исправление отображения отрицательных значений в окне навыков ("S")
 SafeWrite8(0x499E17, 0x7F);                // jg

// Исправление возврата одетой брони и оружия в руках
 MakeCall(0x465556, &inven_right_hand_hook, true);
 MakeCall(0x465596, &inven_left_hand_hook, true);
 MakeCall(0x4655D6, &inven_worn_hook, true);

// Исправление ошибки неучёта веса одетых вещей
 MakeCall(0x467132, &loot_container_hook, false);
#ifdef TRACE
 MakeCall(0x4689D1, &barter_inventory_hook, false);
#endif
 HookCall(0x467765, &move_inventory_hook);
 HookCall(0x46A146, &correctWeight);

// Ширина текста 64, а не 80 
 SafeWrite8(0x46872D, 64);
 SafeWrite8(0x4688D5, 64);

// Исправление ошибки в инвентаре игрока связанной с IFACE_BAR_MODE=1 из f1_res.ini
 MakeCall(0x464D05, &inven_pickup_hook, true);

// Исправление ошибки использования только одной пачки патронов когда оружие находится перед
// патронами
 HookCall(0x46960B, &drop_ammo_into_weapon_hook);

 dlog("Applying black skilldex patch.", DL_INIT);
 HookCall(0x487BC0, &PipStatus_hook);
 dlogr(" Done", DL_INIT);

 dlog("Applying Dodgy Door Fix.", DL_INIT);
 MakeCall(0x4112E6, &action_melee_hook, true);
 MakeCall(0x411A50, &action_ranged_hook, true);
 dlogr(" Done", DL_INIT);

// При выводе количества полученных очков опыта учитывать перк 'Прилежный ученик'
 MakeCall(0x49D178, &stat_pc_add_experience_hook, false);
 HookCall(0x420273, &combat_give_exps_hook);
 MakeCall(0x4679CC, &loot_container_hook1, true);

// Исправление "NPC turns into a container"
 MakeCall(0x422839, &set_new_results_hook, false);
 MakeCall(0x428DD0, &critter_wake_clear_hook, true);
 MakeCall(0x47B181, &obj_load_func_hook, true);
 MakeCall(0x485B0C, &partyMemberPrepLoad_hook, false);

}
