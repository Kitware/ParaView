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

#include "vtkMath.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkSuperquadricSource.h"
#include "vtkImageMandelbrotSource.h"

#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVSourceList.h"
#include "vtkPVActorComposite.h"

#include "vtkPVSourceInterface.h"
#include "vtkPVEnSightReaderInterface.h"
#include "vtkPVDataSetReaderInterface.h"
#include "vtkPVArrayCalculator.h"
#include "vtkPVThreshold.h"
#include "vtkPVContour.h"
#include "vtkPVGlyph3D.h"

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
  this->FilterMenu = vtkKWMenu::New();
  this->Toolbar = vtkKWToolbar::New();
  this->ResetCameraButton = vtkKWPushButton::New();
  this->SourceListButton = vtkKWPushButton::New();
  this->CameraStyleButton = vtkKWPushButton::New();
  this->CurrentSourceButton = vtkKWPushButton::New();
  this->CurrentActorButton = vtkKWPushButton::New();

  this->CalculatorButton = vtkKWPushButton::New();
  this->ThresholdButton = vtkKWPushButton::New();
  this->ContourButton = vtkKWPushButton::New();
  this->GlyphButton = vtkKWPushButton::New();
  
  this->Sources = vtkKWCompositeCollection::New();
  
  this->ApplicationAreaFrame = vtkKWLabeledFrame::New();
  this->SourceList = vtkPVSourceList::New();
  this->SourceList->SetSources(this->Sources);
  this->SourceList->SetParent(this->ApplicationAreaFrame->GetFrame());
  
  this->CameraStyle = vtkInteractorStyleTrackballCamera::New();
  
  this->SourceInterfaces = vtkCollection::New();
  this->CurrentPVData = NULL;
}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  this->Toolbar->Delete();
  this->Toolbar = NULL;
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;
  this->SourceListButton->Delete();
  this->SourceListButton = NULL;
  this->CameraStyleButton->Delete();
  this->CameraStyleButton = NULL;
  this->CurrentSourceButton->Delete();
  this->CurrentSourceButton = NULL;
  this->CurrentActorButton->Delete();
  this->CurrentActorButton = NULL;
  
  this->CalculatorButton->Delete();
  this->CalculatorButton = NULL;
  
  this->ThresholdButton->Delete();
  this->ThresholdButton = NULL;
  
  this->ContourButton->Delete();
  this->ContourButton = NULL;

  this->GlyphButton->Delete();
  this->GlyphButton = NULL;
  
  this->SourceList->Delete();
  this->SourceList = NULL;
  
  this->ApplicationAreaFrame->Delete();
  
  this->CameraStyle->Delete();
  this->CameraStyle = NULL;
  
  this->MainView->Delete();
  this->MainView = NULL;
  
  this->SourceInterfaces->Delete();
  this->SourceInterfaces = NULL;
  
  this->CreateMenu->Delete();
  this->CreateMenu = NULL;
  
  this->FilterMenu->Delete();
  this->FilterMenu = NULL;  
  
  this->SetCurrentPVData(NULL);
}


