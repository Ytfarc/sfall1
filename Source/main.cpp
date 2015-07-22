//#define STRICT
//#define WIN32_LEAN_AND_MEAN

#include "main.h"

#include <math.h>
#include <stdio.h>
#include "AmmoMod.h"
#include "Bugs.h"
#include "Console.h"
#include "CRC.h"
#include "Credits.h"
#include "Criticals.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "Graphics.h"
#include "input.h"
#include "Inventory.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "MainMenu.h"
#include "PartyControl.h"
#include "Quests.h"
#include "timer.h"
#include "version.h"

char ini[65];
char translationIni[65];

static char mapName[65];
static char versionString[65];
static char windowName[65];
static char configName[65];
static char dmModelName[65];
static char dfModelName[65];
static char MovieNames[14*65];

static const char* origMovieNames[] = {
 "iplogo.mve",
 "mplogo.mve",
 "intro.mve",
 "vexpld.mve",
 "cathexp.mve",
 "ovrintro.mve",
 "boil3.mve",
 "ovrrun.mve",
 "walkm.mve",
 "walkw.mve",
 "dipedv.mve",
 "boil1.mve",
 "boil2.mve",
 "raekills.mve",
};

//GetTickCount calls
static const DWORD offsetsA[] = {
 0x4B3AAC,                                  // GNW_do_bk_process
 0x4B40ED,                                  // get_time
 0x4B40FC,                                  // pause_for_tocks
 0x4B4138,                                  // block_for_tocks
 0x4B4160,                                  // elapsed_time
 0x4B4AA6,                                  // GNW95_process_message_
 0x4E0D92,                                  // jmp  GetTickCount
};

//Delayed GetTickCount calls
static const DWORD offsetsB[] = {
 0x4E0CD8,                                  // GetTickCount_0
};

//timeGetTime calls
static const DWORD offsetsC[] = {
 0x491A29,                                  // init_random_
 0x491B0D,                                  // timer_read_
 0x4D75AB,                                  // sub_4D75A0
 0x4D7BD3,                                  // syncWait_
 0x4D82C2,                                  // syncReset_
 0x4D844C,                                  // MVE_syncSync_
 0x4D8746,                                  // wait_for_period_and_half
 0x4E0DA4,                                  // jmp  timeGetTime
};

static DWORD AddrGetTickCount;
static DWORD AddrGetLocalTime;

static DWORD _objItemOutlineState = 0;
static DWORD toggleHighlightsKey;
static DWORD TurnHighlightContainers = 0;
static int idle;

struct sMessage {
 DWORD number;
 DWORD flags;
 char* message;
};

DWORD _combatNumTurns = 0;
DWORD _tmpQNode;

static const DWORD WalkDistance[] = {
 0x411E63,                                  // action_use_an_item_on_object_
 0x412028,                                  // action_get_an_object_
 0x4122E1,                                  // action_loot_container_
 0x412692,                                  // action_use_skill_on_
};

static char KarmaGainMsg[128];
static char KarmaLossMsg[128];
static void _stdcall SetKarma(int value) {
 char buf[128];
 if (value > 0) {
  sprintf_s(buf, KarmaGainMsg, value);
 } else {
  sprintf_s(buf, KarmaLossMsg, -value);
 }
 __asm {
  lea  eax, buf
  call display_print_
 }
}

static void __declspec(naked) op_set_global_var_hook() {
 __asm {
  cmp  eax, 155                             // PLAYER_REPUATION
  jne  end
  pushad
  call game_get_global_var_
  sub  edx, eax
  test edx, edx
  jz   skip
  push edx
  call SetKarma
skip:
  popad
end:
  jmp  game_set_global_var_
 }
}

static void __declspec(naked) intface_item_reload_hook() {
 __asm {
  pushad
  mov  eax, dword ptr ds:[_obj_dude]
  push eax
  call register_clear_
  xor  eax, eax
  inc  eax
  call register_begin_
  xor  edx, edx
  xor  ebx, ebx
  dec  ebx
  pop  eax                                  // _obj_dude
  call register_object_animate_
  call register_end_
  popad
  jmp  gsound_play_sfx_file_
 }
}

static DWORD RetryCombatMinAP;
static void __declspec(naked) combat_turn_hook() {
 __asm {
  xor  eax, eax
retry:
  xchg ebx, eax
  mov  eax, esi
  push edx
  call combat_ai_
  pop  edx
process:
  cmp  dword ptr ds:[_combat_turn_running], 0
  jle  next
  call process_bk_
  jmp  process
next:
  mov  eax, [esi+0x40]                      // curr_mp
  cmp  eax, RetryCombatMinAP
  jl   end
  cmp  eax, ebx
  jne  retry
end:
  retn
 }
}

static void __declspec(naked) intface_rotate_numbers_hook() {
 __asm {
// ebx=old value, ecx=new value
  push edi
  push ebp
  sub  esp, 0x54
  cmp  ebx, ecx
  je   end
  mov  ebx, ecx
  jg   greater
  inc  ebx
  jmp  end
greater:
  cmp  ebx, 0
  jg   skip
  xor  ebx, ebx
  inc  ebx
skip:
  dec  ebx
end:
  mov  esi, 0x4565C6
  jmp  esi
 }
}

static void __declspec(naked) DebugMode() {
 __asm {
  call config_set_value_
  jmp  debug_register_env_
 }
}

static void __declspec(naked) obj_outline_all_items_on_() {
 __asm {
  push ebx
  mov  eax, dword ptr ds:[_map_elevation]
  call obj_find_first_at_
  test eax, eax
  jz   end
loopObject:
  cmp  eax, ds:[_outlined_object]
  je   nextObject
  xchg ecx, eax
  mov  eax, [ecx+0x20]
  and  eax, 0xF000000
  sar  eax, 0x18
  test eax, eax                             // Это ObjType_Item?
  jnz  nextObject                           // Нет
  cmp  dword ptr [ecx+0x7C], eax            // Кому-то принадлежит?
  jnz  nextObject                           // Да
  test dword ptr [ecx+0x74], eax            // Уже подсвечивается?
  jnz  nextObject                           // Да
  mov  edx, 0x10                            // жёлтый
  test byte ptr [ecx+0x25], dl              // Установлен NoHighlight_ (это контейнер)?
  jz   NoHighlight                          // Нет
  cmp  TurnHighlightContainers, eax         // Подсвечивать контейнеры?
  je   nextObject                           // Нет
  mov  edx, 0x4                             // серый
NoHighlight:
  mov  [ecx+0x74], edx
nextObject:
  call obj_find_next_at_
  test eax, eax
  jnz  loopObject
end:
  call tile_refresh_display_
  pop  ebx
  retn
 }
}

