/********************************************************************************
*                                                                               *
*    W i n d o w   W i d g e t   H o s t i n g   X E M B E D   C l i e n t s    *
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
* $Id: FXEmbedderWindow.h,v 1.1 2005/09/25 02:01:32 ArtDvdHike Exp $                  *
********************************************************************************/
#ifndef FXEMBEDDERWINDOW_H
#define FXEMBEDDERWINDOW_H

class FXEmbedderWindow: public FXComposite {
FXDECLARE(FXEmbedderWindow)
friend class FXEmbedApp;	
protected:
#ifdef WIN32
	FXID savedFocusWindow;
#else 
	FXID embeddedWindow;
#endif
protected:
	FXEmbedderWindow() {}

	FXbool isForeignWindow(FXID window) const;
	FXID findEmbeddedWindow() const;

	virtual FXbool preprocessEvent(FXRawEvent&);

#ifndef WIN32
	void registerEmbeddedWindow(FXID window);
	void unregisterEmbeddedWindow();
	
	FXbool forwardEmbeddedWindowKeyEvent();
	void sendEmbeddedWindowClientEvent(int time, int message, int detail, int data1, int data2);
	void updateEmbeddedWindowMap();
#endif
private:
  	FXEmbedderWindow(const FXEmbedderWindow&);
  	FXEmbedderWindow& operator=(const FXEmbedderWindow&);
public:
  	/// Constructor
  	FXEmbedderWindow(FXComposite* p, FXObject* tgt = NULL, FXuint opts = 0, FXint x = 0, FXint y = 0, FXint w = 0, FXint h = 0);

  	/// Destructor
  	virtual ~FXEmbedderWindow();

  	/// Create server-side resources
  	virtual void create();

  	/// Detach server-side resources
  	virtual void detach();

  	/// Destroy server-side resources
  	virtual void destroy();

  	/// Enable the window to receive mouse and keyboard events
  	virtual void enable();

  	/// Disable the window from receiving mouse and keyboard events
  	virtual void disable();

	void attachEmbeddedWindow(FXID window);
	void detachEmbeddedWindow();

	FXID getEmbeddedWindow() const;
		
	void focusNext();
	void focusPrev();
	
	virtual FXbool canFocus() const {return TRUE;}

  	long onConfigure(FXObject*, FXSelector, void*);
  	long onKeyPress(FXObject*, FXSelector, void*);
  	long onKeyRelease(FXObject*, FXSelector, void*);
  	long onFocusIn(FXObject*, FXSelector, void*);
  	long onFocusOut(FXObject*, FXSelector, void*);
  	long onFocusNext(FXObject*, FXSelector, void*);
  	long onFocusPrev(FXObject*, FXSelector, void*);
};
#endif
