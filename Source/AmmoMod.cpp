/*
*    sfall
*    Copyright (C) 2008, 2009, 2010, 2013, 2014  The sfall team
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

static void __declspec(naked) item_w_damage_hook() {
 __asm {
  cmp  ebx, dword ptr ds:[_obj_dude]        // _obj_dude
  jne  skip
  mov  eax, PERK_bonus_hth_damage
  call perk_level_
  shl  eax, 1
skip:
  add  eax, 1
  xchg ebp, eax
  retn
 }
}

static void __declspec(naked) item_w_damage_hook1() {
 __asm {
  call stat_level_
  cmp  ebx, dword ptr ds:[_obj_dude]        // _obj_dude
  jne  skip
  push eax
  mov  eax, PERK_bonus_hth_damage
  call perk_level_
  shl  eax, 1
  add  dword ptr [esp+0x4+0x8], eax         // min_dmg
  pop  eax
skip:
  retn
 }
}

static void __declspec(naked) display_stats_hook() {
 __asm {
  mov  eax, PERK_bonus_hth_damage
  call perk_level_
  shl  eax, 1
  add  dword ptr [esp+4*4], eax             // min_dmg
  jmp  sprintf_
 }
}

static const DWORD display_stats_hook1_End = 0x465E94;
static void __declspec(naked) display_stats_hook1() {
 __asm {
  call stat_level_
  add  eax, 2
  push eax
  mov  eax, PERK_bonus_hth_damage
  call perk_level_
  shl  eax, 1
  add  eax, 1
  push eax
  mov  eax, dword ptr [esp+0x94+0x4]
  push eax
  push 0x4F92AC                             // '%s %d-%d'
  lea  eax, [esp+0xC+0x4]
  push eax
  call sprintf_
  add  esp, 4*5
  jmp  display_stats_hook1_End
 }
}

void AmmoModInit() {
 if (GetPrivateProfileIntA("Misc", "BonusHtHDamageFix", 1, ini)) {
  dlog("Applying Bonus HtH Damage Perk fix.", DL_INIT);
  MakeCall(0x46B126, &item_w_damage_hook, false);
  HookCall(0x46B18E, &item_w_damage_hook1);
  HookCall(0x465C71, &display_stats_hook);
  MakeCall(0x465E71, &display_stats_hook1, true);
  dlogr(" Done", DL_INIT);
 }
}
