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
#include "vtkPVApplication.h"
#include "vtkKWToolbar.h"
#include "vtkPVWindow.h"
#include "vtkOutlineFilter.h"
#include "vtkObjectFactory.h"
#include "vtkKWDialog.h"
#include "vtkKWNotebook.h"

#include "vtkInteractorStylePlaneSource.h"
#include "vtkInteractorStyleCamera.h"

#include "vtkMath.h"
#include "vtkImageReader.h"
#include "vtkOutlineSource.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkKWScale.h"


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
  this->Toolbar = vtkKWToolbar::New();
  this->ResetCameraButton = vtkKWWidget::New();
  this->MainNotebook = vtkKWNotebook::New();
  this->MainNotebookCreated = 0;
  this->IsoScale = vtkKWScale::New();
  this->XPlaneScale = vtkKWScale::New();
  this->ZPlaneScale = vtkKWScale::New();
}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  this->Toolbar->Delete();
  this->Toolbar = NULL;
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;
  
  this->MainNotebook->SetParent(NULL);
  this->MainNotebook->Delete();
  this->MainNotebook = NULL;

  this->IsoScale->SetParent(NULL);
  this->IsoScale->Delete();
  this->IsoScale = NULL;

  this->XPlaneScale->SetParent(NULL);
  this->XPlaneScale->Delete();
  this->XPlaneScale = NULL;

  this->ZPlaneScale->SetParent(NULL);
  this->ZPlaneScale->Delete();
  this->ZPlaneScale = NULL;
}

//----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, char *args)
{
  // invoke super method first
  this->vtkKWWindow::Create(app,"");

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
  
  this->Toolbar->SetParent(this->GetToolbarFrame());
  this->Toolbar->Create(app); 
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
               this->Toolbar->GetWidgetName());
  
  this->ResetCameraButton->SetParent(this->Toolbar);
  this->ResetCameraButton->Create(app, "button", "-text ResetCamera");
  this->ResetCameraButton->SetCommand(this->MainView, "ResetCamera");
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
               this->ResetCameraButton->GetWidgetName());
    
  this->Script( "wm deiconify %s", this->GetWidgetName());

  // Setup an interactor style.
  //this->SetupCone();
  this->SetupVolumeIso();

}

//----------------------------------------------------------------------------
void vtkPVWindow::ShowMainNotebook()
{
  // make sure we have an applicaiton
  if (!this->Application)
    {
    vtkErrorMacro("attempt to update properties without an application set");
    }
  
  // make sure the variable is set, otherwise set it
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(),"Radio",11);
  
  // unpack any current children
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->GetPropertiesParent()->GetWidgetName());

  // do we need to create the Notebook ?
  if ( ! this->MainNotebookCreated)
    {
    this->CreateMainNotebook();
    }

  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->MainNotebook->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVWindow::CreateMainNotebook()
{
  vtkKWApplication *app = this->Application;

  if (this->MainNotebookCreated)
    {  
    return;
    }
  this->MainNotebookCreated = 1;
  
  this->MainNotebook->SetParent(this->GetPropertiesParent());
  this->MainNotebook->Create(this->Application,"");
  this->MainNotebook->AddPage("Vol");
  this->MainNotebook->AddPage("Iso");
  this->MainNotebook->AddPage("Cut");
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->MainNotebook->GetWidgetName());
  this->MainNotebook->Raise("Iso");  
  
  this->IsoScale->SetParent(this->MainNotebook->GetFrame("Iso"));
  this->IsoScale->Create(this->Application, "");
  this->IsoScale->SetRange(0, 2000);
  this->IsoScale->SetValue(1120);
  this->Script("pack %s -pady 2 -padx 2 -fill x -expand yes",
               this->IsoScale->GetWidgetName());
  this->IsoScale->SetCommand(this, "IsoValueChanged");
  
  this->XPlaneScale->SetParent(this->MainNotebook->GetFrame("Cut"));
  this->XPlaneScale->Create(this->Application, "");
  this->XPlaneScale->SetRange(0, 64);
  this->XPlaneScale->SetValue(20);
  this->Script("pack %s -pady 2 -padx 2 -fill x -expand yes",
               this->XPlaneScale->GetWidgetName());
  this->XPlaneScale->SetCommand(this, "XPlaneChanged");
  
  this->ZPlaneScale->SetParent(this->MainNotebook->GetFrame("Cut"));
  this->ZPlaneScale->Create(this->Application, "");
  this->ZPlaneScale->SetRange(0, 64);
  this->ZPlaneScale->SetValue(20);
  this->Script("pack %s -pady 2 -padx 2 -fill x -expand yes",
               this->ZPlaneScale->GetWidgetName());
  this->ZPlaneScale->SetCommand(this, "ZPlaneChanged");
  
}



