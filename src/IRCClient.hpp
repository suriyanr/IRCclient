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

#ifndef CLASS_IRCCLIENT
#define CLASS_IRCCLIENT



#include "XChange.hpp"
#include "TCPConnect.hpp"
#include "IRCServerList.hpp"
#include "IRCChannelList.hpp"
#include "Compatibility.hpp"

class IRCClient {
public:
   void run(XChange *XGlobal);

private:
   THR_HANDLE ToServerThrH;
   THR_HANDLE ToServerNowThrH;
   THR_HANDLE FromServerThrH;
   THR_HANDLE toUIThrH;
   THR_HANDLE DCCThrH;
   THR_HANDLE fromUIThrH;
   THR_HANDLE ToTriggerThrH;
   THR_HANDLE ToTriggerNowThrH;
   THR_HANDLE DCCServerThrH;
   THR_HANDLE DwnldInitThrH;
   THR_HANDLE TimerThrH;
   THR_HANDLE UpnpThrH;
   THR_HANDLE SwarmThrH;

   void init(XChange *XGlobal);   
   void endit(XChange *XGlobal);   
};


#endif
