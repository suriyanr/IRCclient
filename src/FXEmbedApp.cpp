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
*  A p p l i c a t i o n   O b j e c t   W i t h   X E M B E D   S u p p o r t  *
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
* $Id: FXEmbedApp.cpp,v 1.1 2005/09/25 02:01:32 ArtDvdHike Exp $               *
********************************************************************************/
#include "stdafx.h"
#include "FXEmbedApp.h"
#include "FXEmbedTopWindow.h"
#include "FXEmbedderWindow.h"
#include "FXEmbeddedWindow.h"
#include "FXTray.h"

#ifdef WIN32
#define TRAYMESSAGE (WM_USER+1)
#else
#define WINDOW(id) ((Window)(id))
#define DISPLAY() ((Display*)getDisplay())
#endif

FXIMPLEMENT(FXEmbedApp, FXApp, NULL, 0)

void FXEmbedApp::create() {
	FXApp::create();

#ifndef WIN32
	rawEvent = NULL;
	
	embedAtom = XInternAtom(DISPLAY(), "_XEMBED", FALSE);
	embedInfoAtom = XInternAtom(DISPLAY(), "_XEMBED_INFO", FALSE);
    wmProtocolsAtom = XInternAtom(DISPLAY(), "WM_PROTOCOLS", FALSE);
    wmTakeFocusAtom = XInternAtom(DISPLAY(), "WM_TAKE_FOCUS", FALSE);
	timestampAtom = XInternAtom(DISPLAY(), "FOX_TIMESTAMP_PROP", FALSE);
#endif
}

#ifdef WIN32
long FXEmbedApp::dispatchEvent(FXID hwnd, unsigned int iMsg, unsigned int wParam, long lParam) {
	FXbool processed = FALSE;
	long result;

	switch(iMsg) {
		case WM_ACTIVATE: {
			if(wParam != WA_INACTIVE) {
				// Activate
				// Dispatch this event to the original FXApp message proc and then fool it
				// that we have got a WM_SETFOCUS event, in order the focusWindow member to be correctly updated
				result = FXApp::dispatchEvent(hwnd, iMsg, wParam, lParam);
				FXApp::dispatchEvent(hwnd, WM_SETFOCUS, lParam, 0);
			} else {
				// Deactivate
				// Dispatch this event to the original FXApp message proc and then fool it
				// that we have got a WM_KILLFOCUS event, in ordser the focusWindow member to be correctly updated
				result = FXApp::dispatchEvent(hwnd, iMsg, wParam, lParam);
				FXApp::dispatchEvent(hwnd, WM_KILLFOCUS, lParam, 0);
			}

			processed = TRUE;
			break;
		}
		case WM_MOUSEACTIVATE: {
			// This message is processed in order for us to know if an embedded window is about to
			// receive the input focus.
			// If so, we set Fox's logical focus to the FXEmbedderWindow parent, so when the 
			// main window is deactivated and then activated again, FXEmbedderWindow will get FocusIn event
			// and will return the focus to the embedded window
			//
			// The way the activated window is detected is a bit hackish, but I don't think there is a way around it 
			POINT pt;
			if(GetCursorPos(&pt) == 0) {
				DWORD pos = GetMessagePos();
				pt.x = (short)(pos&0xFFFF);
				pt.y = (short)(pos >> 16);
			}
			
			HWND hitWindow = WindowFromPoint(pt);
			FXWindow* window = NULL;
			
			if(findWindowWithId(hitWindow) == NULL) {
				// OK. not a Fox window

				HWND hwnd = hitWindow;
				while(window == NULL) {
					if((hwnd = GetParent(hwnd)) == 0)
						break;

					window = findWindowWithId(hwnd);
				}
			}
				
			if(window != NULL)
				window->setFocus();
			else {
				if(getFocusWindow() != NULL) {
					HWND winFocusWindowId = ::GetFocus();
					HWND foxFocusWindowId = (HWND)getFocusWindow()->id();

					if(foxFocusWindowId != winFocusWindowId && IsChild(foxFocusWindowId, winFocusWindowId))
						// Return the focus to the top window
						::SetFocus(foxFocusWindowId);
				}
			}

			break;
		}
		case WM_SETFOCUS:
		case WM_KILLFOCUS: {
			// We *should not* delegate these to the original FXApp message proc, because 
			// we can now have focus set inside embedded window, which FXApp cannot deal with because 
			// it expects the focus to be always in the main window
			//
			// Fox should really handle WM_ACTIVATE instead of WM_SETFOCUS/WM_KILLFOCUS
			result = DefWindowProc((HWND)hwnd, iMsg, wParam, lParam);

			processed = TRUE;
			break;
		}
		case TRAYMESSAGE: {
			FXWindow* tray = findWindowWithId(hwnd);
			if(tray != NULL) {
				switch(lParam) {
					case WM_LBUTTONDOWN:
						SetForegroundWindow((HWND)tray->id());
						tray->handle(tray, FXSEL(SEL_COMMAND, FXTray::ID_TRAY_CLICKED), NULL);
						break;
					case WM_LBUTTONDBLCLK:
					case WM_RBUTTONDBLCLK:
						SetForegroundWindow((HWND)tray->id());
						tray->handle(tray, FXSEL(SEL_COMMAND, FXTray::ID_TRAY_DBLCLICKED), NULL);
						break;
					case WM_RBUTTONUP:
						SetForegroundWindow((HWND)tray->id());
						tray->handle(tray, FXSEL(SEL_COMMAND, FXTray::ID_TRAY_CONTEXT_MENU), NULL);
						break;
				}

				processed = TRUE;
			}
			break;
		}
	}

	return processed? result: FXApp::dispatchEvent(hwnd, iMsg, wParam, lParam);
}
#endif

