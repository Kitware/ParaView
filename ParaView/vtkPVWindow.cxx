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

#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVActorComposite.h"

#include "vtkPVSourceInterface.h"
#include "vtkPVEnSightReaderInterface.h"
#include "vtkPVDataSetReaderInterface.h"
#include "vtkPVArrayCalculator.h"
#include "vtkPVThreshold.h"
#include "vtkPVContour.h"
#include "vtkPVGlyph3D.h"
#include "vtkPVProbe.h"

#include "vtkKWInteractor.h"
#include "vtkKWFlyInteractor.h"
#include "vtkKWRotateCameraInteractor.h"
#include "vtkKWTranslateCameraInteractor.h"
#include "vtkKWInteractor.h"

#include <ctype.h>

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
  this->SourceMenu = vtkKWMenu::New();
  this->FilterMenu = vtkKWMenu::New();
  this->SelectMenu = vtkKWMenu::New();
  this->VTKMenu = vtkKWMenu::New();
  this->InteractorToolbar = vtkKWToolbar::New();

  this->FlyInteractor = vtkKWFlyInteractor::New();
  this->RotateCameraInteractor = vtkKWRotateCameraInteractor::New();
  this->TranslateCameraInteractor = vtkKWTranslateCameraInteractor::New();
  this->SelectPointInteractor = vtkKWSelectPointInteractor::New();
  
  this->Toolbar = vtkKWToolbar::New();
  this->CalculatorButton = vtkKWPushButton::New();
  this->ThresholdButton = vtkKWPushButton::New();
  this->ContourButton = vtkKWPushButton::New();
  this->GlyphButton = vtkKWPushButton::New();
  this->ProbeButton = vtkKWPushButton::New();

  this->FrameRateLabel = vtkKWLabel::New();
  this->FrameRateScale = vtkKWScale::New();

  this->ReductionCheck = vtkKWCheckButton::New();
  
  this->Sources = vtkKWCompositeCollection::New();
  
  this->ApplicationAreaFrame = vtkKWLabeledFrame::New();
  
  this->CameraStyle = vtkInteractorStyleTrackballCamera::New();
  
  this->SourceInterfaces = vtkCollection::New();
  this->CurrentPVData = NULL;
  //this->CurrentInteractor = NULL;
}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  this->PrepareForDelete();

  this->Sources->Delete();
  this->Sources = NULL;
}

//----------------------------------------------------------------------------
void vtkPVWindow::PrepareForDelete()
{
  if (this->SourceInterfaces)
    {
    this->SourceInterfaces->Delete();
    this->SourceInterfaces = NULL;
    }

  if (this->InteractorToolbar)
    {
    this->InteractorToolbar->Delete();
    this->InteractorToolbar = NULL;
    }

  if (this->FlyInteractor)
    {
    this->FlyInteractor->Delete();
    this->FlyInteractor = NULL;
    }

  if (this->RotateCameraInteractor)
    {
    // Circular reference.
    this->RotateCameraInteractor->PrepareForDelete();
    this->RotateCameraInteractor->Delete();
    this->RotateCameraInteractor = NULL;
    }
  if (this->TranslateCameraInteractor)
    {
    this->TranslateCameraInteractor->Delete();
    this->TranslateCameraInteractor = NULL;
    }
  if (this->SelectPointInteractor)
    {
    this->SelectPointInteractor->Delete();
    this->SelectPointInteractor = NULL;
    }
  
  if (this->CalculatorButton)
    {
    this->CalculatorButton->Delete();
    this->CalculatorButton = NULL;
    }
  
  if (this->ThresholdButton)
    {
    this->ThresholdButton->Delete();
    this->ThresholdButton = NULL;
    }
  
  if (this->ContourButton)
    {
    this->ContourButton->Delete();
    this->ContourButton = NULL;
    }

  if (this->GlyphButton)
    {
    this->GlyphButton->Delete();
    this->GlyphButton = NULL;
    }
  
  if (this->ProbeButton)
    {
    this->ProbeButton->Delete();
    this->ProbeButton = NULL;
    }
  
  if (this->FrameRateLabel)
    {
    this->FrameRateLabel->Delete();
    this->FrameRateLabel = NULL;
    }
  
  if (this->FrameRateScale)
    {
    this->FrameRateScale->Delete();
    this->FrameRateScale = NULL;
    }

  if (this->ReductionCheck)
    {
    this->ReductionCheck->Delete();
    this->ReductionCheck = NULL;
    }
  
  if (this->Toolbar)
    {
    this->Toolbar->Delete();
    this->Toolbar = NULL;
    }

  if (this->ApplicationAreaFrame)
    {
    this->ApplicationAreaFrame->Delete();
    this->ApplicationAreaFrame = NULL;
    }
  
  if (this->CameraStyle)
    {
    this->CameraStyle->Delete();
    this->CameraStyle = NULL;
    }
  
  if (this->MainView)
    {
    this->MainView->Delete();
    this->MainView = NULL;
    }
  
  if (this->SourceMenu)
    {
    this->SourceMenu->Delete();
    this->SourceMenu = NULL;
    }
  
  if (this->FilterMenu)
    {
    this->FilterMenu->Delete();
    this->FilterMenu = NULL;  
    }
  
  if (this->SelectMenu)
    {
    this->SelectMenu->Delete();
    this->SelectMenu = NULL;
    }
  
  if (this->VTKMenu)
    {
    this->VTKMenu->Delete();
    this->VTKMenu = NULL;
    }
  
  this->SetCurrentPVData(NULL);
  //if (this->CurrentInteractor != NULL)
  //  {
  //  this->CurrentInteractor->UnRegister(this);
  //  this->CurrentInteractor = NULL;
  //  }
}


//----------------------------------------------------------------------------
void vtkPVWindow::Close()
{
  vtkKWWindow::Close();
}


