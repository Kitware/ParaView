/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkPVWindow.h"
#include "vtkOutlineFilter.h"
#include "vtkObjectFactory.h"
#include "vtkKWDialog.h"
#include "vtkPVApplication.h"

#include "vtkInteractorStylePlaneSource.h"


//----------------------------------------------------------------------------
vtkPVWindow* vtkPVWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVWindow");
  if(ret)
    {
    return (vtkPVWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVWindow;
}

int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVWindow::vtkPVWindow()
{  
  this->CommandFunction = vtkPVWindowCommand;
  this->RetrieveMenu = vtkKWWidget::New();
  this->CreateMenu = vtkKWWidget::New();
}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
}

//----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, char *args)
{
  // invoke super method first
  this->vtkKWWindow::Create(app,"");

  // Test the slave process.
  vtkPVApplication *pvApp = (vtkPVApplication *)(app);
  char result[255];
  pvApp->RemoteScript(0, "vtkConeSource cone", NULL, 0);
  pvApp->RemoteScript(0, "cone Update", NULL, 0);
  pvApp->RemoteScript(0, "[cone GetOutput] GetNumberOfPoints", result, 255);
  cerr << "The slave process gave this result: " << result << endl;
  
  this->Script("wm geometry %s 900x700+0+0",
                      this->GetWidgetName());
  
  Tcl_Interp *interp = this->Application->GetMainInterp();

  // create the top level
  this->Script(
    "%s insert 0 command -label {New Window} -command {%s NewWindow}",
    this->GetMenuFile()->GetWidgetName(), this->GetTclName());
  
  this->Script(
    "%s insert 1 command -label {Open} -command {%s Open}",
    this->GetMenuFile()->GetWidgetName(), this->GetTclName());
  this->Script(
    "%s insert 2 command -label {Save} -command {%s Save}",
    this->GetMenuFile()->GetWidgetName(), this->GetTclName());
  
  this->CreateMenu->SetParent(this->GetMenu());
  this->CreateMenu->Create(this->Application,"menu","-tearoff 0");
  this->Script("%s insert 2 cascade -label {Create} -menu %s -underline 0",
               this->Menu->GetWidgetName(),this->CreateMenu->GetWidgetName());

  this->RetrieveMenu->SetParent(this->GetMenu());
  this->RetrieveMenu->Create(this->Application,"menu","-tearoff 0");
  this->Script(
    "%s insert 3 cascade -label {Retrieve} -menu %s -underline 0",
    this->GetMenu()->GetWidgetName(),this->RetrieveMenu->GetWidgetName());

  this->SetStatusText("Version 1.0 beta");
  this->CreateDefaultPropertiesParent();
  
  this->Script( "wm withdraw %s", this->GetWidgetName());

  // create the main view
  // we keep a handle to them as well
  this->MainView = vtkPVRenderView::New();
  this->MainView->SetParent(this->ViewFrame);
  // Why do I have to explicitly state the class?  I do not know.
  this->MainView->vtkPVRenderView::Create(this->Application,"-width 200 -height 200");
  this->AddView(this->MainView);
  // force creation of the properties parent
  this->MainView->GetPropertiesParent();
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->Delete();
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());

  char cmd[1024];
  sprintf(cmd,"%s Open", this->GetTclName());
  
  this->Script( "wm deiconify %s", this->GetWidgetName());

  // Setup an interactor style.
  this->SetupTest();

}


//----------------------------------------------------------------------------
void vtkPVWindow::SetupTest()
{
  vtkInteractorStylePlaneSource *planeStyle = 
                      vtkInteractorStylePlaneSource::New();
  this->MainView->SetInteractorStyle(planeStyle);

  vtkActor *actor = planeStyle->GetPlaneActor();

  this->MainView->GetRenderer()->AddActor(actor);
}


//----------------------------------------------------------------------------
void vtkPVWindow::NewWindow()
{
  vtkPVWindow *nw = vtkPVWindow::New();
  nw->Create(this->Application,"");
  this->Application->AddWindow(nw);  
  nw->Delete();
}



//----------------------------------------------------------------------------
void vtkPVWindow::Save() 
{
  //char *path;
  //
}

//----------------------------------------------------------------------------
// Description:
// Chaining method to serialize an object and its superclasses.
void vtkPVWindow::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->vtkKWWindow::SerializeSelf(os,indent);

  os << indent << "MainView ";
  this->MainView->Serialize(os,indent);
}


//----------------------------------------------------------------------------
void vtkPVWindow::SerializeToken(istream& is, const char token[1024])
{
  if (!strcmp(token,"MainView"))
    {
    this->MainView->Serialize(is);
    return;
    }

  vtkKWWindow::SerializeToken(is,token);
}



