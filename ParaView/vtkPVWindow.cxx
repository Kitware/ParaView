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

#include "vtkPVAssignment.h"
#include "vtkPVSource.h"
#include "vtkPVPolyData.h"
#include "vtkPVImageData.h"
#include "vtkPVSourceList.h"
#include "vtkPVActorComposite.h"
#include "vtkPVAnimation.h"
#include "vtkSuperquadricSource.h"
#include "vtkRunTimeContour.h"
#include "vtkImageMandelbrotSource.h"


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
  this->SourceListButton = vtkKWPushButton::New();
  this->CameraStyleButton = vtkKWPushButton::New();
  
  this->Sources = vtkKWCompositeCollection::New();
  
  this->ApplicationAreaFrame = vtkKWLabeledFrame::New();
  this->SourceList = vtkPVSourceList::New();
  this->SourceList->SetSources(this->Sources);
  this->SourceList->SetParent(this->ApplicationAreaFrame->GetFrame());
  
  this->CameraStyle = vtkInteractorStyleTrackballCamera::New();
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
  this->SourceListButton->Delete();
  this->SourceListButton = NULL;
  this->CameraStyleButton->Delete();
  this->CameraStyleButton = NULL;
  
  this->SourceList->Delete();
  this->SourceList = NULL;
  
  this->ApplicationAreaFrame->Delete();
  
  this->CameraStyle->Delete();
  this->CameraStyle = NULL;
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
  this->MenuFile->InsertCommand(2, "Save Camera", this, "Save");
  this->MenuFile->InsertCommand(2, "Save", this, "SavePipeline");

  // Create the menu for creating data sources.  
  this->CreateMenu->SetParent(this->GetMenu());
  this->CreateMenu->Create(this->Application,"-tearoff 0");
  this->Menu->InsertCascade(2,"Create",this->CreateMenu,0);

  this->CreateMenu->AddCommand("ImageReader", this, "CreateImageReader");
  this->CreateMenu->AddCommand("FractalVolume", this, "CreateFractalVolume");
  this->CreateMenu->AddCommand("STLReader", this, "CreateSTLReader");
  this->CreateMenu->AddCommand("RunTimeContour", this, "CreateRunTimeContour");
  this->CreateMenu->AddCommand("Cone", this, "CreateCone");
  this->CreateMenu->AddCommand("Sphere", this, "CreateSphere");
  this->CreateMenu->AddCommand("Axes", this, "CreateAxes");
  this->CreateMenu->AddCommand("Cube", this, "CreateCube");
  this->CreateMenu->AddCommand("Cylinder", this, "CreateCylinder");
  this->CreateMenu->AddCommand("Disk", this, "CreateDisk");
  this->CreateMenu->AddCommand("Line", this, "CreateLine");
  this->CreateMenu->AddCommand("Plane", this, "CreatePlane");
  this->CreateMenu->AddCommand("Points", this, "CreatePoints");
  this->CreateMenu->AddCommand("Superquadric", this, "CreateSuperQuadric");
  this->CreateMenu->AddCommand("Animation", this, "CreateAnimation");

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
  this->SourceListButton->SetParent(this->Toolbar);
  this->SourceListButton->Create(app, "-text SourceList");
  this->SourceListButton->SetCommand(this, "ShowWindowProperties");
  this->CameraStyleButton->SetParent(this->Toolbar);
  this->CameraStyleButton->Create(app, "");
  this->CameraStyleButton->SetLabel("Camera");
  this->CameraStyleButton->SetCommand(this, "UseCameraStyle");
  this->Script("pack %s %s %s %s -side left -pady 0 -fill none -expand no",
	       this->PreviousSourceButton->GetWidgetName(),
	       this->NextSourceButton->GetWidgetName(),
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
//  vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
//  this->MainView->SetInteractorStyle(style);
  this->MainView->SetInteractorStyle(this->CameraStyle);
}

void vtkPVWindow::UseCameraStyle()
{
  this->GetMainView()->SetInteractorStyle(this->CameraStyle);
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
vtkPVPolyDataSource *vtkPVWindow::CreateCone()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkConeSource",
                            "Cone", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("Resolution:", "SetResolution", "GetResolution");
  pvs->AddLabeledEntry("Height:", "SetHeight", "GetHeight");
  pvs->AddLabeledEntry("Radius:", "SetRadius", "GetRadius");
  pvs->AddLabeledToggle("Capping:", "SetCapping", "GetCapping");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
}

