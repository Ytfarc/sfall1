#pragma once

#include <Windows.h>
#include "SafeWrite.h"

extern char ini[65];
extern char translationIni[65];

extern DWORD _combatNumTurns;
extern DWORD _tmpQNode;

void queue_find_first_();
void queue_find_next_();
