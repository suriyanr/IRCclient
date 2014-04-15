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

/********************************************************************************
*                                                                               *
*     W i n d o w   W i d g e t   E m b e d d a b l e   V i a   X E M B E D     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* Major Contributions by Ivan Markov                                            *
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
* $Id: FXEmbeddedWindow.cpp,v 1.1 2005/09/14 04:09:03 ArtDvdHike Exp $         *
********************************************************************************/
#include "stdafx.h"
#include "FXEmbedApp.h"
#include "FXEmbeddedWindow.h"

#ifndef WIN32
#define WINDOW(id) ((Window)(id))
#define DISPLAY() ((Display*)getApp()->getDisplay())

#define EMBED_ATOM (((FXEmbedApp*)getApp())->embedAtom)
#define EMBED_INFO_ATOM (((FXEmbedApp*)getApp())->embedInfoAtom)

// XEMBED message codes (see XEMBED spec)
#define XEMBED_EMBEDDED_NOTIFY			0
#define XEMBED_WINDOW_ACTIVATE  		1
#define XEMBED_WINDOW_DEACTIVATE  		2
#define XEMBED_REQUEST_FOCUS	 		3
#define XEMBED_FOCUS_IN 	 			4
#define XEMBED_FOCUS_OUT  				5
#define XEMBED_FOCUS_NEXT 				6
#define XEMBED_FOCUS_PREV 				7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON 				10
#define XEMBED_MODALITY_OFF 			11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

// XEMBED message details (see XEMBED spec)
#define XEMBED_FOCUS_CURRENT            0
#define XEMBED_FOCUS_FIRST              1
#define XEMBED_FOCUS_LAST               2

// Flags for _XEMBED_INFO (see XEMBED spec)
#define XEMBED_MAPPED                   (1 << 0)
#endif

// Object implementation
FXIMPLEMENT(FXEmbeddedWindow, FXShell, NULL, 0)

FXEmbeddedWindow::FXEmbeddedWindow(FXApp* a, FXID id, FXObject* tgt): FXShell(a, LAYOUT_EXPLICIT, 0, 0, 0, 0),
	embedderWindow(id) {
	setTarget(tgt);
}

FXEmbeddedWindow::~FXEmbeddedWindow() {
	destroy();
}

void FXEmbeddedWindow::create() {
#ifdef WIN32
	FXShell::create();
#else
	FXbool idCreated = xid != NULL;
	FXShell::create();
	if(!idCreated) {
		int data[2] = {0, 0};
        XChangeProperty(DISPLAY(), xid, EMBED_INFO_ATOM, EMBED_INFO_ATOM, 32, PropModeReplace, (FXuchar*)&data, 2);

		((FXEmbedApp*)getApp())->addXInput(xid, StructureNotifyMask|PropertyChangeMask);
		
		if(embedderWindow != NULL) {
			FXID tmpEmbedderWindow = embedderWindow;
			embedderWindow = NULL;
			
			XMapWindow(DISPLAY(), WINDOW(tmpEmbedderWindow));
			XReparentWindow(DISPLAY(), WINDOW(xid), WINDOW(tmpEmbedderWindow), 0, 0);
			XSync(DISPLAY(), FALSE);
		}
	}
#endif	
}

void FXEmbeddedWindow::detach() {
	FXShell::detach();
}

void FXEmbeddedWindow::destroy() {
	FXShell::destroy();
}

void FXEmbeddedWindow::enable() {
#ifdef WIN32
	FXShell::enable();
#else
	FXbool enabled = (flags&FLAG_ENABLED) != 0;
	FXShell::enable();
	if(!enabled)
		((FXEmbedApp*)getApp())->addXInput(xid, StructureNotifyMask|PropertyChangeMask);
#endif
}

void FXEmbeddedWindow::disable() {
#ifdef WIN32
	FXShell::disable();
#else
	FXbool disabled = (flags&FLAG_ENABLED) == 0;
	FXShell::disable();
	if(!disabled)
		((FXEmbedApp*)getApp())->addXInput(xid, StructureNotifyMask|PropertyChangeMask);
#endif
}