//----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, char *args)
{
  vtkPVSourceInterface *sInt;
  
  // invoke super method first
  this->vtkKWWindow::Create(app,"");

  // We need an application before we can read the interface.
  this->ReadSourceInterfaces();
  
  this->Script("wm geometry %s 900x700+0+0",
                      this->GetWidgetName());
  
  // now add property options
  char *rbv = 
    this->GetMenuProperties()->CreateRadioButtonVariable(
      this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(1, "Source List", 
                                            rbv, this, "ShowWindowProperties");
  this->GetMenuProperties()->AddRadioButton(2, "Source",
                                            rbv, this,
                                            "ShowCurrentSourceProperties");
  this->GetMenuProperties()->AddRadioButton(3, "Actor", rbv, this,
                                            "ShowCurrentActorProperties");
  
  delete [] rbv;

  // create the top level
  this->MenuFile->InsertCommand(0,"New Window", this, "NewWindow");
  this->MenuFile->InsertCommand(2, "Save Camera", this, "Save");
  this->MenuFile->InsertCommand(2, "Save", this, "SavePipeline");
  
  // Create the menu for creating data sources.  
  this->CreateMenu->SetParent(this->GetMenu());
  this->CreateMenu->Create(this->Application,"-tearoff 0");
  this->Menu->InsertCascade(2,"Create",this->CreateMenu,0);  
  
  // Create the menu for creating data sources.  
  this->FilterMenu->SetParent(this->GetMenu());
  this->FilterMenu->Create(this->Application,"-tearoff 0");
  this->Menu->InsertCascade(3,"Filter",this->FilterMenu,0);  
  
  // Create all of the menu items for sources with no inputs.
  this->SourceInterfaces->InitTraversal();
  while ( (sInt = (vtkPVSourceInterface*)(this->SourceInterfaces->GetNextItemAsObject())))
    {
    if (sInt->GetInputClassName() == NULL)
      {
      // Remove "vtk" from the class name to get the menu item name.
      this->CreateMenu->AddCommand(sInt->GetSourceClassName()+3, sInt, "CreateCallback");
      }
    }
  
  this->SetStatusText("Version 1.0 beta");
  
  this->Script( "wm withdraw %s", this->GetWidgetName());

  this->Toolbar->SetParent(this->GetToolbarFrame());
  this->Toolbar->Create(app); 

  this->ResetCameraButton->SetParent(this->Toolbar);
  this->ResetCameraButton->Create(app, "-text ResetCamera");
  this->ResetCameraButton->SetCommand(this, "ResetCameraCallback");
  
  this->SourceListButton->SetParent(this->Toolbar);
  this->SourceListButton->Create(app, "-text SourceList");
  this->SourceListButton->SetCommand(this, "ShowWindowProperties");

  this->CurrentSourceButton->SetParent(this->Toolbar);
  this->CurrentSourceButton->Create(app, "-text Source");
  this->CurrentSourceButton->SetCommand(this, "ShowCurrentSourceProperties");

  this->CurrentActorButton->SetParent(this->Toolbar);
  this->CurrentActorButton->Create(app, "-text Actor");
  this->CurrentActorButton->SetCommand(this, "ShowCurrentActorProperties");
  
  this->CalculatorButton->SetParent(this->Toolbar);
  this->CalculatorButton->Create(app, "-text Calculator");
  this->CalculatorButton->SetCommand(this, "CalculatorCallback");
  
  this->ThresholdButton->SetParent(this->Toolbar);
  this->ThresholdButton->Create(app, "-text Threshold");
  this->ThresholdButton->SetCommand(this, "ThresholdCallback");
  
  this->ContourButton->SetParent(this->Toolbar);
  this->ContourButton->Create(app, "-text Contour");
  this->ContourButton->SetCommand(this, "ContourCallback");

  this->GlyphButton->SetParent(this->Toolbar);
  this->GlyphButton->Create(app, "-text Glyph");
  this->GlyphButton->SetCommand(this, "GlyphCallback");

  this->Script("pack %s %s %s %s %s %s %s %s -side left -pady 0 -fill none -expand no",
               this->ResetCameraButton->GetWidgetName(),
               this->SourceListButton->GetWidgetName(),
               this->CurrentSourceButton->GetWidgetName(),
               this->CurrentActorButton->GetWidgetName(),
               this->CalculatorButton->GetWidgetName(),
               this->ThresholdButton->GetWidgetName(),
               this->ContourButton->GetWidgetName(),
               this->GlyphButton->GetWidgetName());

  // This button doesn't do anything useful right now.  It was put in originally
  // so we could switch between interactor styles.
//  this->CameraStyleButton->SetParent(this->Toolbar);
//  this->CameraStyleButton->Create(app, "");
//  this->CameraStyleButton->SetLabel("Camera");
//  this->CameraStyleButton->SetCommand(this, "UseCameraStyle");
//  this->Script("pack %s %s -side left -pady 0 -fill none -expand no",
//	       this->SourceListButton->GetWidgetName(),
//	       this->CameraStyleButton->GetWidgetName());
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
  this->CreateMainView((vtkPVApplication*)app);
  
  this->Script( "wm deiconify %s", this->GetWidgetName());
  
  // Setup an interactor style.
  //  vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
  //  this->MainView->SetInteractorStyle(style);
  this->MainView->SetInteractorStyle(this->CameraStyle);
}



//----------------------------------------------------------------------------
void vtkPVWindow::CreateMainView(vtkPVApplication *pvApp)
{
  vtkPVRenderView *view;
  
  view = vtkPVRenderView::New();
  view->CreateRenderObjects(pvApp);
  
  this->MainView = view;
  this->MainView->SetParent(this->ViewFrame);
  this->MainView->Create(this->Application,"-width 200 -height 200");
  this->AddView(this->MainView);
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());  
}