//----------------------------------------------------------------------------
// Setup a cone
void vtkPVWindow::SetupCone()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  int id, num;
  
  // Lets show a cone as a test.
  num = pvApp->GetController()->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteSimpleScript(id, "vtkConeSource ConeSource");
    pvApp->RemoteSimpleScript(id, "vtkPolyDataMapper ConeMapper");
    pvApp->RemoteSimpleScript(id, "ConeMapper SetInput [ConeSource GetOutput]");
    pvApp->RemoteSimpleScript(id, "vtkActor ConeActor");
    pvApp->RemoteSimpleScript(id, "ConeActor SetMapper ConeMapper");
    pvApp->RemoteSimpleScript(id, "[RenderSlave GetRenderer] AddActor ConeActor");
    // Shift to discount UI process
    pvApp->RemoteScript(id, "[ConeSource GetOutput] SetUpdateExtent %d %d", id-1, num-1);
    }
  
  // Setup an interactor style
  vtkInteractorStylePlaneSource *planeStyle = 
                      vtkInteractorStylePlaneSource::New();
  vtkInteractorStyle *style = vtkInteractorStyleCamera::New();

  //this->MainView->SetInteractorStyle(planeStyle);
  this->MainView->SetInteractorStyle(style);
  
  vtkActor *actor = planeStyle->GetPlaneActor();
  actor->GetProperty()->SetColor(0.0, 0.8, 0.0);
  
  //this->MainView->GetRenderer()->AddActor(actor);
  
  planeStyle->Delete();
  style->Delete();
  
  this->MainView->ResetCamera();
}


