/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWApplication.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWWindowCollection.h"
#ifdef _WIN32
#include <htmlhelp.h>
#endif
#include "vtkKWObject.h"
#include "vtkTclUtil.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkKWWindow.h"
#include "vtkKWEventNotifier.h"
#include "kwinit.h"


int vtkKWApplication::WidgetVisibility = 1;


//------------------------------------------------------------------------------
vtkKWApplication* vtkKWApplication::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWApplication");
  if(ret)
    {
    return (vtkKWApplication*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWApplication;
}




extern "C" int Vtktcl_Init(Tcl_Interp *interp);
extern "C" int Vtkkwwidgetstcl_Init(Tcl_Interp *interp);

int vtkKWApplicationCommand(ClientData cd, Tcl_Interp *interp,
			    int argc, char *argv[]);

vtkKWApplication::vtkKWApplication()
{
  this->CommandFunction = vtkKWApplicationCommand;
  
  this->ApplicationName = new char[strlen("Kitware")+1];
  strcpy(this->ApplicationName, "Kitware" );

  this->ApplicationVersionName = new char[strlen("Kitware10")+1];
  strcpy(this->ApplicationVersionName, "Kitware10" );

  this->ApplicationReleaseName = new char[strlen("unknown")+1];
  strcpy(this->ApplicationReleaseName, "unknown" );

  // setup tcl stuff
  this->MainInterp = Et_Interp;
  this->Windows = vtkKWWindowCollection::New();  
  
  // add the application as $app
	
  //vtkTclGetObjectFromPointer(this->MainInterp, (void *)this, 
  //                           vtkKWApplicationCommand);

  //this->Script("set Application %s",this->MainInterp->result);
  this->Script("set Application %s",this->GetTclName());

  this->BalloonHelpWindow = vtkKWWidget::New();
  this->BalloonHelpLabel = vtkKWWidget::New();
  this->BalloonHelpLabel->SetParent(this->BalloonHelpWindow);
  this->BalloonHelpPending = NULL;

  if (vtkKWApplication::WidgetVisibility)
    {
    //this->BalloonHelpWindow->SetParent(this->GetParentWindow());
    this->BalloonHelpWindow->Create(this, "toplevel", "-background black -borderwidth 1 -relief flat");
    this->BalloonHelpLabel->Create(this, "label", "-background LightYellow -justify left -wraplength 2i");
    this->Script("pack %s", this->BalloonHelpLabel->GetWidgetName());
    this->Script("wm overrideredirect %s 1", this->BalloonHelpWindow->GetWidgetName());
    this->Script("wm withdraw %s", this->BalloonHelpWindow->GetWidgetName());
    }
  
  this->EventNotifier = vtkKWEventNotifier::New();
  this->EventNotifier->SetApplication( this );
}

vtkKWApplication::~vtkKWApplication()
{
  if (this->Windows)
    {
    this->Windows->Delete();
    this->Windows = NULL;
    this->MainInterp = NULL;
    vtkObjectFactory::UnRegisterAllFactories();
    }

  this->SetApplicationName(NULL);
  this->SetApplicationVersionName(NULL);
  this->SetApplicationReleaseName(NULL);
}

void vtkKWApplication::Script(const char *format, ...)
{
  static char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->SimpleScript(event);
}
void vtkKWApplication::SimpleScript(char *event)
{
//#define VTK_DEBUG_SCRIPT
#ifdef VTK_DEBUG_SCRIPT
    vtkOutputWindow::GetInstance()->DisplayText(event);
    vtkOutputWindow::GetInstance()->DisplayText("\n");
#endif
  
  if (Tcl_GlobalEval(this->MainInterp, event) != TCL_OK)
    {
    vtkErrorMacro("\n    Script: \n" << event << "\n    Returned Error: \n"  
		  << this->MainInterp->result << endl);
    }
}

void vtkKWApplication::SetApplicationName(const char *_arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ApplicationName to " << _arg ); 
  if ( this->ApplicationName && _arg && (!strcmp(this->ApplicationName,_arg)))
    { 
    return;
    } 
  if (this->ApplicationName) 
    { 
    delete [] this->ApplicationName; 
    } 
  if (_arg) 
    { 
    this->ApplicationName = new char[strlen(_arg)+1]; 
    strcpy(this->ApplicationName,_arg); 
    } 
   else 
    { 
    this->ApplicationName = NULL; 
    } 
  this->Modified(); 
}

void vtkKWApplication::SetApplicationVersionName(const char *_arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ApplicationVersionName to " << _arg ); 
  if ( this->ApplicationVersionName && _arg && (!strcmp(this->ApplicationVersionName,_arg)))
    { 
    return;
    } 
  if (this->ApplicationVersionName) 
    { 
    delete [] this->ApplicationVersionName; 
    } 
  if (_arg) 
    { 
    this->ApplicationVersionName = new char[strlen(_arg)+1]; 
    strcpy(this->ApplicationVersionName,_arg); 
    } 
   else 
    { 
    this->ApplicationVersionName = NULL; 
    } 
  this->Modified(); 
}

void vtkKWApplication::SetApplicationReleaseName(const char *_arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ApplicationReleaseName to " << _arg ); 
  if ( this->ApplicationReleaseName && _arg && (!strcmp(this->ApplicationReleaseName,_arg)))
    { 
    return;
    } 
  if (this->ApplicationReleaseName) 
    { 
    delete [] this->ApplicationReleaseName; 
    } 
  if (_arg) 
    { 
    this->ApplicationReleaseName = new char[strlen(_arg)+1]; 
    strcpy(this->ApplicationReleaseName,_arg); 
    } 
   else 
    { 
    this->ApplicationReleaseName = NULL; 
    } 
  this->Modified(); 
}

void vtkKWApplication::Close(vtkKWWindow *win)
{
  this->Windows->RemoveItem(win);
  if (this->Windows->GetNumberOfItems() < 1)
    {
    this->Exit();
    }
}

vtkKWWindowCollection *vtkKWApplication::GetWindows()
{
  return this->Windows;
}

void vtkKWApplication::AddWindow(vtkKWWindow *w)
{
  this->Windows->AddItem(w);
}

void vtkKWApplication::Exit()
{
  vtkKWWindow* win = 0;
  this->Windows->InitTraversal();
  while (this->Windows && (win = this->Windows->GetNextKWWindow()))
    {
    win->Close();
    if (this->Windows)
      {
      this->Windows->InitTraversal();
      }
    }
  
  // remove the event notifier which otherwise would cause a ref count loop
  if (this->EventNotifier)
    {
    this->EventNotifier->Delete();
    this->EventNotifier = NULL;
    }
  this->SetBalloonHelpPending(NULL);
  if (this->BalloonHelpWindow)
    {
    this->BalloonHelpWindow->Delete();
    this->BalloonHelpWindow = NULL;
    }
  if (this->BalloonHelpLabel)
    {
    this->BalloonHelpLabel->Delete();
    this->BalloonHelpLabel = NULL;
    }
  this->SetBalloonHelpPending(NULL);
  return;
}
    
#define ET_TCL_LIBRARY "C:/Program Files/Tcl/lib/tcl8.2"
#define ET_TK_LIBRARY "C:/Program Files/Tcl/lib/tk8.2"
Tcl_Interp *vtkKWApplication::InitializeTcl(int argc, char *argv[])
{
  Tcl_Interp *interp;
  
  putenv("TCL_LIBRARY=" ET_TCL_LIBRARY);
  putenv("TK_LIBRARY=" ET_TK_LIBRARY);
  
  Tcl_FindExecutable(argv[0]);
  interp = Tcl_CreateInterp();
  Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

  Et_DoInit(interp);
  
  // initialize VTK
  Vtktcl_Init(interp);

  // initialize Widgets
  if (vtkKWApplication::WidgetVisibility)
    {
    Vtkkwwidgetstcl_Init(interp);
    }

  return interp;
}

void vtkKWApplication::Start()
{ 
  int i;
  
  // look at Tcl for any args
  this->Script("set argc");
  int argc = vtkKWObject::GetIntegerResult(this) + 1;
  char **argv = new char *[argc];
  argv[0] = NULL;
  for (i = 1; i < argc; i++)
    {
    this->Script("lindex $argv %d",i-1);
    argv[i] = strdup(this->GetMainInterp()->result);
    }
  this->Start(argc,argv);
  
  for (i = 0; i < argc; i++)
    {
    if (argv[i])
      {
      free(argv[i]);
      }
    }
  delete [] argv;
}
void vtkKWApplication::Start(char *arg)
{ 
  this->Start(1,&arg);
}
void vtkKWApplication::Start(int argc, char *argv[])
{ 
  while (this->Windows->GetNumberOfItems())
    {
    Tcl_DoOneEvent(0);
    }
  
  //Tk_MainLoop();
}


#ifdef _WIN32
extern void ReadAValue(HKEY hKey,char *val,char *key, char *adefault);
#endif

void vtkKWApplication::DisplayHelp()
{
#ifdef _WIN32
  char temp[1024];
  char fkey[1024];
  char loc[1024];
  sprintf(fkey,"Software\\Kitware\\%i\\Inst",this->GetApplicationKey());  
  HKEY hKey;
  if(RegOpenKeyEx(HKEY_CURRENT_USER, fkey, 
		  0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
    ReadAValue(hKey, loc,"Loc","");
    RegCloseKey(hKey);
    sprintf(temp,"%s/%s.chm::/Introduction/Introduction.htm",
            loc,this->ApplicationName);
    }
  else
    {
    sprintf(temp,"%s.chm::/Introduction/Introduction.htm",this->ApplicationName);
    }
  HtmlHelp(NULL, temp, HH_DISPLAY_TOPIC, 0);
#else
  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->Create(this,"");
  dlg->SetText(
    "HTML help is included in the help subdirectory of\n"
    "this application. You can view this help using a\n"
    "standard web browser by loading the Help.htm file.");
  dlg->Invoke();  
  dlg->Delete();
#endif
}


//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpTrigger(vtkKWWidget *widget)
{
  char *result;

  // If there is no help string, return
  if ( !widget->GetBalloonHelpString() )
    {
    this->SetBalloonHelpPending(NULL);
    return;
    }
  
  this->BalloonHelpCancel();
  this->Script("after 2000 {catch {%s BalloonHelpDisplay %s}}", 
               this->GetTclName(), widget->GetTclName());
  result = this->GetMainInterp()->result;
  this->SetBalloonHelpPending(result);
}


//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpDisplay(vtkKWWidget *widget)
{
  int x, y;

  // If there is no help string, return
  if ( !widget->GetBalloonHelpString() )
    {
    this->SetBalloonHelpPending(NULL);
    return;
    }
  
  // make sure it is really pending
  this->Script("%s configure -text {%s}", 
               this->BalloonHelpLabel->GetWidgetName(), 
               widget->GetBalloonHelpString());

  // Get the position of the mouse in the renderer.
  this->Script( "winfo pointerx %s", widget->GetWidgetName());
  x = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo pointery %s", widget->GetWidgetName());
  y = vtkKWObject::GetIntegerResult(this);

  // Get the position of the parent widget of the one needing help
  this->Script( "winfo rootx %s", widget->GetParent()->GetWidgetName());
  int xw = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo rooty %s", widget->GetParent()->GetWidgetName());
  int yw = vtkKWObject::GetIntegerResult(this);

  // get the size of the balloon window
  this->Script( "winfo reqwidth %s", this->BalloonHelpLabel->GetWidgetName());
  int dx = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo reqheight %s", this->BalloonHelpLabel->GetWidgetName());
  int dy = vtkKWObject::GetIntegerResult(this);
  
  // get the size of the parent window of the one needing help
  this->Script( "winfo width %s", widget->GetParent()->GetWidgetName());
  int dxw = vtkKWObject::GetIntegerResult(this);
  this->Script( "winfo height %s", widget->GetParent()->GetWidgetName());
  int dyw = vtkKWObject::GetIntegerResult(this);
  
  // Set the position of the window relative to the mouse.
  int just = widget->GetBalloonHelpJustification();

  // just 0 == left just 2 == right
  if (just)
    {
    if (x + dx > xw + dxw)
      {
      x = xw + dxw - dx;
      }
    }
  // with left justification (default) still try to keep the 
  // help from going past the right edge of the widget
  else
    {
     // if it goes too far right
    if (x + dx > xw + dxw)
      {
      // move it to the left
      x = xw + dxw - dx;
      // but not past the left edge of the parent widget
      if (x < xw)
        {
        x = xw;
        }
      }
    }
  
  this->Script("wm geometry %s +%d+%d",
               this->BalloonHelpWindow->GetWidgetName(), x, y+15);
  this->Script("update");

  // map the window
  if (this->BalloonHelpPending)
    {
    this->Script("wm deiconify %s", this->BalloonHelpWindow->GetWidgetName());
    this->Script("raise %s", this->BalloonHelpWindow->GetWidgetName());
    
    // remove the balloon help if the mouse moves
    this->Script("bind %s <Motion> {%s BalloonHelpWithdraw}", 
                 widget->GetWidgetName(), this->GetTclName() );
    }
  
  this->SetBalloonHelpPending(NULL);
}


//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpCancel()
{
  if (this->BalloonHelpPending)
    {
    this->Script("after cancel %s", this->BalloonHelpPending);
    this->SetBalloonHelpPending(NULL);
    }
  this->Script("wm withdraw %s",this->BalloonHelpWindow->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkKWApplication::BalloonHelpWithdraw()
{
  this->Script("wm withdraw %s",this->BalloonHelpWindow->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWApplication::SetWidgetVisibility(int v)
{
  vtkKWApplication::WidgetVisibility = v;
}

//----------------------------------------------------------------------------
int vtkKWApplication::GetWidgetVisibility() 
{
  return vtkKWApplication::WidgetVisibility;
}

void vtkKWApplication::DisplayAbout(vtkKWWindow *win)
{
  ostrstream str;
  str << "Application : " << this->GetApplicationName() << "\nVersion : " << this->GetApplicationVersionName() << "\nRelease : " << this->GetApplicationReleaseName() << ends;

  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->Create(this,"");
  dlg->SetText(str.str());
  dlg->Invoke();  
  dlg->Delete();  
}