static void __declspec(naked) obj_outline_all_items_off_() {
 __asm {
  push ebx
  mov  eax, dword ptr ds:[_map_elevation]
  call obj_find_first_at_
  test eax, eax
  jz   end
loopObject:
  cmp  eax, ds:[_outlined_object]
  je   nextObject
  xchg ebx, eax
  mov  eax, [ebx+0x20]
  and  eax, 0xF000000
  sar  eax, 0x18
  test eax, eax                             // Это ObjType_Item?
  jnz  nextObject                           // Нет
  cmp  dword ptr [ebx+0x7C], eax            // Кому-то принадлежит?
  jnz  nextObject                           // Да
  mov  dword ptr [ebx+0x74], eax
nextObject:
  call obj_find_next_at_
  test eax, eax
  jnz  loopObject
end:
  call tile_refresh_display_
  pop  ebx
  retn
 }
}

static void __declspec(naked) gmouse_bk_process_hook() {
 __asm {
  test eax, eax
  jz   end
  test byte ptr [eax+0x25], 0x10            // NoHighlight_
  jnz  end
  mov  dword ptr [eax+0x74], 0
end:
  mov  edx, 0x40
  jmp  obj_outline_object_
 }
}

static void __declspec(naked) obj_remove_outline_hook() {
 __asm {
  call obj_remove_outline_
  test eax, eax
  jnz  end
  cmp  eax, _objItemOutlineState
  je   end
  mov  ds:[_outlined_object], eax
  pushad
  call obj_outline_all_items_on_
  popad
end:
  retn
 }
}

static void RunGlobalScripts() {
 if (idle > -1) Sleep(idle);
}

static void __declspec(naked) get_input_hook() {
 __asm {
  call get_input_
  pushad
  mov  eax, toggleHighlightsKey
  test eax, eax
  jz   end
  push eax
  call KeyDown
  mov  ebx, _objItemOutlineState
  test eax, eax
  jz   notOurKey
  test ebx, ebx
  jnz  end
  inc  ebx
  call obj_outline_all_items_on_
  jmp  setState
notOurKey:
  test ebx, ebx
  jz   end
  dec  ebx
  call obj_outline_all_items_off_
setState:
  mov  _objItemOutlineState, ebx
end:
  call RunGlobalScripts
  popad
  retn
 }
}

static byte XltTable[94];
static byte XltKey = 4;                     // 4 = Scroll Lock, 2 = Caps Lock, 1 = Num Lock
static void __declspec(naked) get_input_str_hook() {
 __asm {
  push ecx
  mov  cl, XltKey
  test byte ptr ds:[_kb_lock_flags], cl
  jz   end
  mov  ecx, offset XltTable
  and  eax, 0xFF
  mov  al, [ecx+eax-0x20]
end:
  mov  byte ptr [esp+esi+4], al
  mov  eax, 0x42E0BB
  jmp  eax
 }
}

static void __declspec(naked) get_input_str2_hook() {
 __asm {
  push ecx
  mov  cl, XltKey
  test byte ptr ds:[_kb_lock_flags], cl
  jz   end
  mov  ecx, offset XltTable
  and  eax, 0xFF
  mov  al, [ecx+eax-0x20]
end:
  mov  byte ptr [esp+edi+4], al
  mov  eax, 0x471789
  jmp  eax
 }
}

static void __declspec(naked) kb_next_ascii_English_US_hook() {
 __asm {
  mov  dh, [eax]
  cmp  dh, 0x1A                             // DIK_LBRACKET
  je   end
  cmp  dh, 0x1B                             // DIK_RBRACKET
  je   end
  cmp  dh, 0x27                             // DIK_SEMICOLON
  je   end
  cmp  dh, 0x28                             // DIK_APOSTROPHE
  je   end
  cmp  dh, 0x33                             // DIK_COMMA
  je   end
  cmp  dh, 0x34                             // DIK_PERIOD
  je   end
  cmp  dh, 0x30                             // DIK_B
end:
  mov  eax, 0x4B7065
  jmp  eax
 }
}

static void __declspec(naked) about_process_input_hook() {
 __asm {
  call text_font_
  mov  al, XltKey
  test byte ptr ds:[_kb_lock_flags], al
  jz   end
  mov  eax, offset XltTable
  and  edx, 0xFF
  mov  dl, [eax+edx-0x20]
end:
  retn
 }
}

static void __declspec(naked) stricmp_hook() {
 __asm {
  push ebx
  push ecx
  push esi
  mov  esi, offset XltTable
  xchg ebx, eax
  xor  ecx, ecx
nextChar:
  mov  al, [ebx]
  mov  ah, [edx]
  mov  cl, al
  cmp  ecx, 0x41                            // 'A'
  jl   skipDst
  cmp  ecx, 0x5A                            // 'Z'
  jle  lowerDst
  cmp  ecx, 0x80                            // 'А'
  jl   skipDst
  cmp  byte ptr [esi+2], 157                // 866 кодировка?
  jne  dst1251                              // Нет
  cmp  ecx, 0x9F                            // 'Я'
  jg   skipDst
  cmp  ecx, 0x8F                            // 'П'
  jle  lowerDst
  add  al, 0x50
  jmp  skipDst
dst1251:
  cmp  ecx, 0xC0                            // 'А'
  jl   skipDst
  cmp  ecx, 0xDF                            // 'Я'
  jg   skipDst
lowerDst:
  add  al, 0x20
skipDst:
  mov  cl, ah
  cmp  ecx, 0x41                            // 'A'
  jl   skipSrc
  cmp  ecx, 0x5A                            // 'Z'
  jle  lowerSrc
  cmp  ecx, 0x80                            // 'А'
  jl   skipSrc
  cmp  byte ptr [esi+2], 157                // 866 кодировка?
  jne  src1251                              // Нет
  cmp  ecx, 0x9F                            // 'Я'
  jg   skipSrc
  cmp  ecx, 0x8F                            // 'П'
  jle  lowerSrc
  add  ah, 0x50
  jmp  skipSrc
src1251:
  cmp  ecx, 0xC0                            // 'А'
  jl   skipSrc
  cmp  ecx, 0xDF                            // 'Я'
  jg   skipSrc
lowerSrc:
  add  ah, 0x20
skipSrc:
  cmp  al, ah
  jne  end
  test ah, ah
  jz   end
  inc  ebx
  inc  edx
  jmp  nextChar
end:
  xor  edx, edx
  mov  dl, al
  mov  al, ah
  and  eax, 0xFF
  sub  edx, eax
  xchg edx, eax
  pop  esi
  pop  ecx
  pop  ebx
  retn
 }
}

static void __declspec(naked) pipboy_hook() {
 __asm {
  call get_input_
  cmp  eax, '1'
  jne  notOne
  mov  eax, 0x1F4
  jmp  click
notOne:
  cmp  eax, '2'
  jne  notTwo
  mov  eax, 0x1F8
  jmp  click
notTwo:
  cmp  eax, '3'
  jne  notThree
  mov  eax, 0x1F5
  jmp  click
notThree:
  cmp  eax, '4'
  jne  notFour
  mov  eax, 0x1F6
click:
  push eax
  mov  eax, 0x4FBA1C                        // 'ib1p1xx1'
  call gsound_play_sfx_file_
  pop  eax
notFour:
  retn
 }
}

