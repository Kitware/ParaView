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
#include "vtkKWPushButton.h"

#include "vtkInteractorStylePlaneSource.h"
#include "vtkInteractorStyleTrackballCamera.h"

#include "vtkMath.h"
#include "vtkSynchronizedTemplates3D.h"

#include "vtkPVAssignment.h"
#include "vtkPVSource.h"
#include "vtkPVConeSource.h"
#include "vtkPVPolyData.h"
#include "vtkPVImageReader.h"
#include "vtkPVImage.h"
#include "vtkPVSourceList.h"



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
  this->CreateMenu = vtkKWMenu::New();
  this->Toolbar = vtkKWToolbar::New();
  this->ResetCameraButton = vtkKWPushButton::New();
  this->PreviousSourceButton = vtkKWPushButton::New();
  this->NextSourceButton = vtkKWPushButton::New();
  this->Sources = vtkKWCompositeCollection::New();
  
  this->ApplicationAreaFrame = vtkKWLabeledFrame::New();
  this->SourceList = vtkPVSourceList::New();
  this->SourceList->SetSources(this->Sources);
  this->SourceList->SetParent(this->ApplicationAreaFrame->GetFrame());
}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  this->Toolbar->Delete();
  this->Toolbar = NULL;
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;
  this->PreviousSourceButton->Delete();
  this->PreviousSourceButton = NULL;
  this->NextSourceButton->Delete();
  this->NextSourceButton = NULL;
  
  this->SourceList->Delete();
  this->SourceList = NULL;
  
  this->ApplicationAreaFrame->Delete();
}

//----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, char *args)
{
  // invoke super method first
  this->vtkKWWindow::Create(app,"");

  this->Script("wm geometry %s 900x700+0+0",
                      this->GetWidgetName());
  
  // now add property options
  char *rbv = 
    this->GetMenuProperties()->CreateRadioButtonVariable(
      this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(1," ParaView Window", 
                                            rbv, this, "ShowWindowProperties");
  delete [] rbv;

  // create the top level
  this->MenuFile->InsertCommand(0,"New Window", this, "NewWindow");

  // Create the menu for creating data sources.  
  this->CreateMenu->SetParent(this->GetMenu());
  this->CreateMenu->Create(this->Application,"-tearoff 0");
  this->Menu->InsertCascade(2,"Create",this->CreateMenu,0);

  this->CreateMenu->AddCommand("Volume", this, "NewVolume");
  this->CreateMenu->AddCommand("Cone", this, "NewCone");

  this->SetStatusText("Version 1.0 beta");
  
  this->Script( "wm withdraw %s", this->GetWidgetName());

  this->Toolbar->SetParent(this->GetToolbarFrame());
  this->Toolbar->Create(app); 

  this->ResetCameraButton->SetParent(this->Toolbar);
  this->ResetCameraButton->Create(app, "-text ResetCamera");
  this->ResetCameraButton->SetCommand(this, "ResetCameraCallback");
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
               this->ResetCameraButton->GetWidgetName());
  
  this->PreviousSourceButton->SetParent(this->Toolbar);
  this->PreviousSourceButton->Create(app, "-text Previous");
  this->PreviousSourceButton->SetCommand(this, "PreviousSource");
  this->NextSourceButton->SetParent(this->Toolbar);
  this->NextSourceButton->Create(app, "-text Next");
  this->NextSourceButton->SetCommand(this, "NextSource");
  this->Script("pack %s %s -side left -pady 0 -fill none -expand no",
	       this->PreviousSourceButton->GetWidgetName(),
	       this->NextSourceButton->GetWidgetName());
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
               this->Toolbar->GetWidgetName());
  
  this->CreateDefaultPropertiesParent();
  
  this->Notebook->SetParent(this->GetPropertiesParent());
  this->Notebook->Create(this->Application,"");
  this->Notebook->AddPage("Preferences");

  this->ApplicationAreaFrame->
    SetParent(this->Notebook->GetFrame("Preferences"));
  this->ApplicationAreaFrame->Create(this->Application);
  this->ApplicationAreaFrame->SetLabel("Sources");
  this->SourceList->Create(app, "");
  this->Script("pack %s -side top -fill x -expand no",
	       this->SourceList->GetWidgetName());

  this->Script("pack %s -side top -anchor w -expand yes -fill x -padx 2 -pady 2",
               this->ApplicationAreaFrame->GetWidgetName());

  // create the main view
  // we keep a handle to them as well
  this->MainView = vtkPVRenderView::New();
  this->MainView->Clone((vtkPVApplication*)app);
  this->MainView->SetParent(this->ViewFrame);
  this->MainView->Create(this->Application,"-width 200 -height 200");
  this->AddView(this->MainView);
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->Delete();
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());  

  this->Script( "wm deiconify %s", this->GetWidgetName());
  
  
  // Setup an interactor style.
  vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
  this->MainView->SetInteractorStyle(style);
}

