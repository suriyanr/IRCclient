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
#include "Utilities.hpp"

#include "Compatibility.hpp"

using namespace std;

int main() {
int HoleIndex, BitIndex;
char ByteArray[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1];
char StringArray[SWARM_MAX_FUTURE_HOLES + 1];

   memset(ByteArray, 0, sizeof(ByteArray));

   for (int i = 0; i < SWARM_MAX_FUTURE_HOLES; i++) {
      setBitInByteArray(ByteArray, i);
      convertByteArrayToString(ByteArray, SWARM_MAX_FUTURE_HOLES, StringArray);
      cout << "After setting bit " << i << ": " << StringArray << endl;
   }

   for (int i = 0; i < SWARM_MAX_FUTURE_HOLES; i++) {
      bool retvalb = isSetBitInByteArray(ByteArray, i);
      cout << "isSetNitInByteArray(" << i << ") " << retvalb << endl;
   }

   for (int i = 0; i < SWARM_MAX_FUTURE_HOLES; i++) {
      clrBitInByteArray(ByteArray, i);
      convertByteArrayToString(ByteArray, SWARM_MAX_FUTURE_HOLES, StringArray);
      cout << "After clearing bit " << i << ": " << StringArray << endl;
   }

   for (int i = 0; i < SWARM_MAX_FUTURE_HOLES; i++) {
      bool retvalb = isSetBitInByteArray(ByteArray, i);
      convertByteArrayToString(ByteArray, SWARM_MAX_FUTURE_HOLES, StringArray);
      cout << "isSetNitInByteArray(" << i << ") " << retvalb << endl;
   }

   // HoleIndex to BitIndex formula test.
   for (HoleIndex = 0; HoleIndex < SWARM_MAX_FUTURE_HOLES; HoleIndex++) {
      BitIndex = (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
      cout << "HoleIndex: " << HoleIndex << " converts to BitIndex: " << BitIndex << endl;
      cout << "In above, HoleIndex 0 should be 2, and HoleIndex 47 should be 126" << endl;
   }

   // Lets Mark HoleIndex SWARM_MAX_FUTURE_HOLES - 1 as 10
   HoleIndex = SWARM_MAX_FUTURE_HOLES - 1;
   BitIndex = (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
   cout << "HoleIndex: " << HoleIndex << " BitIndex: " << BitIndex << " marked as 10" << endl;
   setBitInByteArray(ByteArray, BitIndex);
   clrBitInByteArray(ByteArray, BitIndex + 1);
   // Now check if its marked.
   cout << "isSetBitInByteArray(BitIndex) Bitindex: " << BitIndex << " " << isSetBitInByteArray(ByteArray, BitIndex) << endl;

}