FXbool FXEmbedApp::dispatchEvent(FXRawEvent& ev) {
	FXbool preprocessed = FALSE;

#ifndef WIN32
	// If the event is KeyPress or KeyRelease we have to keep it as we need to 
	// forward raw X11 events for FXEmbedderWindow to embedded windows
	rawEvent = &ev;
	
	switch(ev.xany.type) {
		case FocusIn: {	
			FXWindow* window = findWindowWithId(ev.xany.window);
			preprocessed = window != NULL && window->isMemberOf(FXMETACLASS(FXEmbedTopWindow));
			break;
		}
		case FocusOut: {
			FXWindow* window = findWindowWithId(ev.xany.window);
			preprocessed = window != NULL && window->isMemberOf(FXMETACLASS(FXEmbedTopWindow));
			break;
		}
      	case ClientMessage: {
        	if(ev.xclient.message_type == wmProtocolsAtom) {
        		// WM_PROTOCOLS
          		if((FXID)ev.xclient.data.l[0] == wmTakeFocusAtom) {
          			// WM_TAKE_FOCUS

            	    // Assign focus to innermost modal dialog, even when trying to focus
            		// on another window; these other windows are dead to inputs anyway.
            		// XSetInputFocus causes a spurious BadMatch error; we ignore this in xerrorhandler
            		if(getModalWindow() != NULL && getModalWindow()->id() != NULL)
            			ev.xclient.window = getModalWindow()->id();

					FXWindow* window = findWindowWithId(ev.xclient.window);
					if(window != NULL) {
						if(window->isMemberOf(FXMETACLASS(FXEmbedTopWindow)))
							// Assign to the focus proxy of this one
							window = ((FXEmbedTopWindow*)window)->focusProxy;
							
           				XSetInputFocus(DISPLAY(), window->id(), RevertToParent, ev.xclient.data.l[1] /*getServerTime(ev.xclient.window)*/);
            			preprocessed = TRUE;
            		}
            	}
          	}
          	break;
        }
	}
#else
	MSG* msg = (MSG*)&ev;
	switch(msg->message) {
		case WM_KEYDOWN: // Fall-through
		case WM_KEYUP: {
			if(findWindowWithId(msg->hwnd) == NULL)
				// We need to call TranslateMessage() on the foreign HWND handles, or otherwise they won't receive WM_CHAR events
				TranslateMessage(msg);
			break;
		}
	}
#endif

	if(!preprocessed) {
#ifdef WIN32	
		FXWindow* window = findWindowWithId(((MSG)ev).hwnd);
#else			
		FXWindow* window = findWindowWithId(ev.xany.window);
#endif
		if(window != NULL) {
			if(window->isMemberOf(FXMETACLASS(FXEmbedderWindow)))
				preprocessed = ((FXEmbedderWindow*)window)->preprocessEvent(ev);
			else if(window->isMemberOf(FXMETACLASS(FXEmbeddedWindow)))
				preprocessed = ((FXEmbeddedWindow*)window)->preprocessEvent(ev);
		}
	}
		
	return preprocessed? TRUE: FXApp::dispatchEvent(ev);
}

#ifndef WIN32
void FXEmbedApp::sendWindowClientEvent(FXID window, int time, int message_type, int message, int detail, int data1, int data2) {
	XEvent ev;
	memset(&ev, 0, sizeof(ev));
	
	ev.xclient.type = ClientMessage;
	ev.xclient.window = WINDOW(window);
	ev.xclient.message_type = message_type;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = time != 0? time: CurrentTime;
	ev.xclient.data.l[1] = message;
	ev.xclient.data.l[2] = detail;
	ev.xclient.data.l[3] = data1;
	ev.xclient.data.l[4] = data2;
	XSendEvent(DISPLAY(), WINDOW(window), FALSE, NoEventMask, &ev);
	XSync(DISPLAY(), FALSE);
}

void FXEmbedApp::addXInput(FXID id, FXuint mask) {
	XWindowAttributes wa;
	XGetWindowAttributes(DISPLAY(), WINDOW(id), &wa);
	XSelectInput(DISPLAY(), WINDOW(id), wa.your_event_mask|mask);
	XSync(DISPLAY(), FALSE);
}

void FXEmbedApp::removeXInput(FXID id, FXuint mask) {
	XWindowAttributes wa;
	XGetWindowAttributes(DISPLAY(), WINDOW(id), &wa);
	XSelectInput(DISPLAY(), WINDOW(id), wa.your_event_mask&~mask);
	XSync(DISPLAY(), FALSE);
}

static int timestamp_atom;
static int timestamp_predicate(Display* display, XEvent* xevent, XPointer arg) {
	Window window = (Window)arg;

	return xevent->type == PropertyNotify && xevent->xproperty.window == window && xevent->xproperty.atom == timestamp_atom;
}

FXuint FXEmbedApp::getServerTime(FXID window) {
	timestamp_atom = timestampAtom;
	
	FXuchar c = 'a';
	XChangeProperty(DISPLAY(), (Window)window, timestampAtom, timestampAtom, 8, PropModeReplace, &c, 1);

	XEvent xevent;
	XIfEvent(DISPLAY(), &xevent, timestamp_predicate, (char*)window);

	timestamp_atom = NULL;

	return xevent.xproperty.time;
}
#endif