//----------------------------------------------------------------------------
vtkPVRunTimeContour *vtkPVWindow::CreateRunTimeContour()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkKWPushButton *button = vtkKWPushButton::New();
  vtkKWScale *scale;
  char command[50];
  vtkRunTimeContour *rtc;
  
  // Create the pvSource. Clone the PVSource and the vtkSource.
  // Link the PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVRunTimeContour", "vtkRunTimeContour",
			    "RunTimeContour", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Set the default file name
  pvApp->Script("%s SetFileName [tk_getOpenFile -filetypes {{{} {.vtk}}}]",
                pvs->GetVTKSourceTclName());

  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddFileEntry("FileName:", "SetFileName", "GetFileName", "vtk");
  rtc = vtkRunTimeContour::SafeDownCast(pvs->GetVTKSource());
  scale = pvs->AddScale("ContourValue:", "SetContourValue", "GetContourValue", 
			0, 1, 0.1);
  rtc->SetContourScale(scale);
  scale = pvs->AddScale("XSlice:", "SetXSlice", "GetXSlice", 
			0, 100, 1);
  rtc->SetXScale(scale);
  scale = pvs->AddScale("YSlice:", "SetYSlice", "GetYSlice", 
			0, 100, 1);
  rtc->SetYScale(scale);
  scale = pvs->AddScale("ZSlice:", "SetZSlice", "GetZSlice", 
			0, 100, 1);
  rtc->SetZScale(scale);
  pvs->UpdateParameterWidgets();
  
  pvs->GetWidgets()->AddItem(button);
  button->SetParent(pvs->GetParameterFrame()->GetFrame());
  button->Create(pvApp, "");
  button->SetLabel("Update");
  sprintf(command, "{%s UpdateWidgets}", pvs->GetVTKSourceTclName());
  this->Script("%s configure -command %s", button->GetWidgetName(), command);
  this->Script("pack %s -side left", button->GetWidgetName());
  
  button->Delete();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVRunTimeContour::SafeDownCast(pvs);
}

//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateSphere()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkSphereSource",
                            "Sphere",++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("Radius:", "SetRadius", "GetRadius");
  pvs->AddVector3Entry("Center", "X","Y","Z", "SetCenter", "GetCenter");
  pvs->AddLabeledEntry("Phi Resolution:", "SetPhiResolution", "GetPhiResolution");
  pvs->AddLabeledEntry("Theta Resolution:", "SetThetaResolution", "GetThetaResolution");
  pvs->AddLabeledEntry("Start Theta:", "SetStartTheta", "GetStartTheta");
  pvs->AddLabeledEntry("End Theta:", "SetEndTheta", "GetEndTheta");
  pvs->AddLabeledEntry("Start Phi:", "SetStartPhi", "GetStartPhi");
  pvs->AddLabeledEntry("End Phi:", "SetEndPhi", "GetEndPhi");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 

//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateAxes()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkAxes",
                            "Axes", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("Scale:", "SetScaleFactor", "GetScaleFactor");
  pvs->AddVector3Entry("Origin", "X","Y","Z", "SetOrigin", "GetOrigin");
  pvs->AddLabeledToggle("Symmetric:", "SetSymmetric", "GetSymmetric");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
}
 
//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateCube()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkCubeSource",
                            "Cube", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("X Length:", "SetXLength", "GetXLength");
  pvs->AddLabeledEntry("Y Length:", "SetYLength", "GetYLength");
  pvs->AddLabeledEntry("Z Length:", "SetZLength", "GetZLength");
  pvs->AddLabeledEntry("Center:", "SetCenter", "GetCenter");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 
//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateCylinder()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkCylinderSource",
                            "Cylinder", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("Height:", "SetHeight", "GetHeight");
  pvs->AddLabeledEntry("Radius:", "SetRadius", "GetRadius");
  pvs->AddLabeledEntry("Resolution:", "SetResolution", "GetResolution");
  pvs->AddVector3Entry("Center", "X","Y","Z", "SetCenter", "GetCenter");
  pvs->AddLabeledToggle("Capping:", "SetCapping", "GetCapping");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 