//----------------------------------------------------------------------------
void vtkPVWindow::Create(vtkKWApplication *app, char *args)
{
  vtkPVSourceInterface *sInt;
  vtkKWInteractor *interactor;
  vtkKWRadioButton *button;
  vtkKWWidget *pushButton;
  
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
  this->GetMenuProperties()->AddRadioButton(2, "Source",
                                            rbv, this,
                                            "ShowCurrentSourceProperties");
  delete [] rbv;

  // create the top level
  this->MenuFile->InsertCommand(0, "Open Data File", this, "Open");
  this->MenuFile->InsertCommand(1, "Save Tcl script", this, "SaveInTclScript");
  
  // Log stuff
  this->MenuFile->InsertCommand(4, "Open Log File", this, "StartLog");
  this->MenuFile->InsertCommand(5, "Close Log File", this, "StopLog");

  this->SelectMenu->SetParent(this->GetMenu());
  this->SelectMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(2, "Select", this->SelectMenu, 0);
  
  this->VTKMenu->SetParent(this->GetMenu());
  this->VTKMenu->Create(this->Application, "-tearoff 0");
  this->Menu->InsertCascade(3, "VTK", this->VTKMenu, 0);
  
  // Create the menu for creating data sources.  
  this->SourceMenu->SetParent(this->VTKMenu);
  this->SourceMenu->Create(this->Application, "-tearoff 0");
  this->VTKMenu->AddCascade("Sources", this->SourceMenu, 0);  
  
  // Create the menu for creating data sources.  
  this->FilterMenu->SetParent(this->VTKMenu);
  this->FilterMenu->Create(this->Application, "-tearoff 0");
  this->VTKMenu->AddCascade("Filters", this->FilterMenu, 0);  
  
  // Create all of the menu items for sources with no inputs.
  this->SourceInterfaces->InitTraversal();
  while ( (sInt = (vtkPVSourceInterface*)(this->SourceInterfaces->GetNextItemAsObject())))
    {
    if (sInt->GetInputClassName() == NULL)
      {
      // Remove "vtk" from the class name to get the menu item name.
      this->SourceMenu->AddCommand(sInt->GetSourceClassName()+3, sInt, "CreateCallback");
      }
    }
  
  this->SetStatusText("Version 1.0 beta");
  
  this->Script( "wm withdraw %s", this->GetWidgetName());

  this->InteractorToolbar->SetParent(this->GetToolbarFrame());
  this->InteractorToolbar->Create(app);

  this->Toolbar->SetParent(this->GetToolbarFrame());
  this->Toolbar->Create(app);
  
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

  this->ProbeButton->SetParent(this->Toolbar);
  this->ProbeButton->Create(app, "-text Probe");
  this->ProbeButton->SetCommand(this, "ProbeCallback");
  
  this->Script("pack %s %s %s %s %s -side left -pady 0 -fill none -expand no",
               this->CalculatorButton->GetWidgetName(),
               this->ThresholdButton->GetWidgetName(),
               this->ContourButton->GetWidgetName(),
               this->GlyphButton->GetWidgetName(),
               this->ProbeButton->GetWidgetName());

  this->FrameRateScale->SetParent(this->GetToolbarFrame());
  this->FrameRateScale->Create(app, "-resolution 0.1 -orient horizontal");
  this->FrameRateScale->SetRange(0, 50);
  this->FrameRateScale->SetValue(3.0);
  this->FrameRateScale->SetCommand(this, "FrameRateScaleCallback");
  this->FrameRateScale->SetBalloonHelpString(
    "This slider adjusts the desired frame rate for interaction.  The level of detail is adjusted to achieve the desired rate.");
  this->Script("pack %s -side right -fill none -expand no",
               this->FrameRateScale->GetWidgetName());
  
  this->FrameRateLabel->SetParent(this->GetToolbarFrame());
  this->FrameRateLabel->Create(app, "");
  this->FrameRateLabel->SetLabel("Frame Rate");
  this->Script("pack %s -side right -fill none -expand no",
               this->FrameRateLabel->GetWidgetName());

  this->ReductionCheck->SetParent(this->GetToolbarFrame());
  this->ReductionCheck->Create(app, "-text Reduction");
  this->ReductionCheck->SetState(1);
  this->ReductionCheck->SetCommand(this, "ReductionCheckCallback");
  this->ReductionCheck->SetBalloonHelpString("If selected, tree compositing will scale the size of the render window based on how long the previous render took.");
  this->Script("pack %s -side right -fill none -expand no",
               this->ReductionCheck->GetWidgetName());
  
  // This button doesn't do anything useful right now.  It was put in originally
  // so we could switch between interactor styles.
//  this->CameraStyleButton->SetParent(this->Toolbar);
//  this->CameraStyleButton->Create(app, "");
//  this->CameraStyleButton->SetLabel("Camera");
//  this->CameraStyleButton->SetCommand(this, "UseCameraStyle");
//  this->Script("pack %s %s -side left -pady 0 -fill none -expand no",
//	       this->SourceListButton->GetWidgetName(),
//	       this->CameraStyleButton->GetWidgetName());
  this->Script("pack %s %s -side left -pady 0 -fill none -expand no",
               this->InteractorToolbar->GetWidgetName(),
               this->Toolbar->GetWidgetName());
  
  this->CreateDefaultPropertiesParent();
  
  this->Notebook->SetParent(this->GetPropertiesParent());
  this->Notebook->Create(this->Application,"");
  this->Notebook->AddPage("Preferences");

  this->ApplicationAreaFrame->
    SetParent(this->Notebook->GetFrame("Preferences"));
  this->ApplicationAreaFrame->Create(this->Application);
  this->ApplicationAreaFrame->SetLabel("Sources");

  this->Script("pack %s -side top -anchor w -expand yes -fill x -padx 2 -pady 2",
               this->ApplicationAreaFrame->GetWidgetName());

  // create the main view
  // we keep a handle to them as well
  this->CreateMainView((vtkPVApplication*)app);

  // Set up the button to reset the camera.
  pushButton = vtkKWWidget::New();
  pushButton->SetParent(this->InteractorToolbar);
  pushButton->Create(app, "button", "-image KWResetViewButton -bd 0");
  pushButton->SetCommand(this, "ResetCameraCallback");
  this->Script( "pack %s -side left -fill none -expand no",
                pushButton->GetWidgetName());
  pushButton->SetBalloonHelpString(
    "Reset the view to show all the visible parts.");
  pushButton->Delete();
  pushButton = NULL;

  // set up the interactors
  interactor = this->FlyInteractor;
  interactor->SetParent(this->GetToolbarFrame());
  interactor->SetRenderView(this->GetMainView());
  interactor->Create(this->Application, "");
  button = vtkKWRadioButton::New();
  button->SetParent(this->InteractorToolbar);
  button->Create(app, "-indicatoron 0 -image KWFlyButton -selectimage KWActiveFlyButton -bd 0");
  button->SetBalloonHelpString(
    "Fly View Mode\n   Left Button: Fly toward mouse position.\n   Right Button: Fly backward");
  this->Script("%s configure -command {%s SetInteractor %s}", 
               button->GetWidgetName(), this->MainView->GetTclName(), 
               interactor->GetTclName());
  this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2", 
               button->GetWidgetName());
  interactor->SetToolbarButton(button);
  button->Delete();
  button = NULL;

  interactor = this->RotateCameraInteractor;
  interactor->SetParent(this->GetToolbarFrame());
  interactor->SetRenderView(this->GetMainView());
  interactor->Create(this->Application, "");
  button = vtkKWRadioButton::New();
  button->SetParent(this->InteractorToolbar);
  button->Create(app, "-indicatoron 0 -image KWRotateViewButton -selectimage KWActiveRotateViewButton -bd 0");
  button->SetBalloonHelpString(
    "Rotate View Mode\n   Left Button: Rotate. This depends on where you click on the screen. Near the center of rotation gives XY rotation (view coordinates). Far from the center gives you Z roll.\n   Right Button: Behaves like translate view mode.");
  this->Script("%s configure -command {%s SetInteractor %s}", 
               button->GetWidgetName(), this->MainView->GetTclName(), 
               interactor->GetTclName());
  this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2", 
               button->GetWidgetName());
  interactor->SetToolbarButton(button);
  button->Delete();
  button = NULL;

  interactor = this->TranslateCameraInteractor;
  interactor->SetParent(this->GetToolbarFrame());
  interactor->SetRenderView(this->GetMainView());
  interactor->Create(this->Application, "");
  button = vtkKWRadioButton::New();
  button->SetParent(this->InteractorToolbar);
  button->Create(app, "-indicatoron 0 -image KWTranslateViewButton -selectimage KWActiveTranslateViewButton -bd 0");
  button->SetBalloonHelpString(
    "Translate View Mode\n   Left button: Translate. This depends on where you click on the screen. Near the middle of the screen gives you XY translation. Near the top or bottom of the screen gives you zoom along Z.\n   Right Button: Zoom.");
  this->Script("%s configure -command {%s SetInteractor %s}", 
               button->GetWidgetName(), this->MainView->GetTclName(), 
               interactor->GetTclName());
  this->Script("pack %s -side left -fill none -expand no -padx 2 -pady 2", 
               button->GetWidgetName());
  interactor->SetToolbarButton(button);
  button->Delete();
  button = NULL;

//  this->SelectPointInteractor->SetParent(this->GetToolbarFrame());
  this->SelectPointInteractor->SetRenderView(this->GetMainView());
