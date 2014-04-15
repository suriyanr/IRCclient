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
*    W i n d o w   W i d g e t   H o s t i n g   X E M B E D   C l i e n t s    *
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
* $Id: FXEmbedderWindow.cpp,v 1.1 2005/09/25 02:01:32 ArtDvdHike Exp $         *
********************************************************************************/
#include "stdafx.h"
#include "FXEmbedderWindow.h"
#include "FXEmbedTopWindow.h"
#include "FXEmbedApp.h"

#ifndef WIN32
#define WINDOW(id) ((Window)(id))
#define DISPLAY() ((Display*)getApp()->getDisplay())

#define EMBED_ATOM ((Atom)((FXEmbedApp*)getApp())->embedAtom)
#define EMBED_INFO_ATOM ((Atom)((FXEmbedApp*)getApp())->embedInfoAtom)

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

// Map
FXDEFMAP(FXEmbedderWindow) FXEmbedSocketMap[] = {
	FXMAPFUNC(SEL_CONFIGURE, 0, FXEmbedderWindow::onConfigure),
  	FXMAPFUNC(SEL_KEYPRESS, 0, FXEmbedderWindow::onKeyPress),
  	FXMAPFUNC(SEL_KEYRELEASE, 0, FXEmbedderWindow::onKeyRelease),
  	FXMAPFUNC(SEL_FOCUSIN, 0, FXEmbedderWindow::onFocusIn),
  	FXMAPFUNC(SEL_FOCUSOUT, 0, FXEmbedderWindow::onFocusOut),
  	FXMAPFUNC(SEL_FOCUS_NEXT, 0, FXEmbedderWindow::onFocusNext),
  	FXMAPFUNC(SEL_FOCUS_PREV, 0, FXEmbedderWindow::onFocusPrev)
};

// Object implementation
FXIMPLEMENT(FXEmbedderWindow, FXComposite, FXEmbedSocketMap, ARRAYNUMBER(FXEmbedSocketMap))

FXEmbedderWindow::FXEmbedderWindow(FXComposite* p, FXObject* tgt, FXuint opts, FXint x, FXint y, FXint w, FXint h): 
	FXComposite(p, opts, x, y, w, h) {
	setTarget(tgt);
#ifndef WIN32
	embeddedWindow = NULL;
#else
	savedFocusWindow = NULL;
#endif
	flags|=FLAG_SHOWN;
}

FXEmbedderWindow::~FXEmbedderWindow() {
	destroy();
}

void FXEmbedderWindow::create() {
#ifdef WIN32
	FXComposite::create();
#else
	FXbool idCreated = xid != NULL;
	FXComposite::create();
	if(!idCreated)
		((FXEmbedApp*)getApp())->addXInput(xid, StructureNotifyMask|SubstructureNotifyMask|PropertyChangeMask);
#endif
}

void FXEmbedderWindow::detach() {
	detachEmbeddedWindow();
	FXComposite::detach();
}

void FXEmbedderWindow::destroy() {
	detachEmbeddedWindow();
	FXComposite::destroy();
}

void FXEmbedderWindow::enable() {
#ifdef WIN32
	FXComposite::enable();
#else
	FXbool enabled = (flags&FLAG_ENABLED) != 0;
	FXComposite::enable();
	if(!enabled)
		((FXEmbedApp*)getApp())->addXInput(xid, StructureNotifyMask|SubstructureNotifyMask|PropertyChangeMask);
#endif	
}

void FXEmbedderWindow::disable() {
#ifdef WIN32
	FXComposite::disable();
#else
	FXbool disabled = (flags&FLAG_ENABLED) == 0;
	FXComposite::disable();
	if(!disabled)
		((FXEmbedApp*)getApp())->addXInput(xid, StructureNotifyMask|SubstructureNotifyMask|PropertyChangeMask);
#endif	
}

void FXEmbedderWindow::focusNext() {
	if(findEmbeddedWindow() != NULL) {
#ifdef WIN32
		::SetFocus((HWND)getShell()->id());
#endif

		FXEvent event;

		event.type = SEL_KEYPRESS;
		event.state= 0;
		event.code = KEY_Tab;
		event.text = "";

		getShell()->handle(this, FXSEL(event.type, 0), (void*)&event);
	}
}

void FXEmbedderWindow::focusPrev() {
	if(findEmbeddedWindow() != NULL) {
#ifdef WIN32
		::SetFocus((HWND)getShell()->id());
#endif

		FXEvent event;

		event.type = SEL_KEYPRESS;
		event.state= 0; // TODO
		event.code = KEY_Tab;
		event.text = "";

		getShell()->handle(this, SEL_KEYPRESS, (void*)&event);
	}
}

FXID FXEmbedderWindow::getEmbeddedWindow() const {
	return findEmbeddedWindow();
}

