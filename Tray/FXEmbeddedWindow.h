/********************************************************************************
*                                                                               *
*     W i n d o w   W i d g e t   E m b e d d a b l e   V i a   X E M B E D     *
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
* $Id: FXEmbeddedWindow.h,v 1.1 2005/09/14 04:09:04 ArtDvdHike Exp $                  *
********************************************************************************/
#ifndef FXEMBEDDEDWINDOW_H
#define FXEMBEDDEDWINDOW_H

class FXEmbeddedWindow: public FXShell {
FXDECLARE(FXEmbeddedWindow)
friend class FXEmbedApp;	
protected:
	FXID embedderWindow;

	FXEmbeddedWindow() {}

  	virtual FXbool preprocessEvent(FXRawEvent&);
private:
  	FXEmbeddedWindow(const FXEmbeddedWindow&);
  	FXEmbeddedWindow& operator=(const FXEmbeddedWindow&);
public:
  	/// Constructor
  	FXEmbeddedWindow(FXApp* a, FXID id, FXObject* tgt = NULL);

  	/// Destructor
  	virtual ~FXEmbeddedWindow();

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

  	/// Show this window
  	virtual void show();

  	/// Hide this window
  	virtual void hide();
  	
  	/// Move the focus to this window
  	virtual void setFocus();

  	/// Remove the focus from this window
  	virtual void killFocus();
  	
protected:
	virtual void hostingStarted();
	virtual void hostingEnded();
	
	virtual void embeddingNotify();

#ifndef WIN32
	void registerEmbedderWindow(FXID window);
	void unregisterEmbedderWindow();

	void sendEmbedderWindowClientEvent(int time, int message, int detail, int data1, int data2);
#endif
};
#endif