//  this->SelectPointInteractor->Create(this->Application, "");
  
  this->Script("%s SetInteractor %s", this->GetMainView()->GetTclName(),
               this->FlyInteractor->GetTclName());
  
  this->Script( "wm deiconify %s", this->GetWidgetName());  
}

//----------------------------------------------------------------------------
void vtkPVWindow::CreateMainView(vtkPVApplication *pvApp)
{
  vtkPVRenderView *view;
  
  view = vtkPVRenderView::New();
  view->CreateRenderObjects(pvApp);
  
  this->MainView = view;
  this->MainView->SetParent(this->ViewFrame);
  this->AddView(this->MainView);
  this->MainView->Create(this->Application,"-width 200 -height 200");
  this->MainView->MakeSelected();
  this->MainView->ShowViewProperties();
  this->MainView->SetupBindings();
  this->MainView->AddBindings(); // additional bindings in PV not in KW
  this->Script( "pack %s -expand yes -fill both", 
                this->MainView->GetWidgetName());  
}

//----------------------------------------------------------------------------
void vtkPVWindow::NewWindow()
{
  vtkPVWindow *nw = vtkPVWindow::New();
  nw->Create(this->Application,"");
  this->Application->AddWindow(nw);  
  nw->Delete();
}

void vtkPVWindow::Open()
{
  char *openFileName = NULL;
  char *extension = NULL;
  int i, numSourceInterfaces;
  char* className;
  vtkPVSourceInterface *sInt;
  char rootName[100];
  int position;
  char *endingSlash = NULL;
  char *newRootName;
  istream *input;
  
  this->Script("set openFileName [tk_getOpenFile -filetypes {{{VTK files} {.vtk}} {{EnSight files} {.case}} {{POP files} {.pop}} {{STL files} {.stl}}}]");
  openFileName = this->GetPVApplication()->GetMainInterp()->result;

  if (strcmp(openFileName, "") == 0)
    {
    return;
    }

  input = new ifstream(openFileName, ios::in);
  if (input->fail())
    {
    vtkErrorMacro("Permission denied for opening " << openFileName);
    delete input;
    return;
    }
  
  delete input;
  
  extension = strrchr(openFileName, '.');
  position = extension - openFileName;
  strncpy(rootName, openFileName, position);
  rootName[position] = '\0';
  if ((endingSlash = strrchr(rootName, '/')))
    {
    position = endingSlash - rootName + 1;
    newRootName = new char[strlen(rootName) - position + 1];
    strcpy(newRootName, rootName + position);
    strcpy(rootName, "");
    strcat(rootName, newRootName);
    delete [] newRootName;
    }
  if (isdigit(rootName[0]))
    {
    // A VTK object names beginning with a digit is invalid.
    newRootName = new char[strlen(rootName) + 3];
    sprintf(newRootName, "PV%s", rootName);
    strcpy(rootName, "");
    strcat(rootName, newRootName);
    delete [] newRootName;
    }
  
  numSourceInterfaces = this->SourceInterfaces->GetNumberOfItems();
  
  if (strcmp(extension, ".vtk") == 0)
    {
    for (i = 0; i < numSourceInterfaces; i++)
      {
      sInt =
        ((vtkPVSourceInterface*)this->SourceInterfaces->GetItemAsObject(i));
      className = sInt->GetSourceClassName();
      if (strcmp(className, "vtkDataSetReader") == 0)
	{
	sInt->SetDataFileName(openFileName);
        sInt->SetRootName(rootName);
	((vtkPVDataSetReaderInterface*)sInt)->CreateCallback();
	}
      }
    }
  else if (strcmp(extension, ".case") == 0)
    {
    for (i = 0; i < numSourceInterfaces; i++)
      {
      sInt =
	((vtkPVSourceInterface*)this->SourceInterfaces->GetItemAsObject(i));
      className = sInt->GetSourceClassName();
      if (strcmp(className, "vtkGenericEnSightReader") == 0)
	{
	sInt->SetDataFileName(openFileName);
        sInt->SetRootName(rootName);
	((vtkPVEnSightReaderInterface*)sInt)->CreateCallback();
	}
      }
    }
  else if (strcmp(extension, ".pop") == 0)
    {
    for (i = 0; i < numSourceInterfaces; i++)
      {
      sInt =
	((vtkPVSourceInterface*)this->SourceInterfaces->GetItemAsObject(i));
      className = sInt->GetSourceClassName();
      if (strcmp(className, "vtkPOPReader") == 0)
	{
	sInt->SetDataFileName(openFileName);
        sInt->SetRootName(rootName);
	sInt->CreateCallback();
	}
      }
    }
  else if (strcmp(extension, ".stl") == 0)
    {
    for (i = 0; i < numSourceInterfaces; i++)
      {
      sInt =
	((vtkPVSourceInterface*)this->SourceInterfaces->GetItemAsObject(i));
      className = sInt->GetSourceClassName();
      if (strcmp(className, "vtkSTLReader") == 0)
	{
	sInt->SetDataFileName(openFileName);
        sInt->SetRootName(rootName);
	sInt->CreateCallback();
	}
      }
    }
  else
    {
    vtkErrorMacro("Unknown file extension");
    }
}