static const char* _nar_31 = "nar_31";
static void __declspec(naked) Brotherhood_final() {
 __asm {
  mov  eax, 16                              // BROTHERHOOD_INVADED
  call game_get_global_var_
  test eax, eax
  jnz  nar_31
  mov  eax, 605                             // RHOMBUS_STATUS
  call game_get_global_var_
  mov  edx, 0x438BC5
  test eax, eax
  jz   RhombusDead
  mov  edx, 0x438B98
RhombusDead:
  jmp  edx
nar_31:
  push 0
  mov  edx, 317                             // SEQ5D.FRM
  mov  eax, ObjType_Intrface
  xor  ecx, ecx
  xor  ebx, ebx
  call art_id_
  mov  edx, _nar_31
  mov  ebx, 0x438BDF
  jmp  ebx
 }
}

static DWORD Educated, Lifegiver, Tag_, Mutate_;
static void __declspec(naked) editor_design_hook() {
 __asm {
  call SavePlayer_
  mov  eax, ds:[_Educated]
  mov  Educated, eax
  mov  eax, ds:[_Lifegiver]
  mov  Lifegiver, eax
  mov  eax, ds:[_Tag_]
  mov  Tag_, eax
  mov  eax, ds:[_Mutate_]
  mov  Mutate_, eax
  retn
 }
}

static void __declspec(naked) editor_design_hook2() {
 __asm {
  mov  eax, Educated
  mov  ds:[_Educated], eax
  mov  eax, Lifegiver
  mov  ds:[_Lifegiver], eax
  mov  eax, Tag_
  mov  ds:[_Tag_], eax
  mov  eax, Mutate_
  mov  ds:[_Mutate_], eax
  call RestorePlayer_
  retn
 }
}

static void __declspec(naked) perks_dialog_hook() {
 __asm {
  call ListSkills_
  mov  eax, PERK_educated
  call perk_level_
  mov  dword ptr ds:[_Educated], eax
  mov  eax, PERK_lifegiver
  call perk_level_
  mov  dword ptr ds:[_Lifegiver], eax
  mov  eax, PERK_tag
  call perk_level_
  mov  dword ptr ds:[_Tag_], eax
  mov  eax, PERK_mutate
  call perk_level_
  mov  dword ptr ds:[_Mutate_], eax
  retn
 }
}

static void __declspec(naked) perk_can_add_hook() {
 __asm {
  imul edx, eax, 3
  add  edx, dword ptr [ecx+0xC]
  mov  eax, PCSTAT_level
  call stat_pc_get_
  cmp  eax, edx
  jge  end
  mov  eax, 0x486BC1
  jmp  eax
end:
  mov  edi, 0x486BC8
  jmp  edi
 }
}

static void __declspec(naked) FirstTurnAndNoEnemy() {
 __asm {
  xor  eax, eax
  test byte ptr ds:[_combat_state], 1
  jz   end                                  // Не в бою
  cmp  _combatNumTurns, eax
  jne  end                                  // Это не первый ход
  call combat_should_end_
  test eax, eax                             // Враги есть?
  jz   end                                  // Да
  pushad
  mov  ecx, ds:[_list_total]
  mov  edx, ds:[_obj_dude]
  mov  edx, [edx+0x50]                      // team_num группы поддержки игрока
  mov  edi, ds:[_combat_list]
loopCritter:
  mov  eax, [edi]                           // eax = персонаж
  mov  ebx, [eax+0x50]                      // team_num группы поддержки персонажа
  cmp  edx, ebx                             // Братюня?
  je   nextCritter                          // Да
  mov  eax, [eax+0x54]                      // who_hit_me
  test eax, eax                             // В персонажа стреляли?
  jz   nextCritter                          // Нет
  cmp  edx, [eax+0x50]                      // Стреляли из группы поддержки игрока?
  jne  nextCritter                          // Нет
  popad
  dec  eax                                  // Разборки!!!
  retn
nextCritter:
  add  edi, 4                               // К следующему персонажу в списке
  loop loopCritter                          // Перебираем весь список
  popad
end:
  retn
 }
}

sMessage cantdothat = {661, 0, 0};          // 'Слишком далеко.'
static void __declspec(naked) FirstTurnCheckDist() {
 __asm {
  push eax
  push edx
  call obj_dist_
  cmp  eax, 1                               // Расстояние до объекта больше 1?
  pop  edx
  pop  eax
  jle  end                                  // Нет
  lea  edx, cantdothat
  mov  eax, _proto_main_msg_file
  call message_search_
  cmp  eax, 1
  jne  skip
  mov  eax, cantdothat.message
  call display_print_
skip:
  pop  eax                                  // Уничтожаем адрес возврата
  xor  eax, eax
  dec  eax
end:
  retn
 }
}

static void __declspec(naked) check_move_hook() {
 __asm {
  call FirstTurnAndNoEnemy
  test eax, eax                             // Это первый ход в бою и врагов нет?
  jnz  skip                                 // Да
  cmp  dword ptr [ecx], -1
  je   end
  retn
skip:
  xor  esi, esi
  dec  esi
end:
  pop  eax                                  // Уничтожаем адрес возврата
  mov  eax, 0x417A0F
  jmp  eax
 }
}

static void __declspec(naked) gmouse_bk_process_hook1() {
 __asm {
  xchg ebp, eax
  call FirstTurnAndNoEnemy
  test eax, eax                             // Это первый ход в бою и врагов нет?
  jnz  end                                  // Да
  xchg ebp, eax
  cmp  eax, dword ptr [edx+0x40]
  jg   end
  retn
end:
  pop  eax                                  // Уничтожаем адрес возврата
  mov  eax, 0x443915
  jmp  eax
 }
}

static void __declspec(naked) FakeCombatFix1() {
 __asm {
  push eax                                  // _obj_dude
  call FirstTurnAndNoEnemy
  test eax, eax                             // Это первый ход в бою и врагов нет?
  pop  eax
  jz   end                                  // Нет
  call FirstTurnCheckDist
end:
  jmp  action_get_an_object_
 }
}

static void __declspec(naked) FakeCombatFix2() {
 __asm {
  push eax                                  // _obj_dude
  call FirstTurnAndNoEnemy
  test eax, eax                             // Это первый ход в бою и врагов нет?
  pop  eax
  jz   end                                  // Нет
  call FirstTurnCheckDist
end:
  jmp  action_loot_container_
 }
}

static void __declspec(naked) FakeCombatFix3() {
 __asm {
  cmp  dword ptr ds:[_obj_dude], eax
  jne  end
  push eax
  call FirstTurnAndNoEnemy
  test eax, eax                             // Это первый ход в бою и врагов нет?
  pop  eax
  jz   end                                  // Нет
  call FirstTurnCheckDist
end:
  jmp  action_use_an_item_on_object_
 }
}

static void __declspec(naked) combat_begin_hook() {
 __asm {
  xor  eax, eax
  mov  _combatNumTurns, eax
  dec  eax
  retn
 }
}

static void __declspec(naked) combat_reset_hook() {
 __asm {
  mov  _combatNumTurns, edx
  mov  edx, STAT_max_move_points
  retn
 }
}

static void __declspec(naked) combat_hook() {
 __asm {
  inc  _combatNumTurns
  jmp  combat_should_end_
 }
}

