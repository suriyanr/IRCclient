/********************************************************************************
*                                                                               *
*  A p p l i c a t i o n   O b j e c t   W i t h   X E M B E D   S u p p o r t  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXEmbedApp.h,v 1.1 2005/09/25 02:01:32 ArtDvdHike Exp $                 *
********************************************************************************/
#ifndef FXEMBEDAPP_H
#define FXEMBEDAPP_H

class FXEmbedApp: public FXApp {
FXDECLARE(FXEmbedApp)
friend class FXEmbedTopWindow;
friend class FXEmbedderWindow;
friend class FXEmbeddedWindow;
protected:
#ifndef WIN32
	int 			embedInfoAtom, embedAtom, wmProtocolsAtom, wmTakeFocusAtom, timestampAtom;
	FXRawEvent*		rawEvent;
#endif

public:
 	FXEmbedApp(const FXString& name="Application",const FXString& vendor="FoxDefault"): FXApp()	{}
	virtual ~FXEmbedApp() {}
	
	virtual void create();

protected:
  	/// Dispatch raw event
  	virtual FXbool dispatchEvent(FXRawEvent& ev);

#ifdef WIN32
	virtual long dispatchEvent(FXID hwnd,unsigned int iMsg,unsigned int wParam,long lParam);
#else
public:
	void addXInput(FXID id, FXuint mask);
	void removeXInput(FXID id, FXuint mask);
	void sendWindowClientEvent(FXID window, int time, int message_type, int message, int detail, int data1, int data2);

	FXuint getServerTime(FXID window);
#endif
private:
  	FXEmbedApp(const FXEmbedApp&): FXApp() {}
  	FXEmbedApp &operator=(const FXEmbedApp&) {return *this;}
};
#endif
