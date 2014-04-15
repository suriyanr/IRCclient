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
*    T o p - L e v e l   W i d g e t   W i t h   X E M B E D   S u p p o r t    *
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
* $Id: FXEmbedTopWindow.cpp,v 1.1 2005/09/25 02:01:32 ArtDvdHike Exp $         *
********************************************************************************/
#include "stdafx.h"
#include "FXEmbedTopWindow.h"
#include "FXEmbedApp.h"

// Map
FXDEFMAP(FXEmbedTopWindow) FXEmbedTopWindowMap[] = {
  	FXMAPFUNC(SEL_FOCUSIN, 0, FXEmbedTopWindow::onFocusIn),
  	FXMAPFUNC(SEL_FOCUSOUT, 0, FXEmbedTopWindow::onFocusOut)
};

FXIMPLEMENT(FXEmbedTopWindow, FXTopWindow, FXEmbedTopWindowMap, ARRAYNUMBER(FXEmbedTopWindowMap))

void FXEmbedTopWindow::create() {
#ifdef WIN32	
	FXTopWindow::create();
#else
	FXbool idCreated = xid != NULL;
	FXTopWindow::create();
	if(!idCreated) {
		focusProxy->enable();
		focusProxy->show();

		((FXEmbedApp*)getApp())->addXInput((Window)focusProxy->id(), FocusChangeMask);
	}
#endif
}

void FXEmbedTopWindow::setFocus() {
  FXShell::setFocus();
  if(xid){
#ifndef WIN32
    XSetInputFocus((Display*)getApp()->getDisplay(),(Window)focusProxy->id(),RevertToPointerRoot,((FXEmbedApp*)getApp())->getServerTime(xid));
#else
	// Keep the focus inside the main window for all native Fox windows
	// For embedded widgets, do not change the focus to the main window as these know nothing about Fox
	// but only about Win32 native focus events

	HWND hwnd = GetFocus();
	if(getApp()->findWindowWithId(hwnd) != NULL || !IsChild((HWND)xid, hwnd))
		// Fox window or not a child window of this Fox toplevel window
		SetFocus((HWND)xid);
#endif
    }
  }

void FXEmbedTopWindow::killFocus() {
  FXShell::killFocus();
  if(xid){
#ifndef WIN32
    Window win;
    int    dum;
    XGetInputFocus((Display*)getApp()->getDisplay(),&win,&dum);
    if(win==(Window)focusProxy->id()){
      if(getOwner() && getOwner()->id()){
        getOwner()->setFocus();
        }
      else{
        XSetInputFocus((Display*)getApp()->getDisplay(),PointerRoot,RevertToPointerRoot,((FXEmbedApp*)getApp())->getServerTime(xid));
        }
      }
#else
    if(GetFocus()==(HWND)xid){
      if(getOwner() && getOwner()->id()){
        SetFocus((HWND)getOwner()->id());
        }
      else{
        SetFocus((HWND)NULL);
        }
      }
#endif
    }
}
