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

#ifndef CLASS_IRCLINEINTERPRETGLOBAL
#define CLASS_IRCLINEINTERPRETGLOBAL
#include "Compatibility.hpp"


IRCCommand IRCCommandTable[] = {
   { IC_ERROR,          "ERROR"          },
   { IC_SERVER_NOTICE,  "SERVERNOTICE"   },
   { IC_CONNECT,        "001"            },
   { IC_AWAY,           "301"            },
   { IC_USERHOST,       "302"            },
   { IC_DCCALLOWREPLY,  "617"            },
   { IC_NOTICE,         "NOTICE"         },
   { IC_PRIVMSG,        "PRIVMSG"        },
   { IC_LIST,           "LIST"           },
   { IC_FIND,           "FIND"           },
   { IC_FINDSWARM,      "FINDSWARM"      },
   { IC_PING,           "PING"           },
   { IC_PONG,           "PONG"           },
   { IC_MODE,           "MODE"           },
   { IC_NICKCHANGE,     "NICK"           },
   { IC_CLIENTINFO,     "CLIENTINFO"     },
   { IC_CTCPCLIENTINFOREPLY,"NOTICECLIENTINFO" },
   { IC_CTCPPROPAGATE,  "PROPAGATION"   },
   { IC_CTCPUPGRADE,    "UPGRADE"        },
   { IC_CTCPUPGRADEREPLY, "NOTICEUPGRADE" },
   { IC_CTCPFILESHA1,   "FILESHA1"       },
   { IC_CTCPFILESHA1REPLY, "NOTICEFILESHA1"},
   { IC_VERSION,        "VERSION"        },
   { IC_CTCPPING,       "CTCPPING"       },
   { IC_CTCPPINGREPLY,  "NOTICEPING"     },
   { IC_CTCPTIME,       "TIME"           },
   { IC_CTCPTIMEREPLY,  "NOTICETIME"     },
   { IC_CTCPVERSIONREPLY, "NOTICEVERSION"},
   { IC_CTCPFSERV,      "CTCPFSERV"      },
   { IC_CTCPPORTCHECK,  "PORTCHECK"      },
   { IC_CTCPNORESEND,   "NORESEND"       },
   { IC_CTCPLAG,        "LAG"            },
   { IC_ACTION,         "ACTION"         },
   { IC_DCC_CHAT,       "CHAT"           },
   { IC_DCC_SEND,       "SEND"           },
   { IC_DCC_SWARM,      "SWARM"          },
   { IC_DCC_RESUME,     "RESUME"         },
   { IC_DCC_ACCEPT,     "ACCEPT"         },
   { IC_JOIN,           "JOIN"           },
   { IC_PART,           "PART"           },
   { IC_QUIT,           "QUIT"           },
   { IC_KICK,           "KICK"           },
   { IC_LIST_START,     "321"            },
   { IC_LIST_ENTRY,     "322"            },
   { IC_LIST_END,       "323"            },
   { IC_TOPIC,          "332"            },
   { IC_TOPIC_END,      "333"            },
   { IC_TOPIC_CHANGE,   "TOPIC"          },
   { IC_NICKLIST,       "353"            },
   { IC_NAMES_END,      "366"            },
   { IC_NICKINUSE,      "433"            },
   { IC_BANLIST,        "367"            },
   { IC_BANLIST_END,    "368"            },
   { IC_NUMERIC,        "NUMERIC"        },
//
// Below is non IRC Commands.

   { IC_UNKNOWN,        NULL             }
};


#endif
