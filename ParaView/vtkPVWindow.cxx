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
#include "vtkToolkits.h"

#include "vtkInteractorStylePlaneSource.h"

#include "vtkDirectory.h"
#include "vtkMath.h"

#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVActorComposite.h"

#include "vtkPVSourceInterface.h"
#include "vtkPVSourceInterfaceParser.h"
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

#include "vtkPVSourceInterfaceDirectories.h"

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

  // Allow the user to interactively resize the properties parent.
  this->MiddleFrame->SetSeparatorWidth(6);
  this->MiddleFrame->SetFrame1MinimumWidth(5);
  this->MiddleFrame->SetFrame1Width(360);
  this->MiddleFrame->SetFrame2MinimumWidth(200);
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
#ifdef VTK_USE_PATENTED
  s = (vtkSource *)(pvApp->MakeTclObject("vtkKitwareContourFilter", tclName));
#else
  s = (vtkSource *)(pvApp->MakeTclObject("vtkContourFilter", tclName));
#endif
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

  pvApp->BroadcastScript("%s SetSpatialMatch 2", tclName);
  
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
void vtkPVWindow::ReadSourceInterfaces()
{
  // Add special sources.
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVSourceInterface *sInt;  

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
  
  // A list of standard directories in which to find interfaces.  The
  // first directory in this list that is found is the only one used.
  static const char* standardDirectories[] =
    {
#ifdef vtkPV_SOURCE_CONFIG_DIR
      vtkPV_SOURCE_CONFIG_DIR,
#endif
#ifdef vtkPV_INSTALL_CONFIG_DIR
      vtkPV_INSTALL_CONFIG_DIR,
#endif
      0
    };
  
  // Parse input files from the first directory found to exist.
  int found=0;
  for(const char** dir=standardDirectories; !found && *dir; ++dir)
    {
    found = this->ReadSourceInterfacesFromDirectory(*dir);
    }
  if(!found)
    {
    vtkWarningMacro("Could not find any directories for standard interface files.");
    }
}

//----------------------------------------------------------------------------
void vtkPVWindow::ReadSourceInterfacesFromFile(const char* file)
{
  vtkPVSourceInterfaceParser *parser = vtkPVSourceInterfaceParser::New();
  parser->SetFileName(file);
  parser->SetPVApplication(this->GetPVApplication());
  parser->SetPVWindow(this);
  parser->SetSourceInterfaces(this->SourceInterfaces);
  parser->ParseFile();  
  parser->Delete();
}

//----------------------------------------------------------------------------
// Walk through the list of .xml files in the given directory and
// parse each one for sources and filters.  Returns whether the
// directory was found.
int vtkPVWindow::ReadSourceInterfacesFromDirectory(const char* directory)
{
  vtkDirectory* dir = vtkDirectory::New();
  if(!dir->Open(directory))
    {
    return 0;
    }
  
  for(int i=0; i < dir->GetNumberOfFiles(); ++i)
    {
    const char* file = dir->GetFile(i);
    int extPos = strlen(file)-4;
    
    // Look for the ".xml" extension.
    if((extPos > 0) && (strcmp(file+extPos, ".xml") == 0))
      {
      char* fullPath = new char[strlen(file)+strlen(directory)+2];
      strcpy(fullPath, directory);
      strcat(fullPath, "/");
      strcat(fullPath, file);
      
      this->ReadSourceInterfacesFromFile(fullPath);
      
      delete [] fullPath;
      }
    }
  
  dir->Delete();
  return 1;
}
