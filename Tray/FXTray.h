/********************************************************************************
*                                                                               *
*                      T r a y   I c o n   W i d g e t                          *
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
* $Id: FXTray.h,v 1.1 2005/09/14 04:09:04 ArtDvdHike Exp $                     *
********************************************************************************/
#ifndef FXTRAY_H
#define FXTRAY_H

class FXTray: public FXEmbeddedWindow {
FXDECLARE(FXTray)
protected:
#ifdef WIN32
	FXID				iconId;
	FXbool				visible;
#else
	FXID				managerWindow;
	
	int					sysTrayAtom;
	int					sysTrayOpcodeAtom;
	int					sysTrayMessageDataAtom;
#endif

	FXString			text;
	FXIcon*				icon;

	FXTray() {}
	
#ifndef WIN32
	void sendTrayManagerClientEvent(int time, int message, int detail, int data1, int data2);
	void updateTrayManager();
#endif

public:
	enum {
		ID_TRAY_CLICKED = FXEmbeddedWindow::ID_LAST,
		ID_TRAY_DBLCLICKED,
		ID_TRAY_CONTEXT_MENU,
		
		ID_LAST
	};

	FXTray(FXApp* app, const FXString& txt = "", FXIcon* ic = NULL, FXObject* tgt = NULL);
	virtual ~FXTray();

	virtual void create();
	virtual void destroy();

	FXIcon* getIcon() const {return icon;}
	FXString getTipText() const {return text;}
	
	void setIcon(FXIcon* icon);
	void setTipText(const FXString& txt);

  	/// Return true if the window is shown
  	virtual FXbool shown() const;

  	/// Show this window
	virtual void show();
	
  	/// Hide this window
	virtual void hide();

  	/// Enable the window to receive mouse and keyboard events
  	virtual void enable();

  	/// Disable the window from receiving mouse and keyboard events
  	virtual void disable();

	virtual void hostingStarted();
	
	long onPaint(FXObject*, FXSelector, void*);
  	long onLeftBtnPress(FXObject*, FXSelector, void*);
  	long onRightBtnRelease(FXObject*, FXSelector, void*);
  	long onCmdSetTip(FXObject*, FXSelector, void*);
  	long onCmdGetTip(FXObject*, FXSelector, void*);
  	long onQueryTip(FXObject*, FXSelector, void*);
	
	long onCmdTrayClicked(FXObject*, FXSelector, void*);
	long onCmdTrayDblClicked(FXObject*, FXSelector, void*);
	long onCmdTrayContextMenu(FXObject*, FXSelector, void*);
};
#endif
