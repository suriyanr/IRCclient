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
*                      T r a y   I c o n   W i d g e t                          *
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
* $Id: FXTray.cpp,v 1.2 2005/09/24 03:05:08 ArtDvdHike Exp $                   *
********************************************************************************/
#include "stdafx.h"
#include "FXEmbedApp.h"
#include "FXEmbeddedWindow.h"
#include "FXTray.h"

#ifdef WIN32
#include <shellapi.h>

#define TRAYMESSAGE (WM_USER+1)
#define _NOTIFYICONDATA_V1_SIZE 88
#else
#define WINDOW(id) ((Window)(id))
#define DISPLAY() ((Display*)getApp()->getDisplay())

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1
#endif

// Map
FXDEFMAP(FXTray) FXTrayMap[] = {
	FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_CLICKED, FXTray::onCmdTrayClicked),
	FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_DBLCLICKED, FXTray::onCmdTrayDblClicked),
	FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_CONTEXT_MENU, FXTray::onCmdTrayContextMenu),
	FXMAPFUNC(SEL_PAINT, 0, FXTray::onPaint),
  	FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, FXTray::onLeftBtnPress),
  	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE, 0, FXTray::onRightBtnRelease),
	FXMAPFUNC(SEL_COMMAND, FXTray::ID_SETTIPSTRING, FXTray::onCmdSetTip),
	FXMAPFUNC(SEL_COMMAND, FXTray::ID_GETTIPSTRING, FXTray::onCmdGetTip),
	FXMAPFUNC(SEL_QUERY_TIP, 0, FXTray::onQueryTip)
};

FXIMPLEMENT(FXTray, FXEmbeddedWindow, FXTrayMap, ARRAYNUMBER(FXTrayMap))

// To get the etched & shape pixmaps
class FXIconAccess: public FXIcon {
private:
	FXIconAccess(): FXIcon() {}
	
public:
	static FXID getEtchPixmap(FXIcon* ic) {return ((FXIconAccess*)ic)->etch;}
	static FXID getShapePixmap(FXIcon* ic) {return ((FXIconAccess*)ic)->shape;}
};

FXTray::FXTray(FXApp* app, const FXString& txt, FXIcon* ic, FXObject* tgt): FXEmbeddedWindow(app, 0, tgt),
	text(txt),
	icon(ic),
#ifdef WIN32
	visible(TRUE),
	iconId(NULL)
#else
	sysTrayAtom(0),
	sysTrayOpcodeAtom(0),
	sysTrayMessageDataAtom(0)
#endif	
{
#ifndef WIN32
	char buf[256];
	int screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(DISPLAY()));
	sprintf(buf, "_NET_SYSTEM_TRAY_S%i", screen);
	
	sysTrayAtom = (int)XInternAtom(DISPLAY(), buf, FALSE);
	sysTrayOpcodeAtom = (int)XInternAtom(DISPLAY(), "_NET_SYSTEM_TRAY_OPCODE", FALSE);
	sysTrayMessageDataAtom = (int)XInternAtom(DISPLAY(), "_NET_SYSTEM_TRAY_MESSAGE_DATA", FALSE);
#endif
}

FXTray::~FXTray() {
	target = NULL;
	icon = NULL;
}

void FXTray::create() {
	FXbool idCreated = xid != NULL;
	FXEmbeddedWindow::create();
	
	if(idCreated)
		return;
		
#ifdef WIN32
	NOTIFYICONDATA iconData;
	iconData.cbSize = sizeof(NOTIFYICONDATA);
	iconData.uID = (int)(void*)this;
	iconData.hWnd = (HWND)xid;
	iconData.uFlags = NIF_MESSAGE;
	iconData.uCallbackMessage = TRAYMESSAGE;

	Shell_NotifyIcon(NIM_ADD, &iconData);
#else
	updateTrayManager();
#endif

	setTipText(text);

	if(icon != NULL) {
		icon->create();
		setIcon(icon);
	}
}

