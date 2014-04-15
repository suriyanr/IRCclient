/*
 Copyright (C) 2014 Suriyan Ramasami <suriyan.r@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#ifdef NO_STACKTRACE
#  undef NO_STACKTRACE
#endif

#include "StackTrace.hpp"
#include "LineParse.hpp"

using namespace std;

void Thr();

int main() {
char *ptr = NULL;

THR_HANDLE ThrH;
THREADID tempTID;

   TRACE_INIT_CRASH("Main");
   TRACE();
   cout << "This program will do a SEGV in a thread." << endl;
   cout << "Check if MasalaMate.trace is generated with the right trace" << endl;

#ifdef __MINGW32__
   ThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) Thr, NULL, 0, &tempTID);
#else
   pthread_create(&ThrH, NULL, (void * (*)(void *)) Thr, NULL);
#endif

   TRACE();
   sleep(10);
//   *ptr = 'c';
}

void a();
void b();
void c();
void d();
void e();

void Thr() {

  TRACE_INIT_CRASH("Thr");
  //TRACE_INIT_NOCRASH();

  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  TRACE();
  {
     // Test is constructor and destructor appear in stack
     LineParse LineP;
  }
  TRACE();
  TRACE();

  a();
}
 
void a() {
  b();
}

void b() {
  c();
}

void c() {
  d();
}

void d() {
  e();
}

void e() {
char *p = NULL;
void *StackArray[25];
   // Before we crash lets print the stack.
   backtrace(StackArray, 24);

   *p = 0;
}