//----------------------------------------------------------------------------
void vtkPVWindow::UseCameraStyle()
{
  this->GetMainView()->SetInteractorStyle(this->CameraStyle);
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
  // Save out the camera parameters.  These are the ones used for the run-time
  // simulation.  We may need to add other parameters later.
  FILE *cameraFile;
  vtkCamera *camera = this->GetMainView()->GetRenderer()->GetActiveCamera();
  float position[3];
  float focalPoint[3];
  float viewUp[3];
  float viewAngle;
  float clippingRange[2];
  
  if ((cameraFile = fopen("camera.pv", "w")) == NULL)
    {
    vtkErrorMacro("Couldn't open file: camera.pv");
    return;
    }

  camera->GetPosition(position);
  camera->GetFocalPoint(focalPoint);
  camera->GetViewUp(viewUp);
  viewAngle = camera->GetViewAngle();
  camera->GetClippingRange(clippingRange);
  
  fprintf(cameraFile, "position %.6f %.6f %.6f\n", position[0], position[1],
	  position[2]);
  fprintf(cameraFile, "focal_point %.6f %.6f %.6f\n", focalPoint[0],
	  focalPoint[1], focalPoint[2]);
  fprintf(cameraFile, "view_up %.6f %.6f %.6f\n", viewUp[0], viewUp[1],
	  viewUp[2]);
  fprintf(cameraFile, "view_angle %.6f\n", viewAngle);
  fprintf(cameraFile, "clipping_range %.6f %.6f", clippingRange[0],
	  clippingRange[1]);

  fclose (cameraFile);
}