//----------------------------------------------------------------------------
void vtkPVWindow::SaveInTclScript()
{
  ofstream *file;
  vtkCollection *sources;
  vtkPVSource *pvs;
  char *filename;
  
  this->Script("tk_getSaveFile -filetypes {{{Tcl Scripts} {.tcl}} {{All Files} {.*}}} -defaultextension .tcl");
  filename = this->Application->GetMainInterp()->result;
  
  if (strcmp(filename, "") == 0)
    {
    return;
    }
  
  file = new ofstream(filename, ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file pipeline.tcl");
    delete file;
    file = NULL;
    return;
    }

  *file << "# ParaView Version 0.1\n\n";

  *file << "catch {load vtktcl}\n"
        << "if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL \"../../vtk/examplesTcl/\" }\n\n"
        << "source $VTK_TCL/vtkInt.tcl\n\n"
        << "# create a rendering window and renderer\n";
  
  this->GetMainView()->SaveInTclScript(file);
  
  // Loop through sources ...
  sources = this->GetSources();
  sources->InitTraversal();

  int numSources = sources->GetNumberOfItems();
  int sourceCount = 0;

  while (sourceCount < numSources)
    {
    if (strcmp(sources->GetItemAsObject(sourceCount)->GetClassName(),
               "vtkPVArrayCalculator") == 0)
      {
      ((vtkPVArrayCalculator*)sources->GetItemAsObject(sourceCount))->SaveInTclScript(file);
      }
    else if (strcmp(sources->GetItemAsObject(sourceCount)->GetClassName(),
                    "vtkPVContour") == 0)
      {
      ((vtkPVContour*)sources->GetItemAsObject(sourceCount))->SaveInTclScript(file);
      }
    else if (strcmp(sources->GetItemAsObject(sourceCount)->GetClassName(),
                    "vtkPVGlyph3D") == 0)
      {
      ((vtkPVGlyph3D*)sources->GetItemAsObject(sourceCount))->SaveInTclScript(file);
      }
    else if (strcmp(sources->GetItemAsObject(sourceCount)->GetClassName(),
                    "vtkPVThreshold") == 0)
      {
      ((vtkPVThreshold*)sources->GetItemAsObject(sourceCount))->SaveInTclScript(file);
      }
    else
      {
      pvs = (vtkPVSource*)sources->GetItemAsObject(sourceCount);
      pvs->SaveInTclScript(file);
      }
    sourceCount++;
    }

  this->GetMainView()->AddActorsToTclScript(file);
    
  *file << "# enable user interface interactor\n"
        << "iren SetUserMethod {wm deiconify .vtkInteract}\n"
        << "iren Initialize\n\n"
        << "# prevent the tk window from showing up then start the event loop\n"
        << "wm withdraw .\n";

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
    if (this->FilterMenu)
      {
      this->FilterMenu->DeleteAllMenuItems();
      }
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
    }
  
  // update the input list and (if this source is a glyph) the source list
  if (comp)
    {
    comp->UpdateInputList();
    if (comp->IsA("vtkPVGlyph3D"))
      {
      ((vtkPVGlyph3D*)comp)->UpdateSourceMenu();
      }
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
  this->MainView->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVWindow::FrameRateScaleCallback()
{
  float newRate = this->FrameRateScale->GetValue();
  if (newRate <= 0.0)
    {
    newRate = 0.00001;
    }
  this->GetMainView()->SetInteractiveUpdateRate(newRate);
}

//----------------------------------------------------------------------------
void vtkPVWindow::ReductionCheckCallback()
{
  int reduce = this->ReductionCheck->GetState();
  this->GetMainView()->SetUseReductionFactor(reduce);
}

//----------------------------------------------------------------------------
void vtkPVWindow::CalculatorCallback()
{
  static int instanceCount = 1;
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
  calc->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  calc->SetApplication(pvApp);
  calc->SetVTKSource(s, tclName);
  calc->SetNthPVInput(0, this->GetCurrentPVData());
  calc->SetName(tclName);  

  this->GetMainView()->AddComposite(calc);
  calc->CreateProperties();
  calc->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(calc);

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
  pvd->Delete();
  
  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ThresholdCallback()
{
  static int instanceCount = 1;
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
  threshold->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  threshold->SetApplication(pvApp);
  threshold->SetVTKSource(s, tclName);
  threshold->SetNthPVInput(0, this->GetCurrentPVData());
  threshold->SetName(tclName);

  this->GetMainView()->AddComposite(threshold);
  threshold->CreateProperties();
  threshold->PackScalarsMenu();
  threshold->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(threshold);

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
  pvd->Delete();
  
  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ContourCallback()
{
  static int instanceCount = 1;
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
  contour->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  contour->SetApplication(pvApp);
  contour->SetVTKSource(s, tclName);
  contour->SetNthPVInput(0, this->GetCurrentPVData());
  contour->SetName(tclName);

  this->GetMainView()->AddComposite(contour);
  contour->CreateProperties();
  contour->PackScalarsMenu();
  contour->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(contour);

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
  pvd->Delete();
  
  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::GlyphCallback()
{
  static int instanceCount = 1;
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
  glyph->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  glyph->SetApplication(pvApp);
  glyph->SetVTKSource(s, tclName);
  glyph->SetNthPVInput(0, this->GetCurrentPVData());
  glyph->SetName(tclName);

  this->GetMainView()->AddComposite(glyph);
  glyph->CreateProperties();
  glyph->PackScalarsMenu();
  glyph->PackVectorsMenu();
  glyph->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(glyph);

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
  pvd->Delete();
  
  ++instanceCount;
}

//----------------------------------------------------------------------------
void vtkPVWindow::ProbeCallback()
{
  static int instanceCount = 1;
  char tclName[100];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVProbe *probe;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  vtkPVData *current;
  const char* outputDataType;
  int i, numMenus;
  
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("No data to use as source for vtkProbeFilter");
    return;
    }
  outputDataType = "vtkPolyData";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Probe", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkProbeFilter", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return;
    }
  
  probe = vtkPVProbe::New();
  probe->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  probe->SetApplication(pvApp);
  probe->SetVTKSource(s, tclName);
  probe->SetProbeSourceTclName(this->GetCurrentPVData()->GetVTKDataTclName());
  probe->SetPVProbeSource(this->GetCurrentPVData());
  probe->SetName(tclName);

  this->GetMainView()->AddComposite(probe);
  probe->CreateProperties();
  this->SetCurrentPVSource(probe);

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "Probe", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  probe->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", probe->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());

  probe->Delete();
  pvd->Delete();
  
  ++instanceCount;
  
  this->Script("%s index end", this->Menu->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  // deactivating menus and toolbar buttons (except the interactors)
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state disabled",
                 this->Menu->GetWidgetName(), i);
    }
  this->Script("%s configure -state disabled",
               this->CalculatorButton->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->ThresholdButton->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->ContourButton->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->GlyphButton->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->ProbeButton->GetWidgetName());
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
  this->ShowProperties();
  
  // We need to update the properties-menu radio button too!
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(), "Radio", 2);

  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->MainView->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill x",
               this->MainView->GetNavigationFrame()->GetWidgetName());
  
  if (!this->GetCurrentPVSource())
    {
    return;
    }
  
  this->GetCurrentPVSource()->UpdateInputList();
  if (this->GetCurrentPVSource()->IsA("vtkPVGlyph3D"))
    {
    ((vtkPVGlyph3D*)this->GetCurrentPVSource())->UpdateSourceMenu();
    }

  this->Script("pack %s -side top -fill x",
               this->GetCurrentPVSource()->GetNotebook()->GetWidgetName());
  this->GetCurrentPVSource()->GetNotebook()->Raise("Source");
}

//----------------------------------------------------------------------------
vtkPVApplication *vtkPVWindow::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}


//----------------------------------------------------------------------------
void vtkPVWindow::StartLog()
{
  char *filename = NULL;
  
  this->Script("tk_getSaveFile -filetypes {{{Log Text File} {.txt}}}");
  filename = this->Application->GetMainInterp()->result;

  if (strcmp(filename, "") == 0)
    {
    return;
    }
  
  this->GetPVApplication()->StartLog(filename);
}

//----------------------------------------------------------------------------
void vtkPVWindow::StopLog()
{
  this->GetPVApplication()->StopLog();
}