static DWORD MaxPCLevel = 21;
static void __declspec(naked) stat_pc_min_exp_hook() {
 __asm {
  inc  eax
  cmp  eax, MaxPCLevel
  jg   maxLevel
  push ebx
  mov  ebx, eax
  mov  edx, eax
  sar  edx, 0x1F
  sub  eax, edx
  sar  eax, 1
  test bl, 1
  jnz  skip
  dec  ebx
skip:
  imul ebx, eax
  mov  eax, ebx
  shl  eax, 5
  sub  eax, ebx
  shl  eax, 2
  add  eax, ebx
  shl  eax, 3
  pop  ebx
  jmp  end
maxLevel:
  xor  eax, eax
  dec  eax
end:
  pop  edx                                  // Уничтожаем адрес возврата
  pop  edx
  retn
 }
}

FILETIME ftCurr, ftPrev;
static void _stdcall _GetFileTime(char* filename) {
 char fname[65];
 sprintf_s(fname, "%s%s", "data\\", filename);
 HANDLE hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
 if (hFile != INVALID_HANDLE_VALUE) {
  GetFileTime(hFile, NULL, NULL, &ftCurr);
  CloseHandle(hFile);
 } else {
  ftCurr.dwHighDateTime = 0;
  ftCurr.dwLowDateTime = 0;
 };
}

static const char* commentFmt="%02d/%02d/%d  %02d:%02d:%02d";
static void _stdcall createComment(char* bufstr) {
 SYSTEMTIME stUTC, stLocal;
 char buf[30];
 GetSystemTime(&stUTC);
 SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
 sprintf_s(buf, commentFmt, stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
 strcpy(bufstr, buf);
}

static DWORD AutoQuickSave = 0;
static void __declspec(naked) SaveGame_hook() {
 __asm {
  pushad
  mov  ecx, dword ptr ds:[_slot_cursor]
  mov  dword ptr ds:[_flptr], eax
  test eax, eax
  jz   end                                  // Это пустой слот, можно записывать
  call db_fclose_
  push ecx
  push edi
  call _GetFileTime
  pop  ecx
  mov  edx, ftCurr.dwHighDateTime
  mov  ebx, ftCurr.dwLowDateTime
  jecxz nextSlot                            // Это первый слот
  cmp  edx, ftPrev.dwHighDateTime
  ja   nextSlot                             // Текущий слот записан позже предыдущего
  jb   end                                  // Текущий слот записан раньше предыдущего
  cmp  ebx, ftPrev.dwLowDateTime
  jbe  end
nextSlot:
  mov  ftPrev.dwHighDateTime, edx
  mov  ftPrev.dwLowDateTime, ebx
  inc  ecx
  cmp  ecx, AutoQuickSave                   // Последний слот+1?
  ja   firstSlot                            // Да
  mov  dword ptr ds:[_slot_cursor], ecx
  popad
  mov  eax, 0x46DEB8
  jmp  eax
firstSlot:
  xor  ecx, ecx
end:
  mov  dword ptr ds:[_slot_cursor], ecx
  mov  eax, ecx
  shl  eax, 4
  add  eax, ecx
  shl  eax, 3
  add  eax, _LSData+0x3D                   // eax->_LSData[_slot_cursor].Comment
  push eax
  call createComment
  popad
  xor  edx, edx
  inc  edx
  mov  dword ptr ds:[_quick_done], edx
  mov  edx, 0x46DF33
  jmp  edx
 }
}

static DWORD RemoveFriendlyFoe = 0;
static DWORD ColorLOS = 0;
static void __declspec(naked) combat_update_critter_outline_for_los() {
 __asm {
  pushad
  xchg esi, eax                             // esi = target
  mov  eax, [esi+0x64]
  shr  eax, 0x18
  cmp  eax, ObjType_Critter
  jne  end                                  // Нет
  mov  ecx, ds:[_obj_dude]
  cmp  ecx, esi                             // Это игрок?
  je   end                                  // Да
  mov  eax, esi
  call critter_is_dead_
  test eax, eax                             // Это труп?
  jnz  end                                  // Да
  mov  edi, ds:[_combat_highlight]
  push ecx
  push eax
  mov  eax, esi
  xchg ecx, eax                             // ecx = target, eax=who (_obj_dude)
  mov  ebx, [ecx+0x4]                       // target_tile
  mov  edx, [eax+0x4]                       // who_tile
  call combat_is_shot_blocked_
  pop  edx
  xor  ecx, ecx
  mov  ebp, [esi+0x74]                      // outline
  and  ebp, 0xFFFFFF                        // ebp=текущий цвет контура
  test eax, eax                             // Есть преграды?
  jnz  itsLOS                               // Да
// Прямая видимость
  inc  ecx
  cmp  ebp, ecx                             // красный переливающийся
  je   alreadyOutlined
  cmp  ebp, 8                               // зелёный переливающийся
  je   alreadyOutlined
  mov  ebp, ecx                             // красный переливающийся
  cmp  RemoveFriendlyFoe, eax               // Игнорировать Friendly Foe?
  jne  skipFriendlyFoe                      // Да
  mov  eax, PERK_friendly_foe
  call perk_level_
  test eax, eax
  jz   noFriend
skipFriendlyFoe:
  mov  eax, [edx+0x50]                      // team_num
  cmp  eax, [esi+0x50]                      // Братюня?
  jnz  noFriend                             // Нет
  add  ebp, 7                               // зелёный переливающийся
noFriend:
  jmp  setOutlined
itsLOS:
  xchg edx, eax                             // eax = _obj_dude
  push eax
  mov  edx, esi
  call obj_dist_
  xchg ebx, eax                             // ebx = расстояние от игрока до цели
  mov  edx, STAT_pe
  pop  eax
  call stat_level_
  lea  edx, ds:0[eax*4]
  add  edx, eax                             // edx = Perception * 5
  test byte ptr [esi+0x26], 2               // TransGlass_
  jz   noGlass
  mov  eax, edx
  sar  edx, 0x1F
  sub  eax, edx
  sar  eax, 1                               // edx = (Perception * 5) / 2
  mov  edx, eax
noGlass:
  cmp  ebx, edx
  jg   outRange
  inc  ecx
outRange:
  cmp  ebp, ColorLOS
  je   alreadyOutlined
  mov  ebp, ColorLOS
setOutlined:
  mov  eax, esi
  xor  edx, edx
  call obj_turn_off_outline_
  mov  eax, esi
  xor  edx, edx
  call obj_remove_outline_
  test ecx, ecx
  jz   end
  mov  edx, ebp
  mov  eax, esi
  xor  ebx, ebx
  call obj_outline_object_
  xchg esi, eax
  xor  edx, edx
  test edi, edi
  jz   turn_off_outline
  jmp  turn_on_outline
alreadyOutlined:
  mov  eax, esi
  xor  edx, edx
  test byte ptr [esi+0x77], 0x80            // OutlineOff_
  jz   turn_off_outline
turn_on_outline:
  test edi, edi
  jz   end
  call obj_turn_on_outline_
  jmp  end
turn_off_outline:
  test edi, edi
  jnz  end
  call obj_turn_off_outline_
end:
  popad
  retn
 }
}

static void __declspec(naked) obj_move_to_tile_hook() {
 __asm {
  test byte ptr ds:[_combat_state], 1
  jz   end                                  // Не в бою
  mov  eax, [esp+0x44]
  call combat_update_critter_outline_for_los
end:
  mov  ebx, [esp+0x4C]
  test ebx, ebx
  mov  eax, 0x47C7B0
  jmp  eax
 }
}

void __declspec(naked) queue_find_first_() {
 __asm {
  push ecx
// eax = who, edx = type
  mov  ecx, dword ptr ds:[_queue]
loopQueue:
  jecxz skip
  cmp  eax, dword ptr [ecx+0x8]             // queue.object
  jne  nextQueue
  cmp  edx, dword ptr [ecx+0x4]             // queue.type
  jne  nextQueue
  mov  eax, dword ptr [ecx+0xC]             // queue.data
  jmp  end
nextQueue:
  mov  ecx, dword ptr [ecx+0x10]            // queue.next_queue
  jmp  loopQueue
skip:
  xor  eax, eax
end:
  mov  _tmpQNode, ecx
  pop  ecx
  retn
 }
}

void __declspec(naked) queue_find_next_() {
 __asm {
  push ecx
// eax = who, edx = type
  mov  ecx, _tmpQNode
  jecxz skip
loopQueue:
  mov  ecx, dword ptr [ecx+0x10]            // queue.next_queue
  jecxz skip
  cmp  eax, dword ptr [ecx+0x8]             // queue.object
  jne  loopQueue
  cmp  edx, dword ptr [ecx+0x4]             // queue.type
  jne  loopQueue
  mov  eax, dword ptr [ecx+0xC]             // queue.data
  jmp  end
skip:
  xor  eax, eax
end:
  mov  _tmpQNode, ecx
  pop  ecx
  retn
 }
}

static void __declspec(naked) print_with_linebreak() {
 __asm {
  push esi
  push ecx
  test eax, eax                             // А есть строка?
  jz   end                                  // Нет
  mov  esi, eax
  xor  ecx, ecx
loopString:
  cmp  byte ptr [esi], 0                    // Конец строки
  je   printLine                            // Да
  cmp  byte ptr [esi], 0x5C                 // Возможно перевод строки? '\'
  jne  nextChar                             // Нет
  cmp  byte ptr [esi+1], 0x6E               // Точно перевод строки? 'n'
  jne  nextChar                             // Нет
  inc  ecx
  mov  byte ptr [esi], 0
printLine:
  call edi
  jecxz end
  dec  ecx
  mov  byte ptr [esi], 0x5C
  inc  esi
  mov  eax, esi
  inc  eax
nextChar:
  inc  esi
  jmp  loopString
end:
  pop  ecx
  pop  esi
  retn
 }
}

static void __declspec(naked) display_print_with_linebreak() {
 __asm {
  push edi
  mov  edi, display_print_
  call print_with_linebreak
  pop  edi
  retn
 }
}

static void __declspec(naked) inven_display_msg_with_linebreak() {
 __asm {
  push edi
  mov  edi, inven_display_msg_
  call print_with_linebreak
  pop  edi
  retn
 }
}

static int drugExploit = 0;
static void __declspec(naked) protinst_use_item_hook() {
 __asm {
  dec  drugExploit
  call obj_use_book_
  inc  drugExploit
  retn
 }
}

static void __declspec(naked) UpdateLevel_hook() {
 __asm {
  inc  drugExploit
  call perks_dialog_
  dec  drugExploit
  retn
 }
}

static void __declspec(naked) skill_level_hook() {
 __asm {
  dec  drugExploit
  call skill_level_
  inc  drugExploit
  retn
 }
}

static void __declspec(naked) SliderBtn_hook() {
 __asm {
  dec  drugExploit
  call skill_inc_point_
  inc  drugExploit
  retn
 }
}

static void __declspec(naked) SliderBtn_hook1() {
 __asm {
  dec  drugExploit
  call skill_dec_point_
  inc  drugExploit
  retn
 }
}

static void __declspec(naked) stat_level_hook() {
 __asm {
  call stat_get_bonus_
  cmp  ebx, STAT_lu                         // Проверяем только силу-удачу
  ja   end
//  test eax, eax                             // А есть хоть какой [+/-]бонус?
//  jz   end                                  // Нет
  cmp  drugExploit, 0                       // Вызов из нужных мест?
  jl   checkPenalty                         // Проверка чтения книг/скилла
  jg   noBonus                              // Получение перков
  retn
checkPenalty:
  cmp  eax, 1                               // Положительный эффект?
  jge  end                                  // Да - учитываем его
noBonus:
  xor  eax, eax                             // Не учитываем эффект от наркотиков/радиации/etc
end:
  retn
 }
}

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
  mov  eax, 0x467FE8
  jmp  eax                                  // А есть ли ещё включённые предметы среди продаваемых?
 }
}

