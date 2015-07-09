/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include <fstream>
#include "Console.h"

using namespace std;

static ofstream consolefile;

static void _stdcall ConsoleFilePrint(const char* msg) {
 consolefile << msg << endl;
}

static const DWORD display_print_hook_End = 0x42C0B1;
static void __declspec(naked) display_print_hook() {
 __asm {
  pushad
  push eax
  call ConsoleFilePrint
  popad
  push ebx
  push ecx
  push edx
  push esi
  push edi
  jmp  display_print_hook_End
 }
}

void ConsoleInit() {
 char path[MAX_PATH];
 GetPrivateProfileString("Misc", "ConsoleOutputPath", "", path, MAX_PATH, ini);
 if(strlen(path)>0) {
  consolefile.open(path);
  if(consolefile.is_open()) MakeCall(0x42C0AC, &display_print_hook, true);
 }
}

void ConsoleExit() {
 if(consolefile.is_open()) consolefile.close();
}
