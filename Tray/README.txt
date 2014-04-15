Reasons why Tray is not in main line:
--------------------------------------

1. When minimising the window, we catch it in Linux but not in Windows.
   I had asked for this info in the forums, and I followed as answer given.
   Still doesnt work.
   Code as below:
 
 ---- mywindow.h 8< ----
 class MyWindow : public FXMainWindow {
 
    FXDECLARE(MyWindow)
 
 private:
    MyWindow() {}
 
 public:
    enum {
       ID_MAINWIN=FXMainWindow::ID_LAST,
    };
    MyWindow(FXApp *);
    virtual ~MyWindow();
    virtual void create();
 
    long onMinimize(FXObject*,FXSelector sel,void*);
    long onClose(FXObject*,FXSelector sel,void*);
 };
 ----- 8< --------------
 
 The cpp file: mywindow.cpp
 ----- 8< mywindow.cpp ------
 #include <fx.h>
 #include "mywindow.h"
 
 FXDEFMAP(MyWindow) MyWindowMap[]={
   FXMAPFUNC(SEL_UNMAP, MyWindow::ID_MAINWIN,
 MyWindow::onMinimize),
   FXMAPFUNC(SEL_CLOSE, 0, MyWindow::onClose),
   0
 };
 
 FXIMPLEMENT(MyWindow,FXMainWindow,MyWindowMap,ARRAYNUMBER(MyWindowMap))
 
 MyWindow::MyWindow(FXApp * a)
    :FXMainWindow(a,"The Window
 Title",NULL,NULL,DECOR_ALL,0,0,800,600){
 
    this->setTarget(this);
    this->setSelector(ID_MAINWIN);
 }
 
 MyWindow::~MyWindow(){
 }
 
 void MyWindow::create(){
    FXMainWindow::create();
    show(PLACEMENT_SCREEN);
 }
 
 long MyWindow::onMinimize(FXObject*,FXSelector
 sel,void*) {
 
    if ( (FXSELID(sel) == ID_MAINWIN) &&
         (FXSELTYPE(sel) == SEL_UNMAP) ) {
       FXMessageBox::information(this, MBOX_OK,
 "Minimize", "Minimize pressed");
    }
    return(1);
 }
 
 long MyWindow::onClose(FXObject*,FXSelector sel,void*)
 {
 
    if (FXMessageBox::question(this, MBOX_YES_NO,
 "Close", "Close pressed") == MB
 OX_CLICKED_YES) {
       getApp()->exit(0);
       return(0);
    }
    else {
       return(1);
    }
 }
 ------- 8< -------

 So the best thing is to let minimize act the way it does. Change the Close
 coming from X in window to act as minimize to tray.
 Make sure File->Exit, exits program though instead of minimizing it.

2. I am not able to redirect a click on the Tray icon to my function.
   Posted this in the swt-fox forum and received the below reply:
   You should subscribe an FXObject to FXTray by using FXTray::setTarget(...).
   This object will receive SEL_COMMAND messages with the following IDs: 
   ID_TRAY_CLICKED - when you single click on the icon 
   ID_TRAY_DBLCLICKED - when you double-click on the icon 
   ID_TRAY_CONTEXT_MENU - when you right-click on the icon 

   ah, so I had created an application of type FXApp. To get the new clicks
   I should create application from FXEmbedApp.

3. FXEmbedTopWindow.cpp and FXEmbedTopWindow.h is not required. Though the .h
   file is included in a couple of cpp files, we can safely comment out that
   include.
   OK its not required for Windows port.
   It is required for Linux port.
   So we keep it all.
