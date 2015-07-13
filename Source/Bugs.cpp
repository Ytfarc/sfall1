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

static const DWORD item_d_check_addict_hook_End = 0x46CC5D;
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
  jmp  item_d_check_addict_hook_End
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

static const DWORD inven_right_hand_hook_Cont = 0x46555D;
static const DWORD inven_right_hand_hook_End = 0x465572;
static void __declspec(naked) inven_right_hand_hook() {
 __asm {
  cmp  eax, ds:[_inven_dude]
  je   end
  jmp  inven_right_hand_hook_Cont
end:
  xchg edx, eax
  jmp  inven_right_hand_hook_End
 }
}

static const DWORD inven_left_hand_hook_Cont = 0x46559D;
static const DWORD inven_left_hand_hook_End = 0x4655B2;
static void __declspec(naked) inven_left_hand_hook() {
 __asm {
  cmp  eax, ds:[_inven_dude]
  je   end
  jmp  inven_left_hand_hook_Cont
end:
  xchg edx, eax
  jmp  inven_left_hand_hook_End
 }
}

static const DWORD inven_worn_hook_Cont = 0x4655DD;
static const DWORD inven_worn_hook_End = 0x4655F2;
static void __declspec(naked) inven_worn_hook() {
 __asm {
  cmp  eax, ds:[_inven_dude]
  je   end
  jmp  inven_worn_hook_Cont
end:
  xchg edx, eax
  jmp  inven_worn_hook_End
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
static const DWORD inven_pickup_hook_Fail = 0x464E16;
static const DWORD inven_pickup_hook_Loop = 0x464D00;
static const DWORD inven_pickup_hook_End = 0x464D9A;
static const DWORD inven_pickup_hook_End1 = 0x464D3C;
static void __declspec(naked) inven_pickup_hook() {
 __asm {
  cmp  inven_pickup_loop, -1
  jne  inLoop
  test eax, eax
  jnz  startLoop
  jmp  inven_pickup_hook_Fail
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
  jmp  inven_pickup_hook_End
inRange:
  pop  eax
  jmp  inven_pickup_hook_End1
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

static const DWORD barter_attempt_transaction_hook_Cont = 0x467FE8;
static void __declspec(naked) barter_attempt_transaction_hook() {
 __asm {
  cmp  dword ptr [eax+0x64], PID_ACTIVE_GEIGER_COUNTER
  je   found
  cmp  dword ptr [eax+0x64], PID_ACTIVE_STEALTH_BOY
  je   found
  xor  eax, eax
  dec  eax
  retn
found:
  call item_m_turn_off_
  pop  eax                                  // Уничтожаем адрес возврата
  jmp  barter_attempt_transaction_hook_Cont // А есть ли ещё включённые предметы среди продаваемых?
 }
}

static void __declspec(naked) item_m_turn_off_hook() {
 __asm {
  and  byte ptr [eax+0x25], 0xDF            // Сбросим флаг использованного предмета
  jmp  queue_remove_this_
 }
}

//checks if an attacked object is a critter before attempting dodge animation
static void __declspec(naked) action_melee_hook() {
 __asm {
  mov  eax, [ebp+0x20]                      // (original code) objStruct ptr
  mov  ebx, [eax+0x20]                      // objStruct->FID
  and  ebx, 0x0F000000
  sar  ebx, 0x18
  cmp  ebx, ObjType_Critter                 // check if object FID type flag is set to critter
  jne  end                                  // if object not a critter leave jump condition flags
                                            // set to skip dodge animation
  test byte ptr [eax+0x44], 3               // (original code) DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN
end:
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

// Исправление невозможности продажи ранее использованных "Счетчик Гейгера"/"Невидимка"
// SafeWrite8(0x46ADDA, 0xBA);
 SafeWrite8(0x46ADFA, 0xBA);
 SafeWrite8(0x46AE2C, 0xBA);
 MakeCall(0x467FF3, &barter_attempt_transaction_hook, false);
 HookCall(0x46C0B9, &item_m_turn_off_hook);

 dlog("Applying Dodgy Door Fix.", DL_INIT);
 SafeWrite16(0x4112E3, 0x9090);
 MakeCall(0x4112E5, &action_melee_hook, false);
 dlogr(" Done", DL_INIT);

}
