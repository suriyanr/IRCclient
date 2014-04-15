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

#include <stdio.h>

unsigned long CRCTable[256];
unsigned long CalculateBufferCRC(unsigned int count, unsigned long crc, void *buffer );

unsigned long CalculateFileCRC( file )
FILE *file;
{
    unsigned long crc;
    int count;
    unsigned char buffer[ 512 ];
    int i;

    crc = 0xFFFFFFFFL;
    i = 0;
    for ( ; ; ) {
        count = fread( buffer, 1, 512, file );
#if 0
        if ( ( i++ % 32 ) == 0 )
            putc( '.', stdout );
#endif
        if ( count == 0 )
            break;
        crc = CalculateBufferCRC( count, crc, buffer );
    }
#if 0
    putc( ' ', stdout );
#endif
    return( crc ^= 0xFFFFFFFFL );
}

#define CRC32_POLYNOMIAL     0xEDB88320L

void BuildCRCTable()
{
    int i;
    int j;
    unsigned long crc;

    for ( i = 0; i <= 255 ; i++ ) {
        crc = i;
        for ( j = 8 ; j > 0; j-- ) {
            if ( crc & 1 )
                crc = ( crc >> 1 ) ^ CRC32_POLYNOMIAL;
            else
                crc >>= 1;
        }
        CRCTable[ i ] = crc;
    }
}

unsigned long CalculateBufferCRC( count, crc, buffer )
unsigned int count;
unsigned long crc;
void *buffer;
{
    unsigned char *p;
    unsigned long temp1;
    unsigned long temp2;

    p = (unsigned char*) buffer;
    while ( count-- != 0 ) {
        temp1 = ( crc >> 8 ) & 0x00FFFFFFL;
        temp2 = CRCTable[ ( (int) crc ^ *p++ ) & 0xff ];
        crc = temp1 ^ temp2;
    }
    return( crc );
}

int main(int argc, char *argv[]) {
FILE *fp;
unsigned long crc = 0;

   BuildCRCTable();
   fp = fopen(argv[1], "r");
   if (fp != NULL) {
      crc = CalculateFileCRC(fp);
   }
   printf("CRC is: %lx\n", crc);
}
