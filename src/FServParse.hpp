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

#ifndef CLASS_FSERVPARSE
#define CLASS_FSERVPARSE

#include "Compatibility.hpp"
#include "LineParse.hpp"
#include "TriggerType.hpp"

// This Class is for extracting Trigger Information out of a line.
// Line is in format: nick Trigger_Text

class FServParse {
public:
   FServParse();
   ~FServParse();
   FServParse& operator = (char *); // Equating it to a character string
   TriggerTypeE getTriggerType();
   const char *getTriggerName();
   const char *getTriggerNick();
   const char *getTriggerDottedIP();
   unsigned long getTriggerLongIP();
   const char *getFileName();
   const char *getPropagatedNick();
   size_t getFileSize();
   long getPackNumber();
   char getClientType();
   int getCurrentSends();
   int getTotalSends();
   int getCurrentQueues();
   int getTotalQueues();
   bool isIrofferFirewalled();
   const char *removeColors(char *line);
   void printDebug();

private:
   void parse(char *line);
   void init();
   void freeAll();

   TriggerTypeE Type; // If INVALID => not parsed or not a valid trigger type.
   char *TriggerName; // Used if Type = FSERVMSG or FSERVCTCP or PROPAGATIONCTCP
   char *TriggerNick;
   char *TriggerDottedIP;
   unsigned long TriggerLongIP;
   char *PropagatedNick; // Used if Type = PROPAGATIONCTCP
   char *FileName; // Used if Type = XDCC
   size_t FileSize; // Used if Type = XDCC
   long PackNumber; // Used if Type = XDCC

   bool IrofferFirewalled; // Used if Type = IROFFER_FIREWALL_LINE

   int CurrentSends; // Used if Type = SENDS_QS_LINE or FSERVCTCP 
                     // or PROPAGATIONCTCP
   int TotalSends;
   int CurrentQueues;
   int TotalQueues;
   char ClientType; // SysReset/Iroffer/MasalaMate: S/I/M

   char *Stripped; // Returned by removeColors

   void populateIP(); // To fill up TriggerLongIP, TriggerDottedIP
   char *savedHostName; // populateIP works on this.
};

#endif
