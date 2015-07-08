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
#include "Logging.h"

#ifdef TRACE

#include "main.h"
#include <fstream>

using namespace std;

static int DebugTypes=0;
static ofstream Log;

void dlog(const char* a, int type) {
 if(type!=DL_MAIN&&!(type&DebugTypes)) return;
 Log << a;
 Log.flush();
}
void dlogr(const char* a, int type) {
 if(type!=DL_MAIN&&!(type&DebugTypes)) return;
 Log << a << "\r\n";
 Log.flush();
}

void LoggingInit() {
 Log.open("sfall-log.txt", ios_base::out | ios_base::trunc);
 if (GetPrivateProfileIntA("Debugging", "Init", 0, ".\\ddraw.ini")) DebugTypes|=DL_INIT;
}

#endif

void dlog_f(const char *format, int type, ...) {

#ifdef TRACE

 int i;
 va_list arg;
 char buf[1024];
 va_start(arg, format);
 i = vsprintf(buf, format, arg);
 va_end(arg);
 Log << buf;
 Log.flush();

#endif

}
