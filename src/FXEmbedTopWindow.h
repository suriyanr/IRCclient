/********************************************************************************
*                                                                               *
*    T o p - L e v e l   W i d g e t   W i t h   X E M B E D   S u p p o r t    *
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
* $Id: FXEmbedTopWindow.h,v 1.1 2005/09/25 02:01:32 ArtDvdHike Exp $                  *
********************************************************************************/
#ifndef FXEMBEDTOPWINDOW_H
#define FXEMBEDTOPWINDOW_H

class FXEmbedTopWindow: public FXTopWindow {
FXDECLARE(FXEmbedTopWindow)
friend class FXEmbedApp;
friend class FXEmbedderWindow;
protected:
#ifndef WIN32
	FXWindow* focusProxy;
#endif
	
public:
	FXEmbedTopWindow(FXApp* a,const FXString& name,FXIcon *ic=NULL,FXIcon *mi=NULL,FXuint opts=DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0,FXint hs=0,FXint vs=0):
		FXTopWindow(a, name, ic, mi, opts, x, y, w, h, pl, pr, pt, pb, hs, vs)
#ifndef WIN32		
		, focusProxy(new FXWindow(this, 0, -1, -1, 1, 1))
#endif		
	{}

	FXEmbedTopWindow(FXWindow* owner,const FXString& name,FXIcon *ic=NULL,FXIcon *mi=NULL,FXuint opts=DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0,FXint hs=0,FXint vs=0):
		FXTopWindow(owner, name, ic, mi, opts, x, y, w, h, pl, pr, pt, pb, hs, vs)
#ifndef WIN32		
		, focusProxy(new FXWindow(this, 0, -1, -1, 1, 1))
#endif		
	{}

	virtual ~FXEmbedTopWindow() {
#ifndef WIN32		
		focusProxy = NULL;
#endif
	}
	
	virtual void create();
	
  	virtual void setFocus();
  	virtual void killFocus();

protected:
	FXEmbedTopWindow() {}
private:
  	FXEmbedTopWindow(const FXEmbedTopWindow&): FXTopWindow() {}
  	FXEmbedTopWindow &operator=(const FXEmbedTopWindow&) {return *this;}
};
#endif
