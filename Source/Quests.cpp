/*
 *    sfall
 *    Copyright (C) 2015  The sfall team
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

#include <stdio.h>
#include "Define.h"

void QuestsInit() {
 char buf[MAX_PATH-3];
 GetPrivateProfileString("Misc", "QuestsFile", "", buf, MAX_PATH, ini);
 if (strlen(buf) > 0) {
  char iniQuests[MAX_PATH], section[4], thread[8];
  sprintf(iniQuests, ".\\%s", buf);
  short* questsTable = (short*)_sthreads;
  for (int city = 0; city < 12; city++) {
   _itoa_s(city * 10 + 700, section, 10);
   for (int quest = 0; quest < 9; quest++) {
    sprintf(thread, "Quest%d", city * 10 + 701 + quest);
    short gvar_index = GetPrivateProfileIntA(section, thread, -1, iniQuests);
    if (gvar_index != -1) questsTable[city * 9 + quest] = gvar_index;
   }
  }
 }
}