FXID FXEmbedderWindow::findEmbeddedWindow() const {
#ifdef WIN32
	for(HWND hwnd = GetWindow((HWND)xid, GW_CHILD); hwnd != NULL; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
		if(isForeignWindow(hwnd))
			return (FXID)hwnd;

	return NULL;
#else
	return embeddedWindow;
#endif
}

FXbool FXEmbedderWindow::isForeignWindow(FXID window) const {
	// The handle is a non-Fox window if there is no FXWindow class that wraps it
	return getApp()->findWindowWithId(window) == NULL;
}

FXbool FXEmbedderWindow::preprocessEvent(FXRawEvent& ev) {
#ifndef WIN32
	switch(ev.xany.type) {
		case ReparentNotify:
			if(embeddedWindow == NULL && ev.xreparent.parent == WINDOW(xid)) 
				registerEmbeddedWindow((FXID)ev.xreparent.window);
			else if(embeddedWindow != NULL && ev.xreparent.window == WINDOW(embeddedWindow))
				unregisterEmbeddedWindow();
			break;
		case CreateNotify:
			if(embeddedWindow == NULL && ev.xcreatewindow.parent == WINDOW(xid)) 
				registerEmbeddedWindow((FXID)ev.xcreatewindow.window);
			break;
		case DestroyNotify:
			if(ev.xdestroywindow.window == WINDOW(embeddedWindow)) 
				unregisterEmbeddedWindow();
			break;
		case PropertyNotify:
			if(ev.xproperty.window == WINDOW(embeddedWindow) && ev.xproperty.atom == EMBED_INFO_ATOM)
				updateEmbeddedWindowMap();
			break;
		case ClientMessage:
			if(ev.xclient.message_type == EMBED_ATOM) {
				int type = ev.xclient.data.l[1];
				switch(type) {
					case XEMBED_REQUEST_FOCUS: setFocus(); break;
					case XEMBED_FOCUS_PREV: handle(this, SEL_FOCUS_PREV, NULL); break;
					case XEMBED_FOCUS_NEXT: handle(this, SEL_FOCUS_NEXT, NULL); break;
				}
			break;
		}
	}
#endif
	
	return FALSE; // Continue normal dispatch
}

void FXEmbedderWindow::attachEmbeddedWindow(FXID window) {
	if(findEmbeddedWindow() != NULL)
		// Already attached
		return;

#ifndef WIN32		
	XMapWindow(DISPLAY(), WINDOW(window));
	XReparentWindow(DISPLAY(), WINDOW(window), WINDOW(xid), 0, 0);
	XSync(DISPLAY(), FALSE);
#else
	SetParent((HWND)window, (HWND)xid);
#endif
}

void FXEmbedderWindow::detachEmbeddedWindow() {
	FXID embeddedWindow = findEmbeddedWindow();

	if(embeddedWindow == NULL)
		// Already detached
		return;

#ifndef WIN32		
	XUnmapWindow(DISPLAY(), WINDOW(embeddedWindow));
	XReparentWindow(DISPLAY(), WINDOW(embeddedWindow), XDefaultRootWindow(DISPLAY()), 0, 0);
	XSync(DISPLAY(), FALSE);
	
	unregisterEmbeddedWindow();
#else
	SetParent((HWND)embeddedWindow, NULL);
#endif
}

#ifndef WIN32
void FXEmbedderWindow::registerEmbeddedWindow(FXID window) {
	if(!isForeignWindow(window))
		// User is attaching/adding to us a regular FXWindow child
		return;

	if(embeddedWindow != NULL)
		// Already registered
		return;
		
	embeddedWindow = window;

	// Inform the window that it is about to be embedded
	sendEmbeddedWindowClientEvent(0, XEMBED_EMBEDDED_NOTIFY, 0, 0, 0);

	// We need to listen to property change events on the embedded window
	XSelectInput(DISPLAY(), WINDOW(embeddedWindow), PropertyChangeMask);
	XSync(DISPLAY(), FALSE);

	updateEmbeddedWindowMap();

	// Resize
	XMoveResizeWindow(DISPLAY(), WINDOW(embeddedWindow), 0, 0, getWidth(), getHeight());

	// Activate and focus, if needed
	FXWindow* shell = getShell();
	if(shell->hasFocus()) {
		shell->raise();
		sendEmbeddedWindowClientEvent(0, XEMBED_WINDOW_ACTIVATE, 0, 0, 0);

		if(hasFocus())
			sendEmbeddedWindowClientEvent(0, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT, 0, 0);
	}
}

void FXEmbedderWindow::unregisterEmbeddedWindow() {
	embeddedWindow = NULL;
}

FXbool FXEmbedderWindow::forwardEmbeddedWindowKeyEvent() {
	if(embeddedWindow == NULL) 
		// No embedded window to forward key events to
		return FALSE;

	FXRawEvent* event = ((FXEmbedApp*)getApp())->rawEvent;
		
    event->xkey.window = WINDOW(embeddedWindow);
	XSendEvent(DISPLAY(), WINDOW(embeddedWindow), FALSE, NoEventMask, (XEvent*)event);
	XSync(DISPLAY(), FALSE);

	return TRUE;
}

void FXEmbedderWindow::sendEmbeddedWindowClientEvent(int time, int message, int detail, int data1, int data2) {
	if(embeddedWindow == NULL) 
		// No embedded window to send events to
		return;

	FXEmbedApp* app = (FXEmbedApp*)getApp();
	
	if(time == 0)
		time = app->getServerTime(id());
	
	app->sendWindowClientEvent(embeddedWindow, time, EMBED_ATOM, message, detail, data1, data2);
}

void FXEmbedderWindow::updateEmbeddedWindowMap() {
	if(embeddedWindow == NULL) 
		// No embedded window to update
		return;

	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	unsigned char* data = NULL;
	if(XGetWindowProperty(DISPLAY(), WINDOW(embeddedWindow), EMBED_INFO_ATOM, 0, 2, FALSE, EMBED_INFO_ATOM, &type, &format, &nitems, &bytes_after, &data) == 0)
		if(type == EMBED_INFO_ATOM && nitems >= 2) {
			int flags = ((int*)data)[1];
			if((flags&XEMBED_MAPPED) != 0)
				XMapWindow(DISPLAY(), WINDOW(embeddedWindow));
			else
				XUnmapWindow(DISPLAY(), WINDOW(embeddedWindow));
		}

	if(data != NULL) 
		XFree((void*)data);
}
#endif

long FXEmbedderWindow::onConfigure(FXObject* sender, FXSelector sel, void* ptr) {
	long result = FXComposite::onConfigure(sender, sel, ptr);

	FXID embeddedWindow = findEmbeddedWindow();
	if(embeddedWindow != NULL) {
#ifdef WIN32
		MoveWindow((HWND)embeddedWindow, 0, 0, getWidth(), getHeight(), TRUE/*bRepaint*/);
#else
		XMoveResizeWindow(DISPLAY(), WINDOW(embeddedWindow), 0, 0, getWidth(), getHeight());
#endif
	}

	return result;
}

long FXEmbedderWindow::onKeyPress(FXObject* sender, FXSelector sel, void* ptr) {
	FXID embeddedWindow = findEmbeddedWindow();
	if(embeddedWindow != NULL) {
#ifndef WIN32
		return forwardEmbeddedWindowKeyEvent()? 1: 0;
#else
		return 0;
#endif
	}
	
	return FXComposite::onKeyPress(sender, sel, ptr);
}

long FXEmbedderWindow::onKeyRelease(FXObject* sender, FXSelector sel, void* ptr) {
	FXID embeddedWindow = findEmbeddedWindow();
	if(embeddedWindow != NULL) {
#ifndef WIN32
		return forwardEmbeddedWindowKeyEvent()? 1: 0;
#else
		return 0;
#endif
	}
	
	return FXComposite::onKeyRelease(sender, sel, ptr);
}

long FXEmbedderWindow::onFocusIn(FXObject* sender, FXSelector sel, void* ptr) {
	long result = FXComposite::onFocusIn(sender, sel, ptr);

	FXID embeddedWindow = findEmbeddedWindow();
	if(embeddedWindow != NULL) {
#ifndef WIN32
		sendEmbeddedWindowClientEvent(0, XEMBED_WINDOW_ACTIVATE, 0, 0, 0);	
		sendEmbeddedWindowClientEvent(0, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT, 0, 0);
#else
		if(savedFocusWindow != NULL && IsWindow((HWND)savedFocusWindow))
			SetFocus((HWND)savedFocusWindow);
		else
			SetFocus((HWND)embeddedWindow);
#endif
	}

	return result;
}

long FXEmbedderWindow::onFocusOut(FXObject* sender, FXSelector sel, void* ptr) {
	long result = FXComposite::onFocusOut(sender, sel, ptr);

	FXID embeddedWindow = findEmbeddedWindow();
	if(embeddedWindow != NULL) {
#ifndef WIN32
		sendEmbeddedWindowClientEvent(0, XEMBED_FOCUS_OUT, 0, 0, 0);
		sendEmbeddedWindowClientEvent(0, XEMBED_WINDOW_DEACTIVATE, 0, 0, 0);
#else
		savedFocusWindow = GetFocus();
#endif
	}

	return result;
}

long FXEmbedderWindow::onFocusNext(FXObject* sender, FXSelector sel, void* ptr) {
#ifndef WIN32
	if(embeddedWindow != NULL) {
		sendEmbeddedWindowClientEvent(0, XEMBED_FOCUS_IN, XEMBED_FOCUS_FIRST, 0, 0);
		return 1;
	}
#endif

	return FXComposite::onFocusNext(sender, sel, ptr);
}

long FXEmbedderWindow::onFocusPrev(FXObject* sender, FXSelector sel, void* ptr) {
#ifndef WIN32
	if(embeddedWindow != NULL) {
		sendEmbeddedWindowClientEvent(0, XEMBED_FOCUS_IN, XEMBED_FOCUS_LAST, 0, 0);
		return 1;
	}
#endif

	return FXComposite::onFocusPrev(sender, sel, ptr);
}
