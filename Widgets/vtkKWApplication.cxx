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

int vtkKWApplicationCommand(ClientData cd, Tcl_Interp *interp,
			    int argc, char *argv[]);

vtkKWApplication::vtkKWApplication()
{
  this->ApplicationName = new char[strlen("Kitware")+1];
  strcpy(this->ApplicationName, "Kitware" );
  
  // setup tcl stuff
  //this->MainInterp = Tcl_CreateInterp();
  this->MainInterp = vtkTclGetGlobalInterp();
  
  /// Delete the 'exit' command, which can screw things up 
  //Tcl_DeleteCommand(this->MainInterp, "exit");
    
  //this->MainWindow = Tk_MainWindow(this->MainInterp);

  // call the compiled init funciton
  //Et_DoInit(this->MainInterp);
  
  //if (Vtktcl_Init(this->MainInterp) == TCL_ERROR) 
  //  {
  //  vtkErrorMacro("Could not initialize vtk");
  //  }

  // remove . so that people will not be tempted to use it
  //Tcl_GlobalEval(this->MainInterp, "wm withdraw .");
  this->Windows = vtkKWWindowCollection::New();  
  
  // add the application as $app
  vtkTclGetObjectFromPointer(this->MainInterp, (void *)this, 
                             vtkKWApplicationCommand);

  this->Script("set Application %s",this->MainInterp->result);
}

vtkKWApplication::~vtkKWApplication()
{
  this->SetApplicationName(NULL);
}

void vtkKWApplication::Script(char *format, ...)
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
  if (Tcl_GlobalEval(this->MainInterp, event) != TCL_OK)
    {
    vtkGenericWarningMacro("Error returned from tcl script.\n" <<
			   this->MainInterp->result << endl);
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
  this->Windows->RemoveAllItems();
  this->Windows->Delete();
  this->Windows = NULL;  
  this->Script("exit");
  //Tcl_GlobalEval(this->MainInterp, "destroy .");  
  //Tcl_DeleteInterp(this->MainInterp);
  this->MainInterp = NULL;
}
    
void vtkKWApplication::Start()
{ 
  this->Start(0,NULL);
}
void vtkKWApplication::Start(char *arg)
{ 
  this->Start(1,&arg);
}
void vtkKWApplication::Start(int argc, char *argv[])
{ 
//  Tk_MainLoop();
}

void vtkKWApplication::DisplayAbout(vtkKWWindow *win)
{
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
  sprintf(fkey,"Software\\Kitware\\%i\\Inst",0);  
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
#endif
}