//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateDisk()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkDiskSource",
                            "Disk", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("InnerRadius:", "SetInnerRadius", "GetInnerRadius");
  pvs->AddLabeledEntry("OuterRadius:", "SetOuterRadius", "GetOuterRadius");
  pvs->AddLabeledEntry("Radial Res:", "SetRadialResolution", "GetRadialResolution");
  pvs->AddLabeledEntry("Circumference Res:", "SetCircumferentialResolution", "GetCircumferentialResolution");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 
//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateLine()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkLineSource",
                            "Line", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddVector3Entry("Point1:", "X","Y","Z", "SetPoint1", "GetPoint1");
  pvs->AddVector3Entry("Point2:", "X","Y","Z", "SetPoint2", "GetPoint2");
  pvs->AddLabeledEntry("Resolution:", "SetResolution", "GetResolution");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 
//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreatePlane()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkPlaneSource",
                            "Plane", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("X Resolution:", "SetXResolution", "GetXResolution");
  pvs->AddLabeledEntry("Y Resolution:", "SetYResolution", "GetYResolution");
  pvs->AddVector3Entry("Origin:","X","Y","Z", "SetOrigin", "GetOrigin");
  pvs->AddVector3Entry("Point1:","X","Y","Z", "SetPoint1", "GetPoint1");
  pvs->AddVector3Entry("Point2:","X","Y","Z", "SetPoint2", "GetPoint2");

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 
//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreatePoints()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkPointSource",
                            "Points", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("NumberOfPoints:", "SetNumberOfPoints", "GetNumberOfPoints");
  pvs->AddLabeledEntry("Radius:", "SetRadius", "GetRadius");
  pvs->AddVector3Entry("Center:", "X","Y","Z", "SetCenter", "GetCenter");
  pvs->AddModeList("Distribution:", "SetDistribution", "GetDistribution");
  pvs->AddModeListItem("Shell", 0);
  pvs->AddModeListItem("Uniform", 1);
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 

//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateSTLReader()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkSTLReader",
                            "STL", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Set the default file name
  pvApp->Script("%s SetFileName [tk_getOpenFile -filetypes {{{} {.stl}}}]",
                pvs->GetVTKSourceTclName());

  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddFileEntry("FileName:", "SetFileName", "GetFileName", "stl");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
} 

//----------------------------------------------------------------------------
vtkPVPolyDataSource *vtkPVWindow::CreateSuperQuadric()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Linkthe PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVPolyDataSource","vtkSuperquadricSource",
                            "Superquadric", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledToggle("Toroidal:","SetToroidal","GetToroidal");
  pvs->AddVector3Entry("Center:","X","Y","Z","SetCenter","GetCenter");
  pvs->AddVector3Entry("Scale:","X","Y","Z","SetScale","GetScale");
  pvs->AddLabeledEntry("ThetaResolution:","SetThetaResolution","GetThetaResolution");
  pvs->AddLabeledEntry("PhiResolution:","SetPhiResolution","GetPhiResolution");
  pvs->AddScale("Thickness:","SetThickness","GetThickness",
                VTK_MIN_SUPERQUADRIC_THICKNESS, 1.0, 0.05);
  pvs->AddScale("ThetaRoundness:","SetThetaRoundness","GetThetaRoundness",
                0.0, 1.0, 0.05);
  pvs->AddScale("PhiRoundness:","SetPhiRoundness","GetPhiRoundness",
                0.0, 1.0, 0.05);
  pvs->AddLabeledEntry("Size:","SetSize","GetSize");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVPolyDataSource::SafeDownCast(pvs);
}

