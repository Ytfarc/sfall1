/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "Criticals.h"
#include "Logging.h"
//#include <stdio.h>

struct CritStruct {
 union {
  struct {
   DWORD DamageMultiplier;
   DWORD EffectFlags;
   DWORD StatCheck;
   DWORD StatMod;
   DWORD FailureEffect;
   DWORD Message;
   DWORD FailMessage;
  };
  DWORD values[7];
 };
};

#define SetEntry(a,b,c,d,e) defaultTable[a*9*6 + b*6 + c].values[d]=e;

void CritInit() {
 dlog("Initilizing critical table bug fixes.", DL_INIT);
 CritStruct* defaultTable=(CritStruct*)0x4FEDA8;

// Crit_children.Leg_Right.Effect2
 SetEntry(2,4,1,4,0); // FailureEffect, (DAM_CRIP_ARM_RIGHT||DAM_BLIND||DAM_ON_FIRE||DAM_EXPLODE)->0
 SetEntry(2,4,1,5,5216); // Message, 5000->5216
 SetEntry(2,4,1,6,5000); // FailMessage, 0->5000

// Crit_children.Leg_Right.Effect3
 SetEntry(2,4,2,4,0); // FailureEffect, (DAM_CRIP_ARM_RIGHT||DAM_BLIND||DAM_ON_FIRE||DAM_EXPLODE)->0
 SetEntry(2,4,2,5,5216); // Message, 5000->5216
 SetEntry(2,4,2,6,5000); // FailMessage, 0->5000
 
// Crit_children.Leg_Left.Effect2
 SetEntry(2,5,1,4,0); // FailureEffect, (DAM_CRIP_ARM_RIGHT||DAM_BLIND||DAM_ON_FIRE||DAM_EXPLODE)->0
 SetEntry(2,5,1,5,5216); // Message, 5000->5216
 SetEntry(2,5,1,6,5000); // FailMessage, 0->5000

// Crit_children.Leg_Left.Effect3
 SetEntry(2,5,2,4,0); // FailureEffect, (DAM_CRIP_ARM_RIGHT||DAM_BLIND||DAM_ON_FIRE||DAM_EXPLODE)->0
 SetEntry(2,5,2,5,5216); // Message, 5000->5216
 SetEntry(2,5,2,6,5000); // FailMessage, 0->5000

// Crit_super_mutant.Leg_Left.Effect2.FailMessage
 SetEntry(3,5,1,6,5306); // 5312->5306

// Crit_ghoul.Head.Effect5.StatCheck
 SetEntry(4,0,4,2,-1); // STAT_st->NoSTAT

// Crit_brahmin.Head.Effect5.StatCheck
 SetEntry(5,0,4,2,-1); // STAT_st->NoSTAT

// Crit_radscorpion.Leg_Right.Effect2.FailureEffect
 SetEntry(6,4,1,4,2); // 0->DAM_KNOCKED_DOWN

// Crit_radscorpion.Leg_Left.Effect2.FailureEffect
 SetEntry(6,5,1,4,2); // 0->DAM_KNOCKED_DOWN

// Crit_radscorpion.Leg_Left.Effect3.FailMessage
 SetEntry(6,5,2,6,5608); // 5008->5608

// Crit_centaur.Torso.Effect4.FailureEffect
 SetEntry(9,3,3,4,2); // 0->DAM_KNOCKED_DOWN

// Crit_deathclaw.Leg_Left
 SetEntry(13,5,1,4,4); // Effect2.FailureEffect, DAM_CRIP_LEG_RIGHT->DAM_CRIP_LEG_LEFT
 SetEntry(13,5,2,4,4); // Effect3.FailureEffect, DAM_CRIP_LEG_RIGHT->DAM_CRIP_LEG_LEFT
 SetEntry(13,5,3,4,4); // Effect4.FailureEffect, DAM_CRIP_LEG_RIGHT->DAM_CRIP_LEG_LEFT
 SetEntry(13,5,4,4,4); // Effect5.FailureEffect, DAM_CRIP_LEG_RIGHT->DAM_CRIP_LEG_LEFT
 SetEntry(13,5,5,4,4); // Effect6.FailureEffect, DAM_CRIP_LEG_RIGHT->DAM_CRIP_LEG_LEFT
 dlogr(" Done", DL_INIT);
}