//----------------------------------------------------------------------------
// Lets experiment with an interface prototype.
// We will eventually read these from a file.
void vtkPVWindow::ReadSourceInterfaces()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVMethodInterface *mInt;
  vtkPVSourceInterface *sInt;
  

  // ---- Arrow Source ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkArrowSource");
  sInt->SetRootName("Arrow");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("TipResolution");
  mInt->SetSetCommand("SetTipResolution");
  mInt->SetGetCommand("GetTipResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the number of faces on the tip.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("TipRadius");
  mInt->SetSetCommand("SetTipRadius");
  mInt->SetGetCommand("GetTipRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the radius of the widest part of the tip.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("TipLength");
  mInt->SetSetCommand("SetTipLength");
  mInt->SetGetCommand("GetTipLength");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the length of the tip (the whole arrow is length 1)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ShaftResolution");
  mInt->SetSetCommand("SetShaftResolution");
  mInt->SetGetCommand("GetShaftResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the number of faces on shaft");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ShaftRadius");
  mInt->SetSetCommand("SetShaftRadius");
  mInt->SetGetCommand("GetShaftRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the radius of the shaft");
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
  mInt->SetBalloonHelp("Set the size of the axes");
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
  mInt->SetBalloonHelp("Set the x, y, z coordinates of the origin of the axes");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Symmetric");
  mInt->SetSetCommand("SetSymmetric");
  mInt->SetGetCommand("GetSymmetric");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to display the negative axes");
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
  mInt->SetBalloonHelp("Set the number of faces on this cone");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the radius of the widest part of the cone");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Height");
  mInt->SetSetCommand("SetHeight");
  mInt->SetGetCommand("GetHeight");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the height of the cone");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Capping");
  mInt->SetSetCommand("SetCapping");
  mInt->SetGetCommand("GetCapping");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Set whether to draw the base of the cone");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Cube ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCubeSource");
  sInt->SetRootName("Cube");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("XLength");
  mInt->SetSetCommand("SetXLength");
  mInt->SetGetCommand("GetXLength");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The length of the cube in the x direction.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("YLength");
  mInt->SetSetCommand("SetYLength");
  mInt->SetGetCommand("GetYLength");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The length of the cube in the y direction.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ZLength");
  mInt->SetSetCommand("SetZLength");
  mInt->SetGetCommand("GetZLength");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The length of the cube in the z direction.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Center");
  mInt->SetSetCommand("SetCenter");
  mInt->SetGetCommand("GetCenter");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the center of the cube.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Cylinder ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCylinderSource");
  sInt->SetRootName("Cyl");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Resolution");
  mInt->SetSetCommand("SetResolution");
  mInt->SetGetCommand("GetResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The number of facets used to define the cylinder.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Height");
  mInt->SetSetCommand("SetHeight");
  mInt->SetGetCommand("GetHeight");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The height of the cylinder (along the y axis).");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The radius of the cylinder.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Center");
  mInt->SetSetCommand("SetCenter");
  mInt->SetGetCommand("GetCenter");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the center of the cylinder.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Capping");
  mInt->SetSetCommand("SetCapping");
  mInt->SetGetCommand("GetCapping");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Set whether to draw the ends of the cylinder");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

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
  mInt->SetBalloonHelp("Set the min and max values of the data in each dimension");
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
  mInt->SetBalloonHelp("Choose which axes of the data set to display");
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
  mInt->SetBalloonHelp("Set the imaginary and real values for C (constant) and X (initial value)");
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
  mInt->SetBalloonHelp("Set the inaginary and real values for the spacing for C (constant) and X (initial value)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ImageReader ----
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageReader");
  sInt->SetRootName("ImageRead");
  sInt->SetOutputClassName("vtkImageData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FilePrefix");
  mInt->SetSetCommand("SetFilePrefix");
  mInt->SetGetCommand("GetFilePrefix");
  mInt->AddStringArgument();
  mInt->SetBalloonHelp("Set the prefix for the files for this image data.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ScalarType");
  mInt->SetSetCommand("SetDataScalarType");
  mInt->SetGetCommand("GetDataScalarType");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the scalar type for the data: unsigned char (3), short (4), unsigned short (5), int (6), float (10), double(11)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Extent");
  mInt->SetSetCommand("SetDataExtent");
  mInt->SetGetCommand("GetDataExtent");
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the min and max values of the data in each dimension");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;  
  
  // ---- OutlineCornerSource ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkOutlineCornerSource");
  sInt->SetRootName("Corners");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Bounds");
  mInt->SetSetCommand("SetBounds");
  mInt->SetGetCommand("GetBounds");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Bounds of the outline.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("CornerFactor");
  mInt->SetSetCommand("SetCornerFactor");
  mInt->SetGetCommand("GetCornerFactor");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The relative size of the corners to the length of the corresponding bounds. (0.001 -> 0.5)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

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
  mInt->SetBalloonHelp("Select the file for the data set");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the radius of the data set");
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
  mInt->SetBalloonHelp("Set the coordinates for the center of the sphere.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();  
  mInt->SetBalloonHelp("Set the radius of the sphere");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Theta Resolution");
  mInt->SetSetCommand("SetThetaResolution");
  mInt->SetGetCommand("GetThetaResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the number of points in the longitude direction (ranging from Start Theta to End Theta)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Start Theta");
  mInt->SetSetCommand("SetStartTheta");
  mInt->SetGetCommand("GetStartTheta");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the starting angle in the longitude direction");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("End Theta");
  mInt->SetSetCommand("SetEndTheta");
  mInt->SetGetCommand("GetEndTheta");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the ending angle in the longitude direction");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Phi Resolution");
  mInt->SetSetCommand("SetPhiResolution");
  mInt->SetGetCommand("GetPhiResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the number of points in the latitude direction (ranging from Start Phi to End Phi)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Start Phi");
  mInt->SetSetCommand("SetStartPhi");
  mInt->SetGetCommand("GetStartPhi");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the starting angle in the latitude direction");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("End Phi");
  mInt->SetSetCommand("SetEndPhi");
  mInt->SetGetCommand("GetEndPhi");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the ending angle in the latitude direction");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

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
  mInt->SetBalloonHelp("Select the data file for the STL data set");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- SupderQuadricSource ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSuperquadricSource");
  sInt->SetRootName("SQuad");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Center");
  mInt->SetSetCommand("SetCenter");
  mInt->SetGetCommand("GetCenter");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the center of the superquadric");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Scale");
  mInt->SetSetCommand("SetScale");
  mInt->SetGetCommand("GetScale");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the scale of the superquadric");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ThetaResolution");
  mInt->SetSetCommand("SetThetaResolution");
  mInt->SetGetCommand("GetThetaResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The number of points in the longitude direction.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("PhiResolution");
  mInt->SetSetCommand("SetPhiResolution");
  mInt->SetGetCommand("GetPhiResolution");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The number of points in the latitude direction.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Thickness");
  mInt->SetSetCommand("SetThickness");
  mInt->SetGetCommand("GetThickness");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Changing thickness maintains the outside diameter of the toroid.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ThetaRoundness");
  mInt->SetSetCommand("SetThetaRoundness");
  mInt->SetGetCommand("GetThetaRoundness");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Values range from 0 (rectangular) to 1 (circular) to higher order.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("PhiRoundness");
  mInt->SetSetCommand("SetPhiRoundness");
  mInt->SetGetCommand("GetPhiRoundness");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Values range from 0 (rectangular) to 1 (circular) to higher order.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Size");
  mInt->SetSetCommand("SetSize");
  mInt->SetGetCommand("GetSize");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Isotropic size");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Toroidal");
  mInt->SetSetCommand("SetToroidal");
  mInt->SetGetCommand("GetToroidal");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Whether or not the superquadric is toroidal or ellipsoidal");
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
  mInt->SetBalloonHelp("Enter the text to display");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // Filters

  // ---- BrownianPoints ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkBrownianPoints");
  sInt->SetRootName("BPts");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("MinimumSpeed");
  mInt->SetSetCommand("SetMinimumSpeed");
  mInt->SetGetCommand("GetMinimumSpeed");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The minimum size of the random point vectors generated.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("MaximumSpeed");
  mInt->SetSetCommand("SetMaximumSpeed");
  mInt->SetGetCommand("GetMaximumSpeed");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The maximum size of the random point vectors generated.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ButterflySubdivisionFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkButterflySubdivisionFilter");
  sInt->SetRootName("BFSubDiv");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfSubdivisions");
  mInt->SetSetCommand("SetNumberOfSubdivisions");
  mInt->SetGetCommand("GetNumberOfSubdivisions");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Each subdivision changes single triangles into four triangles..");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- CellCenters ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCellCenters");
  sInt->SetRootName("Centers");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("VertexCells");
  mInt->SetSetCommand("SetVertexCells");
  mInt->SetGetCommand("GetVertexCells");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Generate vertex as geometry of just points.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- CellDataToPointData ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCellDataToPointData");
  sInt->SetRootName("CellToPoint");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("PassCellData");
  mInt->SetSetCommand("SetPassCellData");
  mInt->SetGetCommand("GetPassCellData");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp(" Control whether the input cell data is to be passed to the output. If on, then the input cell data is passed through to the output; otherwise, only generated point data is placed into the output.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- CellDerivatives ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCellDerivatives");
  sInt->SetRootName("CellDeriv");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("VectorMode");
  mInt->SetSetCommand("SetVectorMode");
  mInt->SetGetCommand("GetVectorMode");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "Pass");
  mInt->AddSelectionEntry(1, "Vorticity");
  mInt->AddSelectionEntry(2, "Gradient");
  mInt->SetBalloonHelp("Value to put in vector cell data");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("TensorMode");
  mInt->SetSetCommand("SetTensorMode");
  mInt->SetGetCommand("GetTensorMode");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "Pass");
  mInt->AddSelectionEntry(1, "Strain");
  mInt->AddSelectionEntry(2, "Gradient");
  mInt->SetBalloonHelp("Value to put in tensaor cell data");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- CleanPolyData ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkCleanPolyData");
  sInt->SetRootName("CleanPD");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;
    
  // ---- ClipPlane ----.
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
  mInt->SetBalloonHelp("Set the origin of the clipping plane");
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
  mInt->SetBalloonHelp("Set the normal of the clipping plane");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetValue");
  mInt->SetGetCommand("GetValue");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the clipping value of the implicit plane");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("InsideOut");
  mInt->SetSetCommand("SetInsideOut");
  mInt->SetGetCommand("GetInsideOut");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select which \"side\" of the data set to clip away");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GenerateClipScalars");
  mInt->SetSetCommand("SetGenerateClipScalars");
  mInt->SetGetCommand("GetGenerateClipScalars");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("If this flag is enabled, then the output scalar values will be interpolated from the implicit function values, and not the input scalar data");
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
  sInt->SetSourceClassName("vtkClipDataSet");
  sInt->SetRootName("ClipDS");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkUnstructuredGrid");
  sInt->DefaultScalarsOn();
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetValue");
  mInt->SetGetCommand("GetValue");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the scalar value to clip by");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("InsideOut");
  mInt->SetSetCommand("SetInsideOut");
  mInt->SetGetCommand("GetInsideOut");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select which \"side\" of the data set to clip away");
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
  sInt->DefaultScalarsOn();
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetValue");
  mInt->SetGetCommand("GetValue");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the scalar value to clip by");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("InsideOut");
  mInt->SetSetCommand("SetInsideOut");
  mInt->SetGetCommand("GetInsideOut");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select which \"side\" of the data set to clip away");
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
  mInt->SetBalloonHelp("Enter the array name of the cell array containing the material values");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Material");
  mInt->SetSetCommand("SetMaterial");
  mInt->SetGetCommand("GetMaterial");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the value of the material to probe");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Array");
  mInt->SetSetCommand("SetArrayName");
  mInt->SetGetCommand("GetArrayName");
  mInt->AddStringArgument();
  mInt->SetBalloonHelp("Set the array name of the array to cut");
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
  mInt->SetBalloonHelp("Specify the normal vector of the plane to cut by");
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
  mInt->SetBalloonHelp("Set the x, y, z coordinates of the origin of the plane");
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
  mInt->SetBalloonHelp("Set the normal vector to the plane");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GenerateCutScalars");
  mInt->SetSetCommand("SetGenerateCutScalars");
  mInt->SetGetCommand("GetGenerateCutScalars");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to generate scalars from the implicit function values or from the input scalar data");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- DataSetSurfaceFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkDataSetSurfaceFilter");
  sInt->SetRootName("Surface");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- DecimatePro ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkDecimatePro");
  sInt->SetRootName("Deci");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("TargetReduction");
  mInt->SetSetCommand("SetTargetReduction");
  mInt->SetGetCommand("GetTargetReduction");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Desired reduction of the total number of triangles.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("PreserveTopology");
  mInt->SetSetCommand("SetPreserveTopology");
  mInt->SetGetCommand("GetPreserveTopology");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("If off, better reduction can occur, but model may break up.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FeatureAngle");
  mInt->SetSetCommand("SetFeatureAngle");
  mInt->SetGetCommand("GetFeatureAngle");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Topology can be split along features.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Delunay2D ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkDelaunay2D");
  sInt->SetRootName("Del2D");
  sInt->SetInputClassName("vtkPointSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Alpha");
  mInt->SetSetCommand("SetAlpha");
  mInt->SetGetCommand("GetAlpha");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Distance value to control output of this filter.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("BoundingTriangulation");
  mInt->SetSetCommand("SetBoundingTriangulation");
  mInt->SetGetCommand("GetBoundingTriangulation");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Include bounding triagulation or not.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Offset");
  mInt->SetSetCommand("SetOffset");
  mInt->SetGetCommand("GetOffset");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Multiplier for initial triagulation size.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Tolerance");
  mInt->SetSetCommand("SetTolerance");
  mInt->SetGetCommand("GetTolerance");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("A value 0->1 for merginge close points.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Delunay3D ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkDelaunay3D");
  sInt->SetRootName("Del3D");
  sInt->SetInputClassName("vtkPointSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Alpha");
  mInt->SetSetCommand("SetAlpha");
  mInt->SetGetCommand("GetAlpha");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Distance value to control output of this filter.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("BoundingTriangulation");
  mInt->SetSetCommand("SetBoundingTriangulation");
  mInt->SetGetCommand("GetBoundingTriangulation");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Include bounding triagulation or not.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Offset");
  mInt->SetSetCommand("SetOffset");
  mInt->SetGetCommand("GetOffset");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Multiplier for initial triagulation size.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Tolerance");
  mInt->SetSetCommand("SetTolerance");
  mInt->SetGetCommand("GetTolerance");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("A value 0->1 for merginge close points.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ElevationFilter ----.
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
  mInt->SetBalloonHelp("Set the minimum point for the elevation");
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
  mInt->SetBalloonHelp("Set the maximum point for the elevation");
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
  mInt->SetBalloonHelp("Set the range of scalar values to generate");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

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

  // ---- ExtractGrid ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkExtractGrid");
  sInt->SetRootName("ExtractGrid");
  sInt->SetInputClassName("vtkStructuredGrid");
  sInt->SetOutputClassName("vtkStructuredGrid");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("VOI");
  mInt->SetSetCommand("SetVOI");
  mInt->SetGetCommand("GetVOI");
  mInt->SetWidgetTypeToExtent();
  mInt->SetBalloonHelp("Set the min/max values of the volume of interest (VOI)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("SampleRate");
  mInt->SetSetCommand("SetSampleRate");
  mInt->SetGetCommand("GetSampleRate");
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the sampling rate for each dimension");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("IncludeBoundary");
  mInt->SetSetCommand("SetIncludeBoundary");
  mInt->SetGetCommand("GetIncludeBoundary");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to always include the boundary of the grid in the output");
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
  mInt->SetBalloonHelp("Select whether to generate ghost cells");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ExtractUnstructuredGridPiece ----.
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
  mInt->SetBalloonHelp("Select whether to generate ghost cells");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ImageClip ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageClip");
  sInt->SetRootName("ImageClip");
  sInt->SetInputClassName("vtkImageData");
  sInt->SetOutputClassName("vtkImageData");
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("OutputWholeExtent");
  mInt->SetSetCommand("SetOutputWholeExtent");
  mInt->SetGetCommand("GetOutputWholeExtent");
  mInt->SetWidgetTypeToExtent();
  mInt->SetBalloonHelp("Set the min/max extents in each dimension of the output");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ImageContinuousDilate ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageContinuousDilate3D");
  sInt->SetRootName("ContDilate");
  sInt->SetInputClassName("vtkImageData");
  sInt->SetOutputClassName("vtkImageData");
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("KernelSize");
  mInt->SetSetCommand("SetKernelSize");
  mInt->SetGetCommand("GetKernelSize");
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Radius of elliptical footprint.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ImageContinuousErode ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageContinuousErode3D");
  sInt->SetRootName("ContErode");
  sInt->SetInputClassName("vtkImageData");
  sInt->SetOutputClassName("vtkImageData");
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("KernelSize");
  mInt->SetSetCommand("SetKernelSize");
  mInt->SetGetCommand("GetKernelSize");
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Radius of elliptical footprint.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;
  
  // ---- ImageGradient ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageGradient");
  sInt->SetRootName("ImageGradient");
  sInt->SetInputClassName("vtkImageData");
  sInt->SetOutputClassName("vtkImageData");
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Dimensionality");
  mInt->SetSetCommand("SetDimensionality");
  mInt->SetGetCommand("GetDimensionality");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "2");
  mInt->AddSelectionEntry(1, "3");
  mInt->SetBalloonHelp("Select whether to perform a 2d or 3d gradient");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;
  
  // ---- ImageSmooth ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkImageGaussianSmooth");
  sInt->SetRootName("GaussSmooth");
  sInt->SetInputClassName("vtkImageData");
  sInt->SetOutputClassName("vtkImageData");
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("StandardDeviations");
  mInt->SetSetCommand("SetStandardDeviations");
  mInt->SetGetCommand("GetStandardDeviations");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("STD for x, y and z axes.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("RadiusFactors");
  mInt->SetSetCommand("SetRadiusFactors");
  mInt->SetGetCommand("GetRadiusFactors");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Kernel size is this factor times STD.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  //Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Dimensionality");
  mInt->SetSetCommand("SetDimensionality");
  mInt->SetGetCommand("GetDimensionality");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "2");
  mInt->AddSelectionEntry(1, "3");
  mInt->SetBalloonHelp("Select whether to perform a 2d or 3d gradient");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;
  
  // ---- InhibitPoints ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkInhibitPoints");
  sInt->SetRootName("InhibPts");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  sInt->DefaultVectorsOn();
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Scale");
  mInt->SetSetCommand("SetScale");
  mInt->SetGetCommand("GetScale");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the size of the inihibition neighborhood.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("MagnitudeThreshold");
  mInt->SetSetCommand("SetMagnitudeThreshold");
  mInt->SetGetCommand("GetMagnitudeThreshold");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Vectors smaller that this are not considered.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GenerateVertices");
  mInt->SetSetCommand("SetGenerateVertices");
  mInt->SetGetCommand("GetGenerateVertices");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Flag to create vertex cells/");
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
  mInt->SetBalloonHelp("Select whether to draw endcaps");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ScaleFactor");
  mInt->SetSetCommand("SetScaleFactor");
  mInt->SetGetCommand("GetScaleFactor");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the extrusion scale factor");
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
  mInt->SetBalloonHelp("Set the direction for the extrusion");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- LinearSubdivisionFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkLinearSubdivisionFilter");
  sInt->SetRootName("LinSubDiv");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfSubdivisions");
  mInt->SetSetCommand("SetNumberOfSubdivisions");
  mInt->SetGetCommand("GetNumberOfSubdivisions");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Each subdivision changes single triangles into four triangles.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- LoopSubdivisionFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkLoopSubdivisionFilter");
  sInt->SetRootName("LoopSubDiv");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfSubdivisions");
  mInt->SetSetCommand("SetNumberOfSubdivisions");
  mInt->SetGetCommand("GetNumberOfSubdivisions");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Each subdivision changes single triangles into four triangles.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- MaskPoints ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkMaskPoints");
  sInt->SetRootName("MaskPts");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("OnRatio");
  mInt->SetSetCommand("SetOnRatio");
  mInt->SetGetCommand("GetOnRatio");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("The ratio of points retained.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("MaxPoints");
  mInt->SetSetCommand("SetMaximumNumberOfPoints");
  mInt->SetGetCommand("GetMaximumNumberOfPoints");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Limit the number of points.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Offset");
  mInt->SetSetCommand("SetOffset");
  mInt->SetGetCommand("GetOffset");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Start with this point.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Random");
  mInt->SetSetCommand("SetRandomMode");
  mInt->SetGetCommand("GetRandomMode");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to randomly select points, or subsample regularly.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("GenerateVertices");
  mInt->SetSetCommand("SetGenerateVertices");
  mInt->SetGetCommand("GetGenerateVertices");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Convienience feature to display points.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- OBB Dicer ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkOBBDicer");
  sInt->SetRootName("Dicer");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FieldData");
  mInt->SetSetCommand("SetFieldData");
  mInt->SetGetCommand("GetFieldData");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Generate point scalars or a field array.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfPointsPerPiece");
  mInt->SetSetCommand("SetNumberOfPointsPerPiece");
  mInt->SetGetCommand("GetNumberOfPointsPerPiece");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Controls piece size.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfPieces");
  mInt->SetSetCommand("SetNumberOfPieces");
  mInt->SetGetCommand("GetNumberOfPieces");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Controls number of pieces generated.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- OutlineCornerFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkOutlineCornerFilter");
  sInt->SetRootName("COutline");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("CornerFactor");
  mInt->SetSetCommand("SetCornerFactor");
  mInt->SetGetCommand("GetCornerFactor");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The relative size of the corners to the length of the corresponding bounds. (0.001 -> 0.5)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- PieceScalars ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPieceScalars");
  sInt->SetRootName("ColorPieces");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Random");
  mInt->SetSetCommand("SetRandomMode");
  mInt->SetGetCommand("GetRandomMode");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to use random colors for the various pieces");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- PointDataToCellData ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPointDataToCellData");
  sInt->SetRootName("PtToCell");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkDataSet");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("PassPointData");
  mInt->SetSetCommand("SetPassPointData");
  mInt->SetGetCommand("GetPassPointData");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- PolyDataNormals ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPolyDataNormals");
  sInt->SetRootName("PDNormals");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FeatureAngle");
  mInt->SetSetCommand("SetFeatureAngle");
  mInt->SetGetCommand("GetFeatureAngle");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Points are duplicated along features over this angle (0->180)");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Splitting");
  mInt->SetSetCommand("SetSplitting");
  mInt->SetGetCommand("GetSplitting");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off the splitting of sharp edges.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Consistency");
  mInt->SetSetCommand("SetConsistency");
  mInt->SetGetCommand("GetConsistency");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off the enforcement of consistent polygon ordering.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Consistency");
  mInt->SetSetCommand("SetConsistency");
  mInt->SetGetCommand("GetConsistency");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off the enforcement of consistent polygon ordering.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("CellNormals");
  mInt->SetSetCommand("SetComputeCellNormals");
  mInt->SetGetCommand("GetComputeCellNormals");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off the computation of cell normals.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Consistency");
  mInt->SetSetCommand("SetConsistency");
  mInt->SetGetCommand("GetConsistency");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off the enforcement of consistent polygon ordering.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("FlipNormals");
  mInt->SetSetCommand("SetFlipNormals");
  mInt->SetGetCommand("GetFlipNormals");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Flipping reverves the meaning of front and back.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NonManifold");
  mInt->SetSetCommand("SetNonManifoldTraversal");
  mInt->SetGetCommand("GetNonManifoldTraversal");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off traversal across non-manifold edges. This will prevent problems where the consistency of polygonal ordering is corrupted due to topological loops.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- PolyDataStreamer ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkPolyDataStreamer");
  sInt->SetRootName("Stream");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfDivisions");
  mInt->SetSetCommand("SetNumberOfStreamDivisions");
  mInt->SetGetCommand("GetNumberOfStreamDivisions");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("Set the number of pieces to divide the data set into for streaming");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- ReverseSense ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkReverseSense");
  sInt->SetRootName("RevSense");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("CellOrder");
  mInt->SetSetCommand("SetReverseCells");
  mInt->SetGetCommand("GetReverseCells");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ReverseNormals");
  mInt->SetSetCommand("SetReverseNormals");
  mInt->SetGetCommand("GetReverseNormals");
  mInt->SetWidgetTypeToToggle();
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- RibbonFilter ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkRibbonFilter");
  sInt->SetRootName("Ribbon");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Width");
  mInt->SetSetCommand("SetWidth");
  mInt->SetGetCommand("GetWidth");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The half width of the ribbon (or minimum).");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Angle");
  mInt->SetSetCommand("SetAngle");
  mInt->SetGetCommand("GetAngle");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The offset angle of the ribbon from the line normal (0->360).");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("DefaultNormal");
  mInt->SetSetCommand("SetDefaultNormal");
  mInt->SetGetCommand("GetDefaultNormal");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("If no normals are supplied");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("VaryWidth");
  mInt->SetSetCommand("SetVaryWidth");
  mInt->SetGetCommand("GetVaryWidth");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Turn on/off the variation of ribbon width with scalar value.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- RotationalExtrusion ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkRotationalExtrusionFilter");
  sInt->SetRootName("RotExtrude");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Resoultion");
  mInt->SetSetCommand("SetResolution");
  mInt->SetGetCommand("GetResolution");
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
  mInt->SetVariableName("Angle");
  mInt->SetSetCommand("SetAngle");
  mInt->SetGetCommand("GetAngle");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the angle of rotation.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Translation");
  mInt->SetSetCommand("SetTranslation");
  mInt->SetGetCommand("GetTranslation");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The total amount of translation along the z-axis.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("DeltaRadius");
  mInt->SetSetCommand("SetDeltaRadius");
  mInt->SetGetCommand("GetDeltaRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The change in radius during sweep process.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL; 
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Quadric Clustering ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkQuadricClustering");
  sInt->SetRootName("QC");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Spacing");
  mInt->SetSetCommand("SetDivisionSpacing");
  mInt->SetGetCommand("GetDivisionSpacing");
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->AddFloatArgument();  
  mInt->SetBalloonHelp("Set the spacing of the bins in each dimension");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("UseInputPoints");
  mInt->SetSetCommand("SetUseInputPoints");
  mInt->SetGetCommand("GetUseInputPoints");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to use points from the input in the output or to calculate optimum representative points for each bin");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("UseFeatureEdges");
  mInt->SetSetCommand("SetUseFeatureEdges");
  mInt->SetGetCommand("GetUseFeatureEdges");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to use feature edge quadrics to match up the boundaries between pieces");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("UseFeaturePoints");
  mInt->SetSetCommand("SetUseFeaturePoints");
  mInt->SetGetCommand("GetUseFeaturePoints");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to use feature point quadrics to align piece boundaries");
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
  mInt->SetBalloonHelp("Set the amount to shrink by");
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
  mInt->SetVariableName("Attribute");
  mInt->SetSetCommand("SetAttribute");
  mInt->SetGetCommand("GetAttribute");
  mInt->SetWidgetTypeToSelection();
  mInt->AddSelectionEntry(0, "Scalars");
  mInt->AddSelectionEntry(1, "Vectors");
  mInt->SetBalloonHelp("Select whether the array contains scalars or vectors");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Field Name");
  mInt->SetSetCommand("SetFieldName");
  mInt->SetGetCommand("GetFieldName");
  mInt->AddStringArgument();
  mInt->SetBalloonHelp("Set the name of the array containing the data to operate on");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- SingleContour ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSingleContourFilter");
  sInt->SetRootName("SingleContour");
  sInt->SetInputClassName("vtkDataSet");
  sInt->SetOutputClassName("vtkPolyData");
  sInt->DefaultScalarsOn();
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Value");
  mInt->SetSetCommand("SetFirstValue");
  mInt->SetGetCommand("GetFirstValue");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the contour value");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Normals");
  mInt->SetSetCommand("SetComputeNormals");
  mInt->SetGetCommand("GetComputeNormals");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to compute normals");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Gradients");
  mInt->SetSetCommand("SetComputeGradients");
  mInt->SetGetCommand("GetComputeGradients");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to compute gradients");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Scalars");
  mInt->SetSetCommand("SetComputeScalars");
  mInt->SetGetCommand("GetComputeScalars");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to compute scalars");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;
  
  // ---- Smooth ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkSmoothPolyDataFilter");
  sInt->SetRootName("Smooth");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("NumberOfIterations");
  mInt->SetSetCommand("SetNumberOfIterations");
  mInt->SetGetCommand("GetNumberOfIterations");
  mInt->AddIntegerArgument();
  mInt->SetBalloonHelp("More iterations produces better smoothing.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Convergence");
  mInt->SetSetCommand("SetConvergence");
  mInt->SetGetCommand("GetConvergence");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Smooting factor (0->1).");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Geometry ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkStructuredGridGeometryFilter");
  sInt->SetRootName("GridGeom");
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
  mInt->SetBalloonHelp("Set the min/max extents of the grid");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;

  // ---- Triangle ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkTriangleFilter");
  sInt->SetRootName("Tri");
  sInt->SetInputClassName("vtkPolyData");
  sInt->SetOutputClassName("vtkPolyData");
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
  mInt->SetBalloonHelp("Set the number of sides for the tube");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Capping");
  mInt->SetSetCommand("SetCapping");
  mInt->SetGetCommand("GetCapping");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Select whether to draw endcaps on the tube");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Radius");
  mInt->SetSetCommand("SetRadius");
  mInt->SetGetCommand("GetRadius");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the radius of the tube");
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
  mInt->SetBalloonHelp("Select whether/how to vary the radius of the tube");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("RadiusFactor");
  mInt->SetSetCommand("SetRadiusFactor");
  mInt->SetGetCommand("GetRadiusFactor");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Set the maximum tube radius in terms of a multiple of the minimum radius");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;  

  // ---- WarpScalar ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkWarpScalar");
  sInt->SetRootName("WarpS");
  sInt->DefaultScalarsOn();
  sInt->SetInputClassName("vtkPointSet");
  sInt->SetOutputClassName("vtkPointSet");
  sInt->DefaultScalarsOn();
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ScaleFactor");
  mInt->SetSetCommand("SetScaleFactor");
  mInt->SetGetCommand("GetScaleFactor");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Displacement is vector times scale.");
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
  mInt->SetBalloonHelp("Warp direction.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("UseNormal");
  mInt->SetSetCommand("SetUseNormal");
  mInt->SetGetCommand("GetUseNormal");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Use instance model normals rather than instance normal.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("XYPlane");
  mInt->SetSetCommand("SetXYPlane");
  mInt->SetGetCommand("GetXYPlane");
  mInt->SetWidgetTypeToToggle();
  mInt->SetBalloonHelp("Z value is used to warp the surface, scalars to color surface.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;  
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;  

  // ---- WarpVector ----.
  sInt = vtkPVSourceInterface::New();
  sInt->SetApplication(pvApp);
  sInt->SetPVWindow(this);
  sInt->SetSourceClassName("vtkWarpVector");
  sInt->SetRootName("WarpV");
  sInt->SetInputClassName("vtkPointSet");
  sInt->SetOutputClassName("vtkPointSet");
  sInt->DefaultVectorsOn();
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ScaleFactor");
  mInt->SetSetCommand("SetScaleFactor");
  mInt->SetGetCommand("GetScaleFactor");
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("Displacement is vector times scale.");
  sInt->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Add it to the list.
  this->SourceInterfaces->AddItem(sInt);
  sInt->Delete();
  sInt = NULL;  
}

