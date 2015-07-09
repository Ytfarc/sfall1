//#define STRICT
//#define WIN32_LEAN_AND_MEAN

#include "main.h"

#include "Define.h"
#include "FalloutEngine.h"
#include "Graphics.h"
#include "input.h"
#include "PartyControl.h"

static void __declspec(naked) ResetState() {
 __asm {
  pushad
  xor  eax, eax
  mov  ForcingGraphicsRefresh, eax
  cmp  GraphicsMode, 3
  jbe  end
  call graphics_OnGameLoad
end:
  call PartyControlReset
  popad
  retn
 }
}

static void __declspec(naked) LoadGame_hook() {
 __asm {
  call ResetState
  jmp  LoadSlot_
 }
}

static void __declspec(naked) gnw_main_hook() {
 __asm {
  call ResetState
  jmp  main_menu_loop_
 }
}

static void __declspec(naked) gnw_main_hook1() {
 __asm {
  call ResetState
  jmp  main_game_loop_
 }
}

static char SaveFailMsg[128];
static DWORD SaveInCombatFix = 0;
static const DWORD SaveGame_hook_End = 0x46EB16;
static void __declspec(naked) SaveGame_hook() {
 __asm {
  xor  esi, esi
  test byte ptr ds:[_combat_state], 1
  jz   skip                                 // Не в бою
  cmp  IsControllingNPC, esi
  jne  end
  cmp  SaveInCombatFix, esi
  je   skip
  cmp  SaveInCombatFix, 2
  je   end
  pushad
  mov  eax, ds:[_obj_dude]
  mov  ebx, [eax+0x40]                      // curr_mp
  mov  edx, STAT_max_move_points
  call stat_level_
  cmp  eax, ebx
  jne  restore
  mov  eax, PERK_bonus_move
  call perk_level_
  shl  eax, 1
  cmp  eax, ds:[_combat_free_move]
  jne  restore
  popad
skip:
  dec  esi
  retn
restore:
  popad
end:
  mov  eax, offset SaveFailMsg
  call display_print_
  pop  eax                                  // Уничтожаем адрес возврата
  jmp  SaveGame_hook_End
 }
}

void LoadGameHookInit() {
 HookCall(0x46EC9C, LoadGame_hook);
 HookCall(0x46F712, LoadGame_hook);
 HookCall(0x472AD7, gnw_main_hook);
 HookCall(0x472B4B, gnw_main_hook1);
 GetPrivateProfileString("sfall", "SaveInCombat", "Cannot save at this time.", SaveFailMsg, 128, translationIni);
 MakeCall(0x46DE37, &SaveGame_hook, false);
 SaveInCombatFix = GetPrivateProfileInt("Misc", "SaveInCombatFix", 1, ini);
 if (SaveInCombatFix < 0 || SaveInCombatFix > 2) SaveInCombatFix = 0;
}
