#pragma once

DWORD _stdcall FakeGetTickCount();
void _stdcall FakeGetLocalTime(LPSYSTEMTIME);
void TimerInit();