//----------------------------------------------------------------------------
void vtkPVWindow::SavePipeline()
{
  ofstream *file;
  vtkCollection *sources;
  vtkPVSource *pvs;

  file = new ofstream("pipeline.pv", ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file pipeline.pv");
    delete file;
    file = NULL;
    return;
    }

  *file << "<ParaView Version= 0.1>\n";
  // Loop through sources ...
  sources = this->SourceList->GetSources();
  sources->InitTraversal();
  while ( (pvs=(vtkPVSource*)(sources->GetNextItemAsObject())) )
    {
    pvs->Save(file);
    }
  *file << "</ParaView>\n";

  if (file)
    {
    file->close();
    delete file;
    file = NULL;
    }

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
void vtkPVWindow::SetCurrentPVData(vtkPVData *pvd)
{
  vtkPVSourceInterface *sInt;
  
  if (this->CurrentPVData)
    {
    this->CurrentPVData->UnRegister(this);
    this->CurrentPVData = NULL;
    // Remove all of the entries from the filter menu.
    this->FilterMenu->DeleteAllMenuItems();
    }
  if (pvd)
    {
    pvd->Register(this);
    this->CurrentPVData = pvd;
    // Add all the appropriate filters to the filter menu.
    this->SourceInterfaces->InitTraversal();
    while ( (sInt = (vtkPVSourceInterface*)(this->SourceInterfaces->GetNextItemAsObject())))
      {
      if (sInt->GetIsValidInput(pvd))
	{
	this->FilterMenu->AddCommand(sInt->GetSourceClassName()+3, sInt, "CreateCallback");
	}
      }
    }
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetPreviousPVSource()
{
  int pos = this->Sources->IsItemPresent(this->GetCurrentPVSource());
  return vtkPVSource::SafeDownCast(this->Sources->GetItemAsObject(pos-2));
}


//----------------------------------------------------------------------------
void vtkPVWindow::SetCurrentPVSource(vtkPVSource *comp)
{
  this->MainView->SetSelectedComposite(comp);  
  if (comp)
    {
    this->SetCurrentPVData(comp->GetNthPVOutput(0));
    }
  else
    {
    this->SetCurrentPVData(NULL);
    }
  
  if (comp && this->Sources->IsItemPresent(comp) == 0)
    {
    this->Sources->AddItem(comp);
    this->SourceList->Update();
    }
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVWindow::GetCurrentPVSource()
{
  return vtkPVSource::SafeDownCast(this->GetMainView()->GetSelectedComposite());
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

//----------------------------------------------------------------------------
void vtkPVWindow::CalculatorCallback()
{
  static int instanceCount = 0;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVArrayCalculator *calc;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  vtkPVData *current;
  const char* outputDataType;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot determine output type.");
    return;
    }
  outputDataType = current->GetVTKData()->GetClassName();
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Calculator", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkArrayCalculator", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return;
    }
  
  calc = vtkPVArrayCalculator::New();
  calc->SetPropertiesParent(this->GetMainView()->GetSourceParent());
  calc->SetApplication(pvApp);
  calc->SetVTKSource(s, tclName);
  calc->SetNthPVInput(0, this->GetCurrentPVData());
  calc->SetName(tclName);  

  this->GetMainView()->AddComposite(calc);
  calc->CreateProperties();
  calc->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(calc);
  this->GetMainView()->ShowSourceParent();

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "Calculator", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  calc->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", calc->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  calc->Delete();

  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ThresholdCallback()
{
  static int instanceCount = 0;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVThreshold *threshold;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  vtkPVData *current;
  const char* outputDataType;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot threshold; no input");
    return;
    }
  outputDataType = "vtkUnstructuredGrid";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Threshold", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkThreshold", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return;
    }
  
  threshold = vtkPVThreshold::New();
  threshold->SetPropertiesParent(this->GetMainView()->GetSourceParent());
  threshold->SetApplication(pvApp);
  threshold->SetVTKSource(s, tclName);
  threshold->SetNthPVInput(0, this->GetCurrentPVData());
  threshold->SetName(tclName);

  this->GetMainView()->AddComposite(threshold);
  threshold->CreateProperties();
  threshold->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(threshold);
  this->GetMainView()->ShowSourceParent();

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "Threshold", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  threshold->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", threshold->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  threshold->Delete();

  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ContourCallback()
{
  static int instanceCount = 0;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVContour *contour;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  vtkPVData *current;
  const char* outputDataType;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot determine output type");
    return;
    }
  outputDataType = "vtkPolyData";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Contour", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkKitwareContourFilter", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return;
    }
  
  contour = vtkPVContour::New();
  contour->SetPropertiesParent(this->GetMainView()->GetSourceParent());
  contour->SetApplication(pvApp);
  contour->SetVTKSource(s, tclName);
  contour->SetNthPVInput(0, this->GetCurrentPVData());
  contour->SetName(tclName);

  this->GetMainView()->AddComposite(contour);
  contour->CreateProperties();
  contour->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(contour);
  this->GetMainView()->ShowSourceParent();

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "Contour", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  contour->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", contour->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  contour->Delete();

  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::GlyphCallback()
{
  static int instanceCount = 0;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVGlyph3D *glyph;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  vtkPVData *current;
  const char* outputDataType;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot determine output type");
    return;
    }
  outputDataType = "vtkPolyData";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Glyph", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkGlyph3D", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return;
    }
  
  glyph = vtkPVGlyph3D::New();
  glyph->SetPropertiesParent(this->GetMainView()->GetSourceParent());
  glyph->SetApplication(pvApp);
  glyph->SetVTKSource(s, tclName);
  glyph->SetNthPVInput(0, this->GetCurrentPVData());
  glyph->SetName(tclName);

  this->GetMainView()->AddComposite(glyph);
  glyph->CreateProperties();
  glyph->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(glyph);
  this->GetMainView()->ShowSourceParent();

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "Glyph", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  glyph->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", glyph->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  glyph->Delete();

  ++instanceCount;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentSourceProperties()
{
  // We need to update the properties-menu radio button too!
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(), "Radio", 2);
  
  this->GetCurrentPVSource()->UpdateInputList();
  if (this->GetCurrentPVSource()->IsA("vtkPVGlyph3D"))
    {
    ((vtkPVGlyph3D*)this->GetCurrentPVSource())->UpdateSourceMenu();
    }
  this->MainView->ShowSourceParent();
}