void FXTray::destroy() {
#ifdef WIN32
	if(xid != NULL) {
		if(iconId != NULL) {
			DeleteObject(iconId);
			iconId = NULL;
		}
	
		NOTIFYICONDATA iconData;
		iconData.cbSize = _NOTIFYICONDATA_V1_SIZE;
		iconData.uID = (int)(void*)this;
		iconData.hWnd = (HWND)xid;
		iconData.uFlags = 0;
	
		Shell_NotifyIcon(NIM_DELETE, &iconData);
	}
#endif

	FXEmbeddedWindow::destroy();
}

void FXTray::setTipText(const FXString& txt) {
	text = txt;
#ifdef WIN32
	if(xid != NULL) {
		NOTIFYICONDATA iconData;
		iconData.cbSize = _NOTIFYICONDATA_V1_SIZE;
		iconData.uID = (int)(void*)this;
		iconData.hWnd = (HWND)xid;
		iconData.uFlags = NIF_TIP;

		// Note that the size of the szTip field is different in version 5.0 of shell32.dll.
		FXuint tipMaxLen = 64; // TODO SHELL32_MAJOR < 5? 64: 128;
		memset(iconData.szTip, 0, tipMaxLen);
		strncpy(iconData.szTip, txt.text(), tipMaxLen - 1);

		Shell_NotifyIcon(NIM_MODIFY, &iconData);
	}
#endif
}

void FXTray::setIcon(FXIcon* ic) {
	icon = ic;

#ifdef WIN32
	if(xid != NULL) {
		if(iconId != NULL) {
			DeleteObject(iconId);
			iconId = NULL;
		}

		if(ic != NULL) {
			ICONINFO iconInfo;
			iconInfo.fIcon = TRUE;
			iconInfo.hbmColor = isEnabled()? (HBITMAP)ic->id(): (HBITMAP)FXIconAccess::getEtchPixmap(ic);
			iconInfo.hbmMask = (HBITMAP)FXIconAccess::getShapePixmap(ic);

			iconId = CreateIconIndirect(&iconInfo);
		}

		NOTIFYICONDATA iconData;
		iconData.cbSize = _NOTIFYICONDATA_V1_SIZE;
		iconData.uID = (int)(void*)this;
		iconData.hWnd = (HWND)xid;
		iconData.uFlags = NIF_ICON;
		iconData.hIcon = (HICON)iconId;

		Shell_NotifyIcon(NIM_MODIFY, &iconData);
	}
#else
	FXint width, height;
	
	if(ic != NULL) {
		width = FXMAX(ic->getWidth(), 1);
		height = FXMAX(ic->getHeight(), 1);
	} else
		width = height = 1;
	
	XSizeHints size;

	size.flags = PMinSize|PMaxSize|PBaseSize;

	size.min_width 		= width;
	size.min_height 	= height;
	size.max_width 		= width;
	size.max_height 	= height;
	size.base_width 	= width;
	size.base_height	= height;

    XSetWMNormalHints(DISPLAY(), xid, &size);

	update();
#endif
}

void FXTray::enable() {
	FXEmbeddedWindow::enable();
#ifdef WIN32
	setIcon(icon);
#else
	update();
#endif
}

void FXTray::disable() {
	FXEmbeddedWindow::disable();
#ifdef WIN32
	setIcon(icon);
#else
	update();
#endif
}

FXbool FXTray::shown() const {
#ifdef WIN32
	return visible;
#else
	return FXEmbeddedWindow::shown();
#endif
}

void FXTray::show() {
#ifdef WIN32
	visible = TRUE;
#else
	FXEmbeddedWindow::show();
#endif
}

void FXTray::hide() {
#ifdef WIN32
	visible = FALSE;
#else
	FXEmbeddedWindow::hide();
#endif
}

void FXTray::hostingStarted() {
#ifndef WIN32
	// Our window should have the background of the parent window
	// This doesn't currently work for IceWM though
	XSetWindowAttributes sattr;
	sattr.background_pixmap = ParentRelative;
	XChangeWindowAttributes(DISPLAY(), WINDOW(xid), CWBackPixmap, &sattr);

	update();
#endif
}

