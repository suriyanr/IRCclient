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

#include "Base64.hpp"
#include "StackTrace.hpp"

const char *base64Table =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

Base64::Base64() {
   TRACE();
   buffer = NULL;
}

Base64::~Base64() {
   TRACE();
   delete buffer;
}

void Base64::in(const char *str) {
unsigned char *src;
unsigned char *dst;
int bits, data, srcLen, dstLen;

   TRACE();
   delete buffer;
   buffer = NULL;

   // make base64 string
   srcLen = strlen(str);
   dstLen = (srcLen + 2) / 3 * 4;
   buffer = new char[dstLen + 1];
   bits = data = 0;
   src = (unsigned char *) str;
   dst = (unsigned char *) buffer;
   while (dstLen--) {
      if (bits < 6) {
         data = (data << 8) | *src;
         bits += 8;
         if (*src != 0) {
                src++;
         }
      }
      *dst++ = base64Table[0x3F & (data >> (bits - 6))];
      bits -= 6;
   }
   *dst = '\0';

   // fix-up tail padding
   switch (srcLen % 3) {
      case 1:
         *--dst = '=';
      case 2:
         *--dst = '=';
   }
}

const char *Base64::out() {
   TRACE();
   return(buffer);
}