//----------------------------------------------------------------------------
void vtkPVWindow::ShowCurrentActorProperties()
{
  // We need to update the properties-menu radio button too!
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(), "Radio", 3);
  
  this->MainView->ShowActorParent();
}

//----------------------------------------------------------------------------
vtkPVApplication *vtkPVWindow::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}


//----------------------------------------------------------------------------
// Lets experiment with an interface prototype.
// We will eventually read these from a file.
void vtkPVWindow::ReadSourceInterfaces()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVMethodInterface *mInt;
  vtkPVSourceInterface *sInt;

  
  
  
  // ============= Image Sources ==============  

  // ---- Fractal Source ----
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageMandelbrotSource");
  sInt->SetRootName("Fractal");
  sInt->SetOutputClassName("vtkImageData");
  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Extent");
  mInt->SetSetCommand("SetWholeExtent");
  mInt->SetGetCommand("GetWholeExtent");
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("SubSpace");
  mInt->SetSetCommand("SetProjectionAxes");
  mInt->SetGetCommand("GetProjectionAxes");
  mInt->AddIntegerArgument();  
  mInt->AddIntegerArgument();  
  mInt->AddIntegerArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Origin");
  mInt->SetSetCommand("SetOriginCX");
  mInt->SetGetCommand("GetOriginCX");
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Spacing");
  mInt->SetSetCommand("SetSampleCX");
  mInt->SetGetCommand("GetSampleCX");
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ============= PolyData Sources ==============  
  
  // ---- STL Reader ----
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSTLReader");
  sInt->SetRootName("STLReader");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FileName");
  mInt->SetSetCommand("SetFileName");
  mInt->SetGetCommand("GetFileName");
  mInt->SetWidgetTypeToFile();
  mInt->SetFileExtension("stl");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Sphere ----
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSphereSource");
  sInt->SetRootName("Sphere");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Center");
  mInt->SetSetCommand("SetCenter");
  mInt->SetGetCommand("GetCenter");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Theta Resolution");
  mInt->SetSetCommand("SetThetaResolution");
  mInt->SetGetCommand("GetThetaResolution");
  mInt->AddIntegerArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Start Theta");
  mInt->SetSetCommand("SetStartTheta");
  mInt->SetGetCommand("GetStartTheta");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("End Theta");
  mInt->SetSetCommand("SetEndTheta");
  mInt->SetGetCommand("GetEndTheta");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Phi Resolution");
  mInt->SetSetCommand("SetPhiResolution");
  mInt->SetGetCommand("GetPhiResolution");
  mInt->AddIntegerArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Start Phi");
  mInt->SetSetCommand("SetStartPhi");
  mInt->SetGetCommand("GetStartPhi");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("End Phi");
  mInt->SetSetCommand("SetEndPhi");
  mInt->SetGetCommand("GetEndPhi");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Cone ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkConeSource");
  sInt->SetRootName("Cone");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Resolution");
  mInt->SetSetCommand("SetResolution");
  mInt->SetGetCommand("GetResolution");
  mInt->AddIntegerArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Height");
  mInt->SetSetCommand("SetHeight");
  mInt->SetGetCommand("GetHeight");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Capping");
  mInt->SetSetCommand("SetCapping");
  mInt->SetGetCommand("GetCapping");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Axes ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkAxes");
  sInt->SetRootName("Axes");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Scale");
  mInt->SetSetCommand("SetScaleFactor");
  mInt->SetGetCommand("GetScaleFactor");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Origin");
  mInt->SetSetCommand("SetOrigin");
  mInt->SetGetCommand("GetOrigin");
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Symmetric");
  mInt->SetSetCommand("SetSymmetric");
  mInt->SetGetCommand("GetSymmetric");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- VectorText ----
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkVectorText");
  sInt->SetRootName("VectorText");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Text");
  mInt->SetSetCommand("SetText");
  mInt->SetGetCommand("GetText");
  mInt->AddStringArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ============= DataSet Sources ==============  
  
  // ---- GenericDataSetReader ----.
  sInt = vtkPVDataSetReaderInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkDataSetReader");
  sInt->SetRootName("DataSet");
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- GenericEnSightReader ----.
  sInt = vtkPVEnSightReaderInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkGenericEnSightReader");
  sInt->SetRootName("EnSight");
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // DataSet to PolyData Filters
  
  // ---- ExtractEdges ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkExtractEdges");
  sInt->SetRootName("ExtractEdges");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Contour ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSingleContourFilter");
  sInt->SetRootName("SingleContour");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetFirstValue");
  mInt->SetGetCommand("GetFirstValue");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Normals");
  mInt->SetSetCommand("SetComputeNormals");
  mInt->SetGetCommand("GetComputeNormals");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Gradients");
  mInt->SetSetCommand("SetComputeGradients");
  mInt->SetGetCommand("GetComputeGradients");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Scalars");
  mInt->SetSetCommand("SetComputeScalars");
  mInt->SetGetCommand("GetComputeScalars");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Cut Plane ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCutPlane");
  sInt->SetRootName("CutPlane");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Origin");
  mInt->SetSetCommand("SetOrigin");
  mInt->SetGetCommand("GetOrigin");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Normal");
  mInt->SetSetCommand("SetNormal");
  mInt->SetGetCommand("GetNormal");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GenerateCutScalars");
  mInt->SetSetCommand("SetGenerateCutScalars");
  mInt->SetGetCommand("GetGenerateCutScalars");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Cut Material ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCutMaterial");
  sInt->SetRootName("CutMaterial");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("MaterialArray");
  mInt->SetSetCommand("SetMaterialArrayName");
  mInt->SetGetCommand("GetMaterialArrayName");
  mInt->AddStringArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Material");
  mInt->SetSetCommand("SetMaterial");
  mInt->SetGetCommand("GetMaterial");
  mInt->AddIntegerArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Array");
  mInt->SetSetCommand("SetArrayName");
  mInt->SetGetCommand("GetArrayName");
  mInt->AddStringArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("UpVector");
  mInt->SetSetCommand("SetUpVector");
  mInt->SetGetCommand("GetUpVector");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ============= PolyData to PolyData Filters ==============
  
  // ---- Cut Plane ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkClipPlane");
  sInt->SetRootName("ClipPlane");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Origin");
  mInt->SetSetCommand("SetOrigin");
  mInt->SetGetCommand("GetOrigin");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Normal");
  mInt->SetSetCommand("SetNormal");
  mInt->SetGetCommand("GetNormal");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetValue");
  mInt->SetGetCommand("GetValue");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("InsideOut");
  mInt->SetSetCommand("SetInsideOut");
  mInt->SetGetCommand("GetInsideOut");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GenerateClipScalars");
  mInt->SetSetCommand("SetGenerateClipScalars");
  mInt->SetGetCommand("GetGenerateClipScalars");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Clip Scalars ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkClipPolyData");
  sInt->SetRootName("ClipScalars");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetValue");
  mInt->SetGetCommand("GetValue");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("InsideOut");
  mInt->SetSetCommand("SetInsideOut");
  mInt->SetGetCommand("GetInsideOut");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Shrink ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkShrinkPolyData");
  sInt->SetRootName("Shrink");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ShrinkFactor");
  mInt->SetSetCommand("SetShrinkFactor");
  mInt->SetGetCommand("GetShrinkFactor");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Tube ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkTubeFilter");
  sInt->SetRootName("Tuber");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfSides");
  mInt->SetSetCommand("SetNumberOfSides");
  mInt->SetGetCommand("GetNumberOfSides");
  mInt->AddIntegerArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Capping");
  mInt->SetSetCommand("SetCapping");
  mInt->SetGetCommand("GetCapping");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("VaryRadius");
  mInt->SetSetCommand("SetVaryRadius");
  mInt->SetGetCommand("GetVaryRadius");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "Off");
  mInt->AddSelectionEntry(1, "ByScalar");
  mInt->AddSelectionEntry(2, "ByVector");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("RadiusFactor");
  mInt->SetSetCommand("SetRadiusFactor");
  mInt->SetGetCommand("GetRadiusFactor");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- LinearExtrusion ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkLinearExtrusionFilter");
  sInt->SetRootName("LinExtrude");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Capping");
  mInt->SetSetCommand("SetCapping");
  mInt->SetGetCommand("GetCapping");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ScaleFactor");
  mInt->SetSetCommand("SetScaleFactor");
  mInt->SetGetCommand("GetScaleFactor");
  mInt->AddFloatArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Vector");
  mInt->SetSetCommand("SetVector");
  mInt->SetGetCommand("GetVector");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ExtractPolyDataPiece ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkExtractPolyDataPiece");
  sInt->SetRootName("PDPiece");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GhostCells");
  mInt->SetSetCommand("SetCreateGhostCells");
  mInt->SetGetCommand("GetCreateGhostCells");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // UnstructuredGrid to UnstructuredGrid Filters

  // ---- ExtractPolyDataPiece ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkExtractUnstructuredGridPiece");
  sInt->SetRootName("UGPiece");
  sInt->SetInputClassName("vtkUnstructuredGrid");
  sInt->SetOutputClassName("vtkUnstructuredGrid");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GhostCells");
  mInt->SetSetCommand("SetCreateGhostCells");
  mInt->SetGetCommand("GetCreateGhostCells");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;


  // StructuredGrid to PolyData Filters
  
  // ---- Geometry ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkStructuredGridGeometryFilter");
  sInt->SetRootName("ExtractGeom");
  sInt->SetInputClassName("vtkStructuredGrid");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Extent");
  mInt->SetSetCommand("SetExtent");
  mInt->SetGetCommand("GetExtent");
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  
  // DataSet to DataSet Filters
  
  // ---- PieceScalars ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPieceScalars");
  sInt->SetRootName("ColorPieces");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ExtractEdges ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkElevationFilter");
  sInt->SetRootName("Elevation");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("LowPoint");
  mInt->SetSetCommand("SetLowPoint");
  mInt->SetGetCommand("GetLowPoint");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("HighPoint");
  mInt->SetSetCommand("SetHighPoint");
  mInt->SetGetCommand("GetHighPoint");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ScalarRange");
  mInt->SetSetCommand("SetScalarRange");
  mInt->SetGetCommand("GetScalarRange");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument(); 
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- FieldDataToAttributeDataFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSimpleFieldDataToAttributeDataFilter");
  sInt->SetRootName("FieldToAttr");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("AttributeType");
  mInt->SetSetCommand("SetAttributeType");
  mInt->SetGetCommand("GetAttributeType");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "Point");
  mInt->AddSelectionEntry(1, "Cell");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Attribute");
  mInt->SetSetCommand("SetAttribute");
  mInt->SetGetCommand("GetAttribute");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "Scalars");
  mInt->AddSelectionEntry(1, "Vectors");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Field Name");
  mInt->SetSetCommand("SetFieldName");
  mInt->SetGetCommand("GetFieldName");
  mInt->AddStringArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // Structured grid sources

  // ---- POP Reader ----
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPOPReader");
  sInt->SetRootName("POPReader");
  sInt->SetOutputClassName("vtkStructuredGrid");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FileName");
  mInt->SetSetCommand("SetFileName");
  mInt->SetGetCommand("GetFileName");
  mInt->SetWidgetTypeToFile();
  mInt->SetFileExtension("pop");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();  
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

}