void FXEmbeddedWindow::show() {
#ifdef WIN32
	FXShell::show();
#else
	if((flags&FLAG_SHOWN) == 0) {
		flags |= FLAG_SHOWN;
		int data[2] = {0, XEMBED_MAPPED};
    	XChangeProperty(DISPLAY(), xid, EMBED_INFO_ATOM, EMBED_INFO_ATOM, 32, PropModeReplace, (FXuchar*)&data, 2);
    }
#endif
}

void FXEmbeddedWindow::hide() {
#ifdef WIN32
	FXShell::hide();
#else
	if((flags&FLAG_SHOWN) != 0) {
		flags &= ~FLAG_SHOWN;
		int data[2] = {0, 0};
    	XChangeProperty(DISPLAY(), xid, EMBED_INFO_ATOM, EMBED_INFO_ATOM, 32, PropModeReplace, (FXuchar*)&data, 2);
    }
#endif
}

void FXEmbeddedWindow::setFocus() {
	FXShell::setFocus();
#ifndef WIN32
	sendEmbedderWindowClientEvent(0, XEMBED_REQUEST_FOCUS, 0, 0, 0);
#endif	
}

void FXEmbeddedWindow::killFocus() {
	FXShell::killFocus();
}

void FXEmbeddedWindow::hostingStarted() {
}

void FXEmbeddedWindow::hostingEnded() {
}

void FXEmbeddedWindow::embeddingNotify() {
}

FXbool FXEmbeddedWindow::preprocessEvent(FXRawEvent& ev) {
#ifndef WIN32
	switch(ev.xany.type) {
		case ReparentNotify:
			if(embedderWindow == NULL && ev.xreparent.window == WINDOW(xid))  {
				registerEmbedderWindow((FXID)ev.xreparent.parent);
				hostingStarted();
			} else if(embedderWindow != NULL && ev.xreparent.parent == WINDOW(embedderWindow)) {
				hostingEnded();
				unregisterEmbedderWindow();
			}
			break;
		case DestroyNotify:
			if(embedderWindow != NULL) {
				hostingEnded();
				unregisterEmbedderWindow();
			}
			break;			
		case ClientMessage:
			if(ev.xclient.message_type == EMBED_ATOM) {
				int type = ev.xclient.data.l[1];
				switch(type) {
					case XEMBED_EMBEDDED_NOTIFY:
						embeddingNotify();
						break;
					case XEMBED_WINDOW_ACTIVATE:
						break;
					case XEMBED_WINDOW_DEACTIVATE:
						break;
					case XEMBED_FOCUS_IN: {
						// Emulate X11 FocusIn event for this shell
						FXRawEvent ev;
						memset(&ev, 0, sizeof(ev));
	
						ev.xany.type = FocusIn;
						ev.xfocus.display = DISPLAY();
						ev.xfocus.window = WINDOW(xid);
						
						((FXEmbedApp*)getApp())->dispatchEvent(ev);
						break;
					}
					case XEMBED_FOCUS_OUT: {
						// Emulate X11 FocusOut event for this shell
						FXRawEvent ev;
						memset(&ev, 0, sizeof(ev));
	
						ev.xany.type = FocusOut;
						ev.xfocus.display = DISPLAY();
						ev.xfocus.window = WINDOW(xid);
						
						((FXEmbedApp*)getApp())->dispatchEvent(ev);
						break;
					}
				}
			break;
		}
	}
#endif
	
	return FALSE; // Continue normal dispatch
}

#ifndef WIN32
void FXEmbeddedWindow::registerEmbedderWindow(FXID window) {
	if(embedderWindow != NULL)
		// Already registered
		return;
		
	embedderWindow = window;
	flags |= FLAG_SHOWN; // Because the XEMBED client window is mapped immediately after being reparented
}

void FXEmbeddedWindow::unregisterEmbedderWindow() {
	if(embedderWindow == NULL)
		// Already unregistered or different window
		return;
	
//	((FXEmbedApp*)getApp())->embedders.remove((void*)embeddedWindow);
	
	embedderWindow = NULL;
}

void FXEmbeddedWindow::sendEmbedderWindowClientEvent(int time, int message, int detail, int data1, int data2) {
	FXEmbedApp* app = (FXEmbedApp*)getApp();
	app->sendWindowClientEvent(embedderWindow, time, EMBED_ATOM, message, detail, data1, data2);
}
#endif