#ifndef WIN32
void FXTray::sendTrayManagerClientEvent(int time, int message, int detail, int data1, int data2) {
	FXEmbedApp* app = (FXEmbedApp*)getApp();
	app->sendWindowClientEvent(managerWindow, time, sysTrayOpcodeAtom, message, detail, data1, data2);
}

void FXTray::updateTrayManager() {
	XGrabServer(DISPLAY());

	managerWindow = XGetSelectionOwner(DISPLAY(), sysTrayAtom);
	if(managerWindow != NULL)	
		XSelectInput(DISPLAY(), managerWindow, StructureNotifyMask|PropertyChangeMask);
	
	XUngrabServer(DISPLAY());
	XFlush(DISPLAY());

	if(managerWindow != NULL)
		sendTrayManagerClientEvent(0, SYSTEM_TRAY_REQUEST_DOCK, (int)xid, 0, 0);
}
#endif

long FXTray::onPaint(FXObject* sender, FXSelector sel, void* ptr) {
#ifndef WIN32
	// This will paint custom background
	// FXEmbeddedWindow::onPaint(sender, sel, ptr);
	
	if(icon != NULL) {
		FXDCWindow dc(this);
		if(isEnabled())
			dc.drawIcon(icon, 
				getWidth() > icon->getWidth()? (getWidth() - icon->getWidth())/2: 0, 
				getHeight() > icon->getHeight()? (getHeight() - icon->getHeight())/2: 0);
		else
			dc.drawIconSunken(icon, 
				getWidth() > icon->getWidth()? (getWidth() - icon->getWidth())/2: 0, 
				getHeight() > icon->getHeight()? (getHeight() - icon->getHeight())/2: 0);
	}
#endif

	return 0;
}

// Set tip using a message
long FXTray::onCmdSetTip(FXObject*, FXSelector, void* ptr) {
  	setTipText(*(FXString*)ptr);
  	return 1;
}

// Get tip using a message
long FXTray::onCmdGetTip(FXObject*, FXSelector, void* ptr) {
  	*((FXString*)ptr) = text;
  	return 1;
}

// We were asked about tip text
long FXTray::onQueryTip(FXObject* sender, FXSelector, void*) {
#ifndef WIN32
  	if(!text.empty() && (flags&FLAG_TIP) != 0) {
    	sender->handle(this, FXSEL(SEL_COMMAND, ID_SETSTRINGVALUE), (void*)&text);
    	return 1;
    }
#endif
  	
  	return 0;
}

// Pressed mouse button
long FXTray::onLeftBtnPress(FXObject* sender, FXSelector sel, void* ptr) {
#ifndef WIN32
	FXShell::onLeftBtnPress(sender, sel, ptr);

	FXEvent* event = (FXEvent*)ptr;
	handle(this, FXSEL(SEL_COMMAND, event->click_count > 1? ID_TRAY_DBLCLICKED: ID_TRAY_CLICKED), 0);
#endif

    return 0;
}

// Released mouse button
long FXTray::onRightBtnRelease(FXObject* sender, FXSelector sel, void* ptr) {
#ifndef WIN32
	FXShell::onRightBtnRelease(sender, sel, ptr);

	FXEvent* event = (FXEvent*)ptr;
	if(!event->moved)
		handle(this, FXSEL(SEL_COMMAND, ID_TRAY_CONTEXT_MENU), 0);
#endif

    return 0;
}

long FXTray::onCmdTrayClicked(FXObject* sender, FXSelector sel, void* ptr) {
	if(target != NULL)
		return target->handle(sender, sel, ptr);

	return 0;
}

long FXTray::onCmdTrayDblClicked(FXObject* sender, FXSelector sel, void* ptr) {
	if(target != NULL)
		return target->handle(sender, sel, ptr);

	return 0;
}

long FXTray::onCmdTrayContextMenu(FXObject* sender, FXSelector sel, void* ptr) {
	if(target != NULL)
		return target->handle(sender, sel, ptr);

	return 0;
}
