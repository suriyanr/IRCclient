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

#include "stdafx.h"
#include "FXEmbedApp.h"
#include "FXEmbeddedWindow.h"
#include "FXTray.h"
#include "Icons.hpp"

#include <iostream>
#include <unistd.h>

using namespace std;

class MyWindow : public FXMainWindow {

    FXDECLARE(MyWindow)

 private:
    MyWindow() {}

 public:
    enum {
       ID_MAINWIN=FXMainWindow::ID_LAST,
       ID_TRAY_QUIT,
    };
    MyWindow(FXApp *);
    virtual ~MyWindow();
    virtual void create();

    long onClose(FXObject*,FXSelector sel,void*);
    long onTrayRestore(FXObject*,FXSelector sel,void*);
    long onTrayDblClick(FXObject*,FXSelector sel,void*);
    long onTrayMenu(FXObject*,FXSelector sel,void*);
    long onTrayQuit(FXObject*,FXSelector sel,void*);

    FXMenuPane *TrayMenu;
};

FXDEFMAP(MyWindow) MyWindowMap[]={
   FXMAPFUNC(SEL_CLOSE, 0, MyWindow::onClose),
   FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_CLICKED, MyWindow::onTrayRestore),
   FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_DBLCLICKED, MyWindow::onTrayDblClick),
   FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_CONTEXT_MENU, MyWindow::onTrayMenu),
   FXMAPFUNC(SEL_COMMAND, MyWindow::ID_TRAY_QUIT, MyWindow::onTrayQuit),
   0
};


FXIMPLEMENT(MyWindow,FXMainWindow,MyWindowMap,ARRAYNUMBER(MyWindowMap))

 MyWindow::MyWindow(FXApp * a)
    :FXMainWindow(a,"The Window Title",NULL,NULL,DECOR_ALL,0,0,800,600){

   // Create the TrayMenu
   TrayMenu = new FXMenuPane(this);
   new FXMenuCommand(TrayMenu, "Exit", NULL, this, ID_TRAY_QUIT);

 }

 MyWindow::~MyWindow(){
    delete TrayMenu;
 }

 void MyWindow::create(){
    FXMainWindow::create();
    show(PLACEMENT_SCREEN);
 }

long MyWindow::onTrayQuit(FXObject*,FXSelector sel,void*) {

   // Pop down menu
   TrayMenu->popdown();

   if (FXMessageBox::question(this, MBOX_YES_NO, "Close", "Close pressed") == MBOX_CLICKED_YES) {
       getApp()->exit(0);
       return(0);
   }

   return(1);
}

 long MyWindow::onClose(FXObject*,FXSelector sel,void*)
 {
    cout << "MyWindow::onClose" << endl;

       // Do not exit application but, minimise in tray
       this->hide();
       return(1);
}

long MyWindow::onTrayMenu(FXObject*,FXSelector sel,void*) {
FXint x, y;
FXuint buttons;
static bool TrayMenuCreated = false;

   cout << "MyWindow::onTrayMenu" << endl;

   getRoot()->getCursorPosition(x ,y, buttons);

   // As this will be in tray bottom/right, we need to move it up for windows.
#ifdef __MINGW32__
   y = y - 35;
#endif

   if (TrayMenuCreated == false) {
      TrayMenuCreated = true;
      TrayMenu->create();
   }

   TrayMenu->popup(NULL, x, y);

   return(1);
}

long MyWindow::onTrayDblClick(FXObject*,FXSelector sel,void*)
 {
   cout << "MyWindow::onTrayDblClick" << endl;
   return(1);
}

long MyWindow::onTrayRestore(FXObject*,FXSelector sel,void*)
 {
   cout << "MyWindow::onTrayRestore" << endl;
   this->show();
   return(1);
}


int main(int argc,char ** argv) {
 
   FXEmbedApp application("FXTray","FOX Tray");
 
   application.init(argc,argv);
   application.create();

   FXICOIcon *tray_icon = new FXICOIcon(&application, TrayIcon);
   tray_icon->create();

 
   FXTray* Tray = new FXTray(&application);
   Tray->create();
   Tray->setTipText("Tray Tip");
   Tray->setIcon(tray_icon);

   MyWindow *Window = new MyWindow(&application);
   Window->create();

   Tray->setTarget(Window);
   Tray->enable();
   Tray->show();

   application.run();

   delete tray_icon;
   delete Tray;

 } 