//----------------------------------------------------------------------------
// Setup the pipeline
void vtkPVWindow::SetupVolumeIso()
{
  int id, num;
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;

  // Put the outline in the UI window.
  
  // We have two options for displaying this in the future:
  // 1: Create an image outline filter that only updates information.
  // 2: Create a distributed outline filter that uses piece hints.
  vtkOutlineSource *outline = vtkOutlineSource::New();
  outline->SetBounds(0, 255, 0, 255, 0, 92 * 1.8);
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(outline->GetOutput());
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  this->MainView->GetRenderer()->AddActor(actor);
  actor->Delete();
  mapper->Delete();
  outline->Delete();
  
  
  
  // Setup the worker pipelines
  num = pvApp->GetController()->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteSimpleScript(id, "vtkImageReader ImageReader");

    //pvApp->RemoteSimpleScript(id, "ImageReader SetFilePrefix {/home/lawcc/vtkdata/fullHead/headsq}");
    //pvApp->RemoteSimpleScript(id, "ImageReader SetDataSpacing 1 1 1.8");
    //pvApp->RemoteSimpleScript(id, "ImageReader SetDataExtent 0 255 0 255 1 93");

    pvApp->RemoteSimpleScript(id, "ImageReader SetFilePrefix {/home/lawcc/vtkdata/headsq/quarter}");
    pvApp->RemoteSimpleScript(id, "ImageReader SetDataSpacing 4 4 1.8");
    pvApp->RemoteSimpleScript(id, "ImageReader SetDataExtent 0 63 0 63 1 93");

    pvApp->RemoteSimpleScript(id, "ImageReader SetDataByteOrderToLittleEndian");
    pvApp->RemoteSimpleScript(id, "vtkSynchronizedTemplates3D Iso");
    pvApp->RemoteSimpleScript(id, "Iso SetInput [ImageReader GetOutput]");
    pvApp->RemoteSimpleScript(id, "Iso SetValue 0 1150");
    pvApp->RemoteScript(id, "[Iso GetOutput] SetUpdateExtent %d %d", id-1, num-1);
    
    pvApp->RemoteSimpleScript(id, "vtkPolyDataMapper IsoMapper");
    pvApp->RemoteSimpleScript(id, "IsoMapper SetInput [Iso GetOutput]");
    pvApp->RemoteSimpleScript(id, "IsoMapper ScalarVisibilityOff");
    pvApp->RemoteSimpleScript(id, "vtkActor IsoActor");
    pvApp->RemoteScript(id, "[IsoActor GetProperty] SetColor %f %f %f",
			vtkMath::Random(), vtkMath::Random(), vtkMath::Random());
    
    pvApp->RemoteSimpleScript(id, "IsoActor SetMapper IsoMapper");
    pvApp->RemoteSimpleScript(id, "[RenderSlave GetRenderer] AddActor IsoActor");
    
    // Z Plane geomtery
    pvApp->RemoteSimpleScript(id, "vtkPVImagePlaneComponent ImagePlaneZ");
    pvApp->RemoteSimpleScript(id, "ImagePlaneZ SetInput [ImageReader GetOutput]");
    pvApp->RemoteScript(id, "ImagePlaneZ SetPiece %d %d", id-1, num-1);
    pvApp->RemoteSimpleScript(id, "ImagePlaneZ SetPlaneExtent 0 63 0 63 20 20");
    pvApp->RemoteSimpleScript(id, "[RenderSlave GetRenderer] AddActor [ImagePlaneZ GetActor]");

    // X Plane geomtery
    pvApp->RemoteSimpleScript(id, "vtkPVImagePlaneComponent ImagePlaneX");
    pvApp->RemoteSimpleScript(id, "ImagePlaneX SetInput [ImageReader GetOutput]");
    pvApp->RemoteScript(id, "ImagePlaneX SetPiece %d %d", id-1, num-1);
    pvApp->RemoteSimpleScript(id, "ImagePlaneX SetPlaneExtent 20 20 0 63 0 92");
    pvApp->RemoteSimpleScript(id, "[RenderSlave GetRenderer] AddActor [ImagePlaneX GetActor]");
    }
  
  // Setup an interactor style
  vtkInteractorStylePlaneSource *planeStyle = 
                      vtkInteractorStylePlaneSource::New();
  vtkInteractorStyle *style = vtkInteractorStyleCamera::New();

  //this->MainView->SetInteractorStyle(planeStyle);
  this->MainView->SetInteractorStyle(style);
  style->Delete();
  
  // now add property options
  char *rbv = 
    this->GetMenuProperties()->CreateRadioButtonVariable(
      this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(11,"Main", rbv, this, "ShowMainNotebook");
  delete [] rbv;
  
  this->ShowMainNotebook();
  
  this->MainView->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVWindow::IsoValueChanged()
{
  int id, num;
  float val;
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  
  val = this->IsoScale->GetValue();
  
  // Tell each of the worker pipelines the new value.
  num = pvApp->GetController()->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteScript(id, "Iso SetValue 0 %f", val);
    }
    
  this->MainView->Render();
}


//----------------------------------------------------------------------------
void vtkPVWindow::XPlaneChanged()
{
  int id, num;
  float val;
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  
  val = this->XPlaneScale->GetValue();
  
  // Tell each of the worker pipelines the new value.
  num = pvApp->GetController()->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteScript(id, "ImagePlaneX SetPosition %d", (int)val);
    }
    
  this->MainView->Render();
}


//----------------------------------------------------------------------------
void vtkPVWindow::ZPlaneChanged()
{
  int id, num;
  float val;
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  
  val = this->ZPlaneScale->GetValue();
  
  // Tell each of the worker pipelines the new value.
  num = pvApp->GetController()->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteScript(id, "ImagePlaneZ SetPosition %d", (int)val);
    }
    
  this->MainView->Render();
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






