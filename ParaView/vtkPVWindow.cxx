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
#include "vtkSuperquadricSource.h"
#include "vtkImageMandelbrotSource.h"

#include "vtkPVSourceInterface.h"
#include "vtkPVEnSightReaderInterface.h"

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
  this->GetMenuProperties()->AddRadioButton(1," ParaView Window", 
                                           rbv, this, "ShowWindowProperties");
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
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
               this->ResetCameraButton->GetWidgetName());
  
  this->SourceListButton->SetParent(this->Toolbar);
  this->SourceListButton->Create(app, "-text SourceList");
  this->SourceListButton->SetCommand(this, "ShowWindowProperties");
  this->CameraStyleButton->SetParent(this->Toolbar);
  this->CameraStyleButton->Create(app, "");
  this->CameraStyleButton->SetLabel("Camera");
  this->CameraStyleButton->SetCommand(this, "UseCameraStyle");
  this->Script("pack %s %s -side left -pady 0 -fill none -expand no",
	       this->SourceListButton->GetWidgetName(),
	       this->CameraStyleButton->GetWidgetName());
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
  
  // ---- EnSightGoldReader ----.
  sInt = vtkPVEnSightReaderInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkEnSightGoldReader");
  sInt->SetRootName("EnSightGoldReader");
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- EnSight6Reader ----.
  sInt = vtkPVEnSightReaderInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkEnSight6Reader");
  sInt->SetRootName("EnSight6Reader");
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

  // ============= PolyData to PolyData Filters ==============
  
  // ---- PieceScalars ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPieceScalars");
  sInt->SetRootName("ColorPieces");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
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

}