void vtkPVWindow::SetCurrentSource(vtkPVSource *comp)
{
  this->MainView->SetSelectedComposite(comp);  
  if (comp && this->Sources->IsItemPresent(comp) == 0)
    {
    this->Sources->AddItem(comp);
    this->SourceList->Update();
    }
}

//----------------------------------------------------------------------------
// Setup a cone
// The first example that is not used anymore.
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
  vtkInteractorStyle *style = vtkInteractorStyleTrackballCamera::New();

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
void vtkPVWindow::NewCone()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVConeSource *cone;
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  
  // Create the pipeline objects in all processes.
  cone = vtkPVConeSource::New();
  cone->Clone(pvApp);
  pvd = vtkPVPolyData::New();
  pvd->Clone(pvApp);
  a = vtkPVAssignment::New();
  a->Clone(pvApp);
  
  // Link the objects together (in all processes).
  cone->SetOutput(pvd);
  cone->SetName("cone");
  cone->SetAssignment(a);
  
  // Add the new Source to the View (in all processes).
  this->MainView->AddComposite(cone);

  // Select this Source
  this->SetCurrentSource(cone);

  // Clean up. (How about on the other processes?)
  this->SourceList->Update();
  cone->Delete();
  cone = NULL;
  a->Delete();
  a = NULL;
  
  this->MainView->ResetCamera();
  this->MainView->Render();
}

//----------------------------------------------------------------------------
// Setup the pipeline
void vtkPVWindow::NewVolume()
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  vtkPVImageReader *reader;
  vtkPVImage *image;
  vtkPVAssignment *a;
  
  reader = vtkPVImageReader::New();
  reader->Clone(pvApp);
  image = vtkPVImage::New();
  image->Clone(pvApp);
  a = vtkPVAssignment::New();
  a->Clone(pvApp);
  
  reader->GetImageReader()->UpdateInformation();
  a->BroadcastWholeExtent(reader->GetImageReader()->GetOutput()->GetWholeExtent());
  
  // Does not actually read.  Just sets the file name ...
  reader->ReadImage();
  
  reader->SetOutput(image);
  reader->SetAssignment(a);
  
  reader->SetName("volume");
  this->MainView->AddComposite(reader);
  this->SetCurrentSource(reader);
  this->SourceList->Update();
  
  this->MainView->ResetCamera();
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

//----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetCurrentSource()
{
  return vtkPVSource::SafeDownCast(this->GetMainView()->GetSelectedComposite());
}

//----------------------------------------------------------------------------
void vtkPVWindow::NextSource()
{
  vtkPVSource *composite = this->GetNextSource();
  if (composite != NULL)
    {
    this->GetCurrentSource()->GetProp()->VisibilityOff();
    this->SetCurrentSource(composite);
    this->GetCurrentSource()->GetProp()->VisibilityOn();
    }
  
  this->MainView->Render();
  this->SourceList->Update();
}

//----------------------------------------------------------------------------
void vtkPVWindow::PreviousSource()
{
  vtkPVSource *composite = this->GetPreviousSource();
  if (composite != NULL)
    {
    this->GetCurrentSource()->GetProp()->VisibilityOff();
    this->SetCurrentSource(composite);
    this->GetCurrentSource()->GetProp()->VisibilityOn();
    }
  
  this->MainView->Render();
  this->SourceList->Update();
}


//----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetNextSource()
{
  int pos = this->Sources->IsItemPresent(this->GetCurrentSource());
  return vtkPVSource::SafeDownCast(this->Sources->GetItemAsObject(pos));
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetPreviousSource()
{
  int pos = this->Sources->IsItemPresent(this->GetCurrentSource());
  return vtkPVSource::SafeDownCast(this->Sources->GetItemAsObject(pos-2));
}

//----------------------------------------------------------------------------
vtkKWCompositeCollection* vtkPVWindow::GetSources()
{
  return this->Sources;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ResetCameraCallback()
{
  
  this->MainView->ResetCamera();
  this->MainView->Render();
}



void vtkPVWindow::ShowWindowProperties()
{
  this->ShowProperties();
  
  // make sure the variable is set, otherwise set it
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(),"Radio",1);

  // forget current props
  this->Script("pack forget [pack slaves %s]",
               this->Notebook->GetParent()->GetWidgetName());  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}