static void __declspec(naked) item_m_turn_off_hook() {
 __asm {
  and  byte ptr [eax+0x25], 0xDF            // Сбросим флаг использованного предмета
  jmp  queue_remove_this_
 }
}

static void DllMain2() {
 DWORD tmp;
 dlogr("In DllMain2", DL_MAIN);

 if (GetPrivateProfileIntA("Speed", "Enable", 0, ini)) {
  AddrGetTickCount = (DWORD)&FakeGetTickCount;
  AddrGetLocalTime = (DWORD)&FakeGetLocalTime;

  for (int i = 0; i < sizeof(offsetsA)/4; i++) {
   SafeWrite32(offsetsA[i], (DWORD)&AddrGetTickCount);
  }
  for (int i = 0; i < sizeof(offsetsB)/4; i++) {
   SafeWrite32(offsetsB[i], (DWORD)&AddrGetTickCount);
  }
  for (int i = 0; i < sizeof(offsetsC)/4; i++) {
   SafeWrite32(offsetsC[i], (DWORD)&AddrGetTickCount);
  }

  SafeWrite32(0x4E0CC0, (DWORD)&AddrGetLocalTime);
  TimerInit();
 }

 dlog("Applying input patch.", DL_INIT);
 SafeWriteStr(0x4FE3A4, "ddraw.dll");
 dlogr(" Done", DL_INIT);

 GraphicsMode = GetPrivateProfileIntA("Graphics", "Mode", 0, ini);
 if (GraphicsMode != 4 && GraphicsMode != 5) GraphicsMode = 0;
 if (GraphicsMode == 4 || GraphicsMode == 5) {
  dlog("Applying dx9 graphics patch.", DL_INIT);
  HMODULE h = LoadLibraryEx("d3dx9_43.dll", 0, LOAD_LIBRARY_AS_DATAFILE);
  if (!h) {
   MessageBoxA(0, "You have selected graphics mode 4 or 5, but d3dx9_43.dll is missing\nSwitch back to mode 0, or install an up to date version of DirectX", "Error", 0);
   ExitProcess(-1);
  } else {
   FreeLibrary(h);
  }
  SafeWrite8(0x4FE39F, '2');
  dlogr(" Done", DL_INIT);
 }

 AmmoModInit();

 mapName[64] = 0;
 if (GetPrivateProfileString("Misc", "StartingMap", "", mapName, 64, ini)) {
  dlog("Applying starting map patch.", DL_INIT);
  SafeWrite32(0x472B42, (DWORD)&mapName);
  dlogr(" Done", DL_INIT);
 }

 versionString[64] = 0;
 if (GetPrivateProfileString("Misc", "VersionString", "", versionString, 64, ini)) {
  dlog("Applying version string patch.", DL_INIT);
  SafeWrite32(0x4A15D8, (DWORD)&versionString);
  dlogr(" Done", DL_INIT);
 }

 windowName[64] = 0;
 if (GetPrivateProfileString("Misc", "WindowName", "", windowName, 64, ini)) {
  dlog("Applying window name patch.", DL_INIT);
  SafeWrite32(0x472D1B, (DWORD)&windowName);
  dlogr(" Done", DL_INIT);
 }

 configName[64] = 0;
 if (GetPrivateProfileString("Misc", "ConfigFile", "", configName, 64, ini)) {
  dlog("Applying config file patch.", DL_INIT);
  SafeWrite32(0x43E0FC, (DWORD)&configName);
  SafeWrite32(0x43E121, (DWORD)&configName);
  dlogr(" Done", DL_INIT);
 }

 dmModelName[64] = 0;
 GetPrivateProfileString("Misc", "MaleDefaultModel", "hmjmps", dmModelName, 64, ini);
 SafeWrite32(0x41840E, (DWORD)&dmModelName);

 dfModelName[64]=0;
 GetPrivateProfileString("Misc", "FemaleDefaultModel", "hfjmps", dfModelName, 64, ini);
 SafeWrite32(0x418431, (DWORD)&dfModelName);

 for (int i = 0; i < 14; i++) {
  MovieNames[i*65+64] = 0;
  char ininame[8];
  strcpy_s(ininame, "Movie");
  _itoa_s(i+1, &ininame[5], 3, 10);
  GetPrivateProfileString("Misc", ininame, origMovieNames[i], &MovieNames[i*65], 64, ini);
  SafeWrite32(0x5054F0 + i*4, (DWORD)&MovieNames[i*65]);
 }

 tmp = GetPrivateProfileInt("Misc", "StartYear", -1, ini);
 if (tmp != -1) {
  dlog("Applying starting year patch.", DL_INIT);
  SafeWrite32(0x491C1C, tmp);
  dlogr(" Done", DL_INIT);
 }

 tmp = GetPrivateProfileInt("Misc", "StartMonth", -1, ini);
 if (tmp != -1) {
  dlog("Applying starting month patch.", DL_INIT);
  SafeWrite32(0x491C32, tmp);
  dlogr(" Done", DL_INIT);
 }

 tmp = GetPrivateProfileInt("Misc", "StartDay", -1, ini);
 if (tmp != -1) {
  dlog("Applying starting day patch.", DL_INIT);
  SafeWrite8(0x491C06, byte(tmp));
  dlogr(" Done", DL_INIT);
 }

 tmp = GetPrivateProfileInt("Misc", "LocalMapXLimit", 0, ini);
 if (tmp) {
  dlog("Applying local map x limit patch.", DL_INIT);
  SafeWrite32(0x49E4AD, tmp);
  dlogr(" Done", DL_INIT);
 }

 tmp = GetPrivateProfileInt("Misc", "LocalMapYLimit", 0, ini);
 if (tmp) {
  dlog("Applying local map y limit patch.", DL_INIT);
  SafeWrite32(0x49E4BB, tmp);
  dlogr(" Done", DL_INIT);
 }

 if (GetPrivateProfileIntA("Misc", "DialogueFix", 1, ini)) {
  dlog("Applying dialogue patch.", DL_INIT);
  SafeWrite8(0x43F296, 0x31);
  dlogr(" Done", DL_INIT);
 }

 CritInit();

 if (GetPrivateProfileInt("Misc", "DisplayKarmaChanges", 0, ini)) {
  dlog("Applying display karma changes patch. ", DL_INIT);
  GetPrivateProfileString("sfall", "KarmaGain", "You gained %d karma.", KarmaGainMsg, 128, translationIni);
  GetPrivateProfileString("sfall", "KarmaLoss", "You lost %d karma.", KarmaLossMsg, 128, translationIni);
  HookCall(0x44D09C, &op_set_global_var_hook);
  dlogr(" Done", DL_INIT);
 }

 if (GetPrivateProfileInt("Misc", "PlayIdleAnimOnReload", 0, ini)) {
  dlog("Applying idle anim on reload patch. ", DL_INIT);
  HookCall(0x4565A9, &intface_item_reload_hook);
  dlogr(" Done", DL_INIT);
 }

 idle = GetPrivateProfileIntA("Misc", "ProcessorIdle", -1, ini);

 if (GetPrivateProfileIntA("Misc", "SkipOpeningMovies", 0, ini)) {
  dlog("Blocking opening movies. ", DL_INIT);
  SafeWrite16(0x472A73, 0x13EB);            // jmps 0x472A88
  dlogr(" Done", DL_INIT);
 }

 RetryCombatMinAP = GetPrivateProfileIntA("Misc", "NPCsTryToSpendExtraAP", 0, ini);
 if (RetryCombatMinAP) {
  dlog("Applying retry combat patch. ", DL_INIT);
  HookCall(0x420B0A, &combat_turn_hook);
  dlogr(" Done", DL_INIT);
 }

 if (GetPrivateProfileIntA("Misc", "RemoveWindowRounding", 0, ini)) {
  SafeWrite32(0x4A50C0, 0x90909090);
  SafeWrite16(0x4A50C4, 0x9090);
 }

 dlogr("Running ConsoleInit().", DL_INIT);
 ConsoleInit();

 if (GetPrivateProfileIntA("Misc", "SpeedInterfaceCounterAnims", 0, ini)) {
  dlog("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
  MakeCall(0x4565C1, &intface_rotate_numbers_hook, true);
  dlogr(" Done", DL_INIT);
 }

 tmp = GetPrivateProfileIntA("Misc", "SpeedInventoryPCRotation", 166, ini);
 if (tmp != 166 && tmp <= 1000) {
  dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
  SafeWrite32(0x46432B, tmp);
  dlogr(" Done", DL_INIT);
 }

 if (GetPrivateProfileIntA("Misc", "RemoveCriticalTimelimits", 0, ini)) {
  dlog("Removing critical time limits.", DL_INIT);
  SafeWrite8(0x491903, 0x0);
  SafeWrite8(0x491944, 0x0);
  dlogr(" Done", DL_INIT);
 }

 dlogr("Patching out ereg call.", DL_INIT);
 BlockCall(0x43B3A8);

 tmp = GetPrivateProfileIntA("Misc", "CombatPanelAnimDelay", 1000, ini);
 if (tmp >= 0 && tmp <= 65535) {
  dlog("Applying CombatPanelAnimDelay patch.", DL_INIT);
  SafeWrite32(0x455556, tmp);
  SafeWrite32(0x4556AB, tmp);
  dlogr(" Done", DL_INIT);
 };

 tmp = GetPrivateProfileIntA("Misc", "DialogPanelAnimDelay", 33, ini);
 if (tmp >= 0 && tmp <= 255) {
  dlog("Applying DialogPanelAnimDelay patch.", DL_INIT);
  SafeWrite32(0x4403F0, tmp);
  SafeWrite32(0x4404B2, tmp);
  dlogr(" Done", DL_INIT);
 }

 tmp = GetPrivateProfileIntA("Debugging", "DebugMode", 0, ini);
 if (tmp && *((DWORD*)0x43DFE3) == 0xFFFE8B69 && *((DWORD*)0x43DFE7) == 0x0F01FE83) {
  dlog("Applying DebugMode patch.", DL_INIT);
  HookCall(0x43DFE2, &DebugMode);
  SafeWrite8(0x4B338B, 0xB8);
  if (tmp == 1) SafeWrite32(0x4B338C, 0x4FE18C);
  else SafeWrite32(0x4B338C, 0x4FE170);
  dlogr(" Done", DL_INIT);
 }

 if (GetPrivateProfileIntA("Misc", "SingleCore", 1, ini)) {
  dlog("Applying single core patch.", DL_INIT);
  HANDLE process = GetCurrentProcess();
  SetProcessAffinityMask(process, 1);
  CloseHandle(process);
  dlogr(" Done", DL_INIT);
 }

 //Bodypart hit chances
 *((DWORD*)0x4FED84) = GetPrivateProfileIntA("Misc", "BodyHit_Head",      0xFFFFFFD8, ini);
 *((DWORD*)0x4FED88) = GetPrivateProfileIntA("Misc", "BodyHit_Left_Arm",  0xFFFFFFE2, ini);
 *((DWORD*)0x4FED8C) = GetPrivateProfileIntA("Misc", "BodyHit_Right_Arm", 0xFFFFFFE2, ini);
 *((DWORD*)0x4FED90) = GetPrivateProfileIntA("Misc", "BodyHit_Torso",     0x00000000, ini);
 *((DWORD*)0x4FED94) = GetPrivateProfileIntA("Misc", "BodyHit_Right_Leg", 0xFFFFFFEC, ini);
 *((DWORD*)0x4FED98) = GetPrivateProfileIntA("Misc", "BodyHit_Left_Leg",  0xFFFFFFEC, ini);
 *((DWORD*)0x4FED9C) = GetPrivateProfileIntA("Misc", "BodyHit_Eyes",      0xFFFFFFC4, ini);
 *((DWORD*)0x4FEDA0) = GetPrivateProfileIntA("Misc", "BodyHit_Groin",     0xFFFFFFE2, ini);
 *((DWORD*)0x4FEDA4) = GetPrivateProfileIntA("Misc", "BodyHit_Uncalled",  0x00000000, ini);

 toggleHighlightsKey = GetPrivateProfileIntA("Input", "ToggleItemHighlightsKey", 0, ini);
 if (toggleHighlightsKey) {
  dlog("Applying ToggleItemHighlightsKey patch.", DL_INIT);
  HookCall(0x443A05, &gmouse_bk_process_hook);
  HookCall(0x443C7A, &obj_remove_outline_hook);
  HookCall(0x446445, &obj_remove_outline_hook);
  TurnHighlightContainers = GetPrivateProfileIntA("Input", "TurnHighlightContainers", 0, ini);
  dlogr(" Done", DL_INIT);
 }

 HookCall(0x472ECB, &get_input_hook);       //hook the main game loop
 HookCall(0x42087E, &get_input_hook);       //hook the combat loop

 dlog("Initing main menu patches.", DL_INIT);
 MainMenuInit();
 dlogr(" Done", DL_INIT);

 CreditsInit();

 if(GetPrivateProfileIntA("Misc", "DisablePipboyAlarm", 0, ini)) {
  SafeWrite8(0x489334, 0xC3);               // retn
 }

 LoadGameHookInit();

 dlog("Running InventoryInit.", DL_INIT);
 InventoryInit();
 dlogr(" Done", DL_INIT);

 dlog("Initing AI control.", DL_INIT);
 PartyControlInit();
 dlogr(" Done", DL_INIT);

 char xltcodes[512];
 if (GetPrivateProfileStringA("Misc", "XltTable", "", xltcodes, 512, ini) > 0) {
  char *xltcode;
  int count = 0;
  xltcode = strtok(xltcodes, ",");
  while (xltcode) {
   int _xltcode = atoi(xltcode);
   if (_xltcode<32 || _xltcode>255) break;
   XltTable[count] = byte(_xltcode);
   if (count == 93) break;
   count++;
   xltcode = strtok(0, ",");
  }
  if (count == 93) {
   XltKey = GetPrivateProfileIntA("Misc", "XltKey", 4, ini);
   if (XltKey!=4 && XltKey!=2 && XltKey!=1) XltKey=4;
   MakeCall(0x42E0B6, &get_input_str_hook, true);
   SafeWrite8(0x42E04E, 0x7D);
   MakeCall(0x471784, &get_input_str2_hook, true);
   SafeWrite8(0x47171C, 0x7D);
   MakeCall(0x4B7060, &kb_next_ascii_English_US_hook, true);
   if (*((DWORD*)0x442BC3) == 0x89031488) {
    HookCall(0x442B86, &about_process_input_hook);
    HookCall(0x442FF2, &stricmp_hook);
    HookCall(0x44304C, &stricmp_hook);
   }
  }
 }

// Клавиши навигации в пипбое
 HookCall(0x486F4E, &pipboy_hook);

// Третий вариант концовки для Братства Стали
 MakeCall(0x438B80, &Brotherhood_final, true);

// Отключение пропадания неиспользованного перка
 SafeWrite8(0x436253, 0x80);                // add  byte ptr ds:_free_perk, 1
 SafeWrite16(0x4362AA, 0x0DFE);             // dec  byte ptr ds:_free_perk
 SafeWrite8(0x4362B1, 0xB1);                // jmp  0x436263
 HookCall(0x42C69E, &editor_design_hook);
 HookCall(0x42CC34, &editor_design_hook2);
 HookCall(0x436974, &perks_dialog_hook);
 MakeCall(0x486BB2, &perk_can_add_hook, true);

// Приподнимем окно перков
 SafeWrite8(0x4364A7, 31);                  // 91-60=31
 SafeWrite8(0x436A5C, 74);                  // 134-60=74

// Уменьшение дистанции переключения на ходьбу при клике на предмет
 for (int i = 0; i < sizeof(WalkDistance)/4; i++) {
  SafeWrite8(WalkDistance[i], 1);
 }

// fix "Pressing A to enter combat before anything else happens, thus getting infinite free running"
 if (GetPrivateProfileInt("Misc", "FakeCombatFix", 0, ini)) {
  MakeCall(0x4179A2, &check_move_hook, false);
  MakeCall(0x4438F9, &gmouse_bk_process_hook1, false);
  HookCall(0x444088, &FakeCombatFix1);       // action_get_an_object_
  HookCall(0x4446BB, &FakeCombatFix1);       // action_get_an_object_
  HookCall(0x444131, &FakeCombatFix2);       // action_loot_container_
  HookCall(0x4446A7, &FakeCombatFix2);       // action_loot_container_
  HookCall(0x411F7B, &FakeCombatFix3);       // action_use_an_object_
  HookCall(0x444270, &FakeCombatFix3);       // gmouse_handle_event_
  HookCall(0x4654F0, &FakeCombatFix3);       // use_inventory_on_
  MakeCall(0x41FF2A, &combat_begin_hook, false);
  MakeCall(0x41FAA3, &combat_reset_hook, false);
  HookCall(0x420D79, &combat_hook);
 }

// Минимальный возраст игрока
 SafeWrite8(0x5082DC, 8);

// Максимальный возраст игрока
 SafeWrite8(0x5082E0, 60);

 if (GetPrivateProfileIntA("Misc", "EnableMusicInDialogue", 0, ini)) {
  SafeWrite8(0x43E74C, 0x00);
//  BlockCall(0x4483B7);
 }

// Максимальный уровень игрока
 MaxPCLevel = GetPrivateProfileIntA("Misc", "MaxPCLevel", 21, ini);
 if (MaxPCLevel != 21 && MaxPCLevel >= 1 && MaxPCLevel <= 99) {
  SafeWrite8(0x4361A5, byte(MaxPCLevel));
  SafeWrite8(0x49D1AE, byte(MaxPCLevel));
  MakeCall(0x49D072, &stat_pc_min_exp_hook, false);
  // Максимальное количество получаемых перков
  SafeWrite8(0x436221, 33);
  SafeWrite8(0x43622C, 33);
  SafeWrite8(0x437DD4, 33);
 }

 AutoQuickSave = GetPrivateProfileIntA("Misc", "AutoQuickSave", 0, ini);
 if (AutoQuickSave >= 1 && AutoQuickSave <= 10) {
  AutoQuickSave--;
  SafeWrite16(0x46DEAB, 0xC031);            // xor  eax, eax
  SafeWrite32(0x46DEAD, 0x505A50A3);        // mov  ds:_slot_cursor, eax
  SafeWrite16(0x46DEB2, 0x04EB);            // jmp  0x46DEB8
  MakeCall(0x46DF13, &SaveGame_hook, true);
 }

 if (GetPrivateProfileIntA("Misc", "DontTurnOffSneakIfYouRun", 0, ini)) {
  SafeWrite8(0x417A7F, 0xEB);
 }

 ColorLOS = GetPrivateProfileIntA("Misc", "ColorLOS", 0, ini);
 if (ColorLOS == 2 || ColorLOS == 4 || ColorLOS == 16 || ColorLOS == 32) {
  HookCall(0x42434F, &combat_update_critter_outline_for_los);
  HookCall(0x4243E9, &combat_update_critter_outline_for_los);
  MakeCall(0x47C7AA, &obj_move_to_tile_hook, true);
 }

// Можно использовать управляющий символ новой строки (\n) в описании объектов из pro_*.msg
 SafeWrite32(0x462D6F, (DWORD)&display_print_with_linebreak);
 SafeWrite32(0x48A7DA, (DWORD)&display_print_with_linebreak);
 SafeWrite32(0x46664A, (DWORD)&inven_display_msg_with_linebreak);

 RemoveFriendlyFoe = GetPrivateProfileIntA("Misc", "RemoveFriendlyFoe", 0, ini);
 if (RemoveFriendlyFoe != 0) {
  SafeWrite32(0x506C90, 100);
  SafeWrite8(0x420027, 0x0);
  SafeWrite8(0x4243C0, 0x0);
  SafeWrite8(0x42612E, 0x0);
 }

 if (GetPrivateProfileIntA("Misc", "DrugExploitFix", 0, ini)) {
  HookCall(0x48B604, &protinst_use_item_hook);
  HookCall(0x436283, &UpdateLevel_hook);
  HookCall(0x434B28, &skill_level_hook);    // SavePlayer_
  HookCall(0x435670, &SliderBtn_hook);
  HookCall(0x4356D7, &skill_level_hook);    // SliderBtn_
  HookCall(0x4356F0, &SliderBtn_hook1);
  HookCall(0x49CA53, &stat_level_hook);
 }

 dlog("Running BugsInit.", DL_INIT);
 BugsInit();
 dlogr(" Done", DL_INIT);

 dlog("Running QuestsInit.", DL_INIT);
 QuestsInit();
 dlogr(" Done", DL_INIT);

// Исправление невозможности продажи ранее использованных "Счетчик Гейгера"/"Невидимка"
 if (GetPrivateProfileIntA("Misc", "CanSellUsedGeiger", 0, ini)) {
  SafeWrite8(0x46ADFA, 0xBA);
  SafeWrite8(0x46AE2C, 0xBA);
  MakeCall(0x467FF3, &barter_attempt_transaction_hook, false);
  HookCall(0x46C0B9, &item_m_turn_off_hook);
 }

 dlogr("Leave DllMain2", DL_MAIN);  
}

static void _stdcall OnExit() {
 ConsoleExit();
}

static void __declspec(naked) _WinMain_hook() {
 __asm {
  pushad
  call OnExit
  popad
  jmp  DOSCmdLineDestroy_
 }
}

bool _stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
 if (dwReason == DLL_PROCESS_ATTACH) {

#ifdef TRACE
  LoggingInit();
#endif

  HookCall(0x4DE7D2, &_WinMain_hook);

  char filepath[MAX_PATH];
  GetModuleFileName(0, filepath, MAX_PATH);

  CRC(filepath);

  bool cmdlineexists = false;
  char* cmdline = GetCommandLineA();
  if (GetPrivateProfileIntA("Main", "UseCommandLine", 0, ".\\ddraw.ini")) {
   while(cmdline[0] == ' ') cmdline++;
   bool InQuote = false;
   int count = -1;

   while (true) {
    count++;
    if (cmdline[count] == 0) break;;
    if (cmdline[count] == ' ' && !InQuote) break;
    if (cmdline[count] == '"') {
     InQuote = !InQuote;
     if (!InQuote) break;
    }
   }
   if (cmdline[count] != 0) {
    count++;
    while (cmdline[count] == ' ') count++;
    cmdline = &cmdline[count];
    cmdlineexists = true;
   }
  }

  if (cmdlineexists && strlen(cmdline)) {
   strcpy_s(ini, ".\\");
   strcat_s(ini, cmdline);
   HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
   if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
   else {
    MessageBox(0, "You gave a command line argument to fallout, but it couldn't be matched to a file\n" \
     "Using default ddraw.ini instead", "Warning", MB_TASKMODAL);
    strcpy_s(ini, ".\\ddraw.ini");
   }
  } else strcpy_s(ini, ".\\ddraw.ini");

  GetPrivateProfileStringA("Main", "TranslationsINI", "./Translations.ini", translationIni, 65, ini);

  DllMain2();
 }
 return true;
}