//----------------------------------------------------------------------------
// Setup the pipeline
vtkPVImageSource *vtkPVWindow::CreateImageReader()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVImageSource","vtkImageReader",
                            "ImageReader", ++instanceCount);
  if (pvs == NULL) {return NULL;}
  
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddLabeledEntry("FilePrefix:", "SetFilePrefix", "GetFilePrefix");
  pvs->AddLabeledEntry("FilePattern:", "SetFilePattern", "GetFilePattern");
  pvs->AddModeList("ByteOrder", "SetDataByteOrder", "GetDataByteOrder");
  pvs->AddModeListItem("BigEndian", 0);
  pvs->AddModeListItem("LittleEndian", 1);
  pvs->AddVector6Entry("DataExtent:", "X Min", "X Max", "Y Min", "Y Max",
		       "Z Min", "Z Max", "SetDataExtent", "GetDataExtent");
  pvs->AddVector3Entry("DataSpacing:", "X", "Y", "Z", "SetDataSpacing", "GetDataSpacing");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
  return vtkPVImageSource::SafeDownCast(pvs);
}

//----------------------------------------------------------------------------
// Setup the pipeline
void vtkPVWindow::CreateFractalVolume()
{
  static int instanceCount = 0;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkImageMandelbrotSource *ms;

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  pvs = pvApp->MakePVSource("vtkPVImageSource","vtkImageMandelbrotSource",
                            "Fractal", ++instanceCount);
  if (pvs == NULL) {return;}
  
  // Set up the defaults.
  ms = vtkImageMandelbrotSource::SafeDownCast(pvs->GetVTKSource());
  ms->SetOriginCX(-0.733569, 0.24405, 0.296116, 0.0);
  ms->SetSampleCX(1.38125e-005, 1.38125e-005, 1.0e-004, 1.0e-004);
  ms->SetWholeExtent(-50, 50, -50, 50, -50, 50);
  /* Small values get munged through tcl?
  this->Script("%s SetOriginCX -0.733569 0.24405 0.296116 0.0253163", 
               pvs->GetVTKSourceTclName());
  this->Script("%s SetSampleCX 1.38125e-005 1.38125e-005 1.0e-004 1.0e-004", 
               pvs->GetVTKSourceTclName());
  this->Script("%s SetWholeExtent -50 50 -50 50 -50 50", 
               pvs->GetVTKSourceTclName());
  */
  // Add the new Source to the View, and make it current.
  this->MainView->AddComposite(pvs);
  this->SetCurrentSource(pvs);

  // Add some source specific widgets.
  // Normally these would be added in the create method.
  pvs->AddVector6Entry("Extent:","X",NULL,"Y",NULL,"Z",NULL,"SetWholeExtent","GetWholeExtent");
  pvs->AddVector3Entry("SubSpace:","X","Y","Z", "SetProjectionAxes", "GetProjectionAxes");
  pvs->AddVector4Entry("Origin:","Cr","Ci","Xr","Xi","SetOriginCX","GetOriginCX");
  pvs->AddVector4Entry("Spacing:","Cr","Ci","Xr","Xi","SetSampleCX","GetSampleCX");
  pvs->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //pvs->Delete();
}

//----------------------------------------------------------------------------
void vtkPVWindow::CreateAnimation()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVAnimation *anim;
  
  // Create the pipeline objects in all processes.
  anim = vtkPVAnimation::New();
  // Although this does not need to be cloned to implement animations,
  // we are using the AutoWidgets wich broadcast there callbacks.  The
  // The clones will just act like dummies.
  anim->Clone(pvApp);
  
  anim->SetName("Animation");
  
  // Add the new Source to the View (in all processes).
  this->MainView->AddComposite(anim);
  anim->SetObject(this->GetCurrentSource());

  // Select this Source
  this->SetCurrentSource(anim);
  
  // Clean up. (How about on the other processes?)
  anim->Delete();
  anim = NULL;
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
    this->GetCurrentSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    this->SetCurrentSource(composite);
    this->GetCurrentSource()->GetPVData()->GetActorComposite()->VisibilityOn();
    }
  
  this->MainView->Render();
  this->SourceList->Update();
  this->MainView->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVWindow::PreviousSource()
{
  vtkPVSource *composite = this->GetPreviousSource();
  if (composite != NULL)
    {
    this->GetCurrentSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    this->SetCurrentSource(composite);
    this->GetCurrentSource()->GetPVData()->GetActorComposite()->VisibilityOn();
    }
  
  this->MainView->Render();
  this->SourceList->Update();
  this->MainView->ResetCamera();
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
