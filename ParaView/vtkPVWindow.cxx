/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkPVCutPlane.h"
#include "vtkPVClipPlane.h"
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
#include "vtkPVAnimationInterface.h"

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
  vtkPVMethodInterface *mInt;

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
  this->CutPlaneButton = vtkKWPushButton::New();
  this->ClipPlaneButton = vtkKWPushButton::New();
  this->ThresholdButton = vtkKWPushButton::New();
  this->ContourButton = vtkKWPushButton::New();
  this->GlyphButton = vtkKWPushButton::New();
  this->ProbeButton = vtkKWPushButton::New();

  this->FrameRateLabel = vtkKWLabel::New();
  this->FrameRateScale = vtkKWScale::New();

  this->ReductionCheck = vtkKWCheckButton::New();
  
  this->Sources = vtkKWCompositeCollection::New();
  this->GlyphSources = vtkKWCompositeCollection::New();
  
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

  // Frame used for animations.
  this->AnimationInterface = vtkPVAnimationInterface::New();

  // Special filters need interfaces also.
  // For now this is just for animations,
  // but should also be for serializing the filters.
  this->ThresholdInterface = vtkPVSourceInterface::New();
  //this->ThresholdInterface->SetApplication(???):
  this->ThresholdInterface->SetPVWindow(this);
  this->ThresholdInterface->SetSourceClassName("vtkThreshold");
  this->ThresholdInterface->SetRootName("Threshold");
  this->ThresholdInterface->SetInputClassName("vtkDataSet");
  this->ThresholdInterface->SetOutputClassName("vtkUnstructuredGrid");
  // Method
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("ThresholdRange");
  mInt->SetSetCommand("ThresholdBetween");
  // There is no simple get string.
  //mInt->SetGetCommand("");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->SetBalloonHelp("The range of the scalars to keep.");
  this->ThresholdInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;

  // I plan to offload the animation, reseting, synchrinization and saving
  // tasks of the interface to pvSourceWidgets.  Until then,
  // this is the easiest way to get CutPlane working is with an interface
  // (even though it is not created with an interface).
  // The threshold interface above is just an uncompleted experiment.
  this->CutPlaneInterface = vtkPVSourceInterface::New();
  //this->CutPlaneInterface->SetApplication(???):
  this->CutPlaneInterface->SetPVWindow(this);
  this->CutPlaneInterface->SetSourceClassName("vtkCutPlane");
  this->CutPlaneInterface->SetRootName("CutPlane");
  this->CutPlaneInterface->SetInputClassName("vtkDataSet");
  this->CutPlaneInterface->SetOutputClassName("vtkPolyData");
  // Offset:
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Offset");
  mInt->SetSetCommand("SetOffset");
  mInt->SetGetCommand("GetOffset");
  mInt->AddFloatArgument();
  this->CutPlaneInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Center:
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Origin");
  mInt->SetSetCommand("SetOrigin");
  mInt->SetGetCommand("GetOrigin");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  this->CutPlaneInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Normal:
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Normal");
  mInt->SetSetCommand("SetNormal");
  mInt->SetGetCommand("GetNormal");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  this->CutPlaneInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;


  // I plan to offload the animation, reseting, synchrinization and saving
  // tasks of the interface to pvSourceWidgets.  Until then,
  // this is the easiest way to get ClipPlane working is with an interface
  // (even though it is not created with an interface).
  // The threshold interface above is just an uncompleted experiment.
  this->ClipPlaneInterface = vtkPVSourceInterface::New();
  //this->ClipPlaneInterface->SetApplication(???):
  this->ClipPlaneInterface->SetPVWindow(this);
  this->ClipPlaneInterface->SetSourceClassName("vtkClipPlane");
  this->ClipPlaneInterface->SetRootName("ClipPlane");
  this->ClipPlaneInterface->SetInputClassName("vtkDataSet");
  this->ClipPlaneInterface->SetOutputClassName("vtkUnstructuredGrid");
  // Offset:
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Offset");
  mInt->SetSetCommand("SetOffset");
  mInt->SetGetCommand("GetOffset");
  mInt->AddFloatArgument();
  this->CutPlaneInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Center:
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Origin");
  mInt->SetSetCommand("SetOrigin");
  mInt->SetGetCommand("GetOrigin");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  this->CutPlaneInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;
  // Normal:
  mInt = vtkPVMethodInterface::New();
  mInt->SetVariableName("Normal");
  mInt->SetSetCommand("SetNormal");
  mInt->SetGetCommand("GetNormal");
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  mInt->AddFloatArgument();
  this->CutPlaneInterface->AddMethodInterface(mInt);
  mInt->Delete();
  mInt = NULL;


}

//----------------------------------------------------------------------------
vtkPVWindow::~vtkPVWindow()
{
  this->PrepareForDelete();

  this->Sources->Delete();
  this->Sources = NULL;

  this->GlyphSources->Delete();
  this->GlyphSources = NULL;
}

//----------------------------------------------------------------------------
void vtkPVWindow::PrepareForDelete()
{
  this->SetCurrentPVData(NULL);

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
  
  if (this->CutPlaneButton)
    {
    this->CutPlaneButton->Delete();
    this->CutPlaneButton = NULL;
    }
  
  if (this->ClipPlaneButton)
    {
    this->ClipPlaneButton->Delete();
    this->ClipPlaneButton = NULL;
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

  if (this->AnimationInterface)
    {
    this->AnimationInterface->Delete();
    this->AnimationInterface = NULL;
    }
  
  if (this->ThresholdInterface)
    {
    this->ThresholdInterface->Delete();
    this->ThresholdInterface = NULL;
    }
  
  if (this->CutPlaneInterface)
    {
    this->CutPlaneInterface->Delete();
    this->CutPlaneInterface = NULL;
    }
  
  if (this->ClipPlaneInterface)
    {
    this->ClipPlaneInterface->Delete();
    this->ClipPlaneInterface = NULL;
    }
  
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
  vtkPVSource *pvs;
  vtkPVData   *pvd;
  vtkSource   *s;
  vtkDataSet  *d;
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  vtkKWInteractor *interactor;
  vtkKWRadioButton *button;
  vtkKWWidget *pushButton;
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("vtkPVWindow::Create needs a vtkPVApplication.");
    return;
    }

  // invoke super method first
  this->vtkKWWindow::Create(pvApp,"");

  // We need an application before we can read the interface.
  this->ReadSourceInterfaces();
  
  this->Script("wm geometry %s 900x700+0+0",
                      this->GetWidgetName());
  
  // now add property options
  char *rbv = 
    this->GetMenuProperties()->CreateRadioButtonVariable(
      this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(2, "  Source",
                                            rbv, this,
                                            "ShowCurrentSourceProperties");
  delete [] rbv;

  rbv = this->GetMenuProperties()->CreateRadioButtonVariable(
           this->GetMenuProperties(),"Radio");
  this->GetMenuProperties()->AddRadioButton(3, "  Animation",
                                            rbv, this,
                                            "ShowAnimationProperties");
  delete [] rbv;


  // create the top level

  this->MenuFile->InsertCommand(0, "Open Data File", this, "OpenCallback");
  // Save current data in VTK format
  this->MenuFile->InsertCommand(1, "Save Data", this, "WriteData");
  
  this->MenuFile->InsertCommand(2, "Export Tcl Script", this, "SaveInTclScript");
  //this->MenuFile->InsertCommand(3, "Save Workspace", this, "SaveWorkspace");
  
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
  this->CalculatorButton->Create(app, "-image PVCalculatorButton");
  this->CalculatorButton->SetCommand(this, "CalculatorCallback");
  this->CalculatorButton->SetBalloonHelpString("Calculator");
  
  this->CutPlaneButton->SetParent(this->Toolbar);
  this->CutPlaneButton->Create(app, "-image PVCutPlaneButton");
  this->CutPlaneButton->SetCommand(this, "CutPlaneCallback");
  this->CutPlaneButton->SetBalloonHelpString("Cut with an implicit plane. It is identical to generating point scalars from an implicit plane, and taking an iso surface. This filter typically reduces the dimensionality of the data.  A 3D input data set will produce an 2D output plane.");

  this->ClipPlaneButton->SetParent(this->Toolbar);
  //this->ClipPlaneButton->Create(app, "-text Clip");
  this->ClipPlaneButton->Create(app, "-image PVClipPlaneButton");
  this->ClipPlaneButton->SetCommand(this, "ClipPlaneCallback");
  this->ClipPlaneButton->SetBalloonHelpString("Clip with an implicit plane.  Takes a portion of the data set away and does not reduce the dimensionality of the data set.  A 3d input data set will produce a 3d output data set.");

  this->ThresholdButton->SetParent(this->Toolbar);
  this->ThresholdButton->Create(app, "-image PVThresholdButton");
  this->ThresholdButton->SetCommand(this, "ThresholdCallback");
  this->ThresholdButton->SetBalloonHelpString("Threshold");
  
  this->ContourButton->SetParent(this->Toolbar);
  this->ContourButton->Create(app, "-image PVContourButton");
  this->ContourButton->SetCommand(this, "ContourCallback");
  this->ContourButton->SetBalloonHelpString("Contour");

  this->GlyphButton->SetParent(this->Toolbar);
//  this->GlyphButton->Create(app, "-text Glyph");
  this->GlyphButton->Create(app, "-image PVGlyphButton");
  this->GlyphButton->SetCommand(this, "GlyphCallback");
  this->GlyphButton->SetBalloonHelpString("Glyph");

  this->ProbeButton->SetParent(this->Toolbar);
  this->ProbeButton->Create(app, "-image PVProbeButton");
  this->ProbeButton->SetCommand(this, "ProbeCallback");
  this->ProbeButton->SetBalloonHelpString("Probe");
  
  this->Script("pack %s %s %s %s %s %s %s -side left -pady 0 -fill none -expand no",
               this->CalculatorButton->GetWidgetName(),
               this->CutPlaneButton->GetWidgetName(),
               this->ClipPlaneButton->GetWidgetName(),
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
  this->CreateMainView(pvApp);

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

  this->SelectPointInteractor->SetRenderView(this->GetMainView());
  
  this->Script("%s SetInteractor %s", this->GetMainView()->GetTclName(),
               this->RotateCameraInteractor->GetTclName());
  this->RotateCameraInteractor->SetCenter(0.0, 0.0, 0.0);
  this->MainView->ResetCamera();

  this->AnimationInterface->SetWindow(this);
  this->AnimationInterface->SetView(this->GetMainView());
  this->AnimationInterface->SetParent(this->MainView->GetPropertiesParent());
  this->AnimationInterface->Create(app, "-bd 2 -relief raised");

  this->Script( "wm deiconify %s", this->GetWidgetName());  

  // Create the sources that can be used for glyphing.
  // ===== Arrow
  s = (vtkSource *)(pvApp->MakeTclObject("vtkArrowSource", "pvGlyphArrow"));
  pvs = vtkPVSource::New();
  pvs->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetVTKSource(s, "pvGlyphArrow");
  pvs->SetName("Arrow");
  //pvs->SetInterface(this->CutPlaneInterface);
  //pvs->CreateProperties();
  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  // Create the object through tcl on all processes.
  d = (vtkDataSet *)(pvApp->MakeTclObject("vtkPolyData", "pvGlyphArrowOutput"));
  pvd->SetVTKData(d, "pvGlyphArrowOutput");
  // Connect the source and data.
  pvs->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", pvs->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  this->GlyphSources->AddItem(pvs);
  pvs->Delete();
  pvd->Delete();
  // ===== Cone
  s = (vtkSource *)(pvApp->MakeTclObject("vtkConeSource", "pvGlyphCone"));
  pvs = vtkPVSource::New();
  pvs->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetVTKSource(s, "pvGlyphCone");
  pvs->SetName("Cone");
  //pvs->SetInterface(this->CutPlaneInterface);
  //pvs->CreateProperties();
  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  // Create the object through tcl on all processes.
  d = (vtkDataSet *)(pvApp->MakeTclObject("vtkPolyData", "pvGlyphConeOutput"));
  pvd->SetVTKData(d, "pvGlyphConeOutput");
  // Connect the source and data.
  pvs->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", pvs->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  this->GlyphSources->AddItem(pvs);
  pvs->Delete();
  pvd->Delete();
  // ===== Sphere
  s = (vtkSource *)(pvApp->MakeTclObject("vtkSphereSource", "pvGlyphSphere"));
  pvs = vtkPVSource::New();
  pvs->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetVTKSource(s, "pvGlyphSphere");
  pvs->SetName("Sphere");
  //pvs->SetInterface(this->CutPlaneInterface);
  //pvs->CreateProperties();
  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  // Create the object through tcl on all processes.
  d = (vtkDataSet *)(pvApp->MakeTclObject("vtkPolyData", "pvGlyphSphereOutput"));
  pvd->SetVTKData(d, "pvGlyphSphereOutput");
  // Connect the source and data.
  pvs->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", pvs->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  this->GlyphSources->AddItem(pvs);
  pvs->Delete();
  pvd->Delete();


  this->DisableFilterButtons();
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

//----------------------------------------------------------------------------
void vtkPVWindow::OpenCallback()
{
  char *openFileName = NULL;
  char *extension = NULL;
  char *endingSlash = NULL;
  istream *input;
  
  this->Script("set openFileName [tk_getOpenFile -filetypes {{{VTK files} {.vtk}} {{PVTK files} {.pvtk}} {{EnSight files} {.case}} {{POP files} {.pop}} {{STL files} {.stl}}}]");
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
  
  this->Open(openFileName);

}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::Open(char *openFileName)
{
  char *extension = NULL;
  vtkPVSourceInterface *sInt;
  char rootName[100];
  int position;
  char *endingSlash = NULL;
  char *newRootName;
  
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
  
  if (strcmp(extension, ".vtk") == 0)
    {
    sInt = this->GetSourceInterface("vtkPDataSetReader");
    }
  else if (strcmp(extension, ".pvtk") == 0)
    {
    sInt = this->GetSourceInterface("vtkPDataSetReader");
    }
  else if (strcmp(extension, ".case") == 0)
    {
    sInt = this->GetSourceInterface("vtkGenericEnSightReader");
    }
  else if (strcmp(extension, ".pop") == 0)
    {
    sInt = this->GetSourceInterface("vtkPOPReader");
    }
  else if (strcmp(extension, ".stl") == 0)
    {
    sInt = this->GetSourceInterface("vtkSTLReader");
    }
  else
    {
    vtkErrorMacro("Unknown file extension");
    sInt = NULL;
    }
  if (sInt == NULL)
    {
    vtkErrorMacro("Could not find interface.");
    return NULL;
    }

  sInt->SetDataFileName(openFileName);
  sInt->SetRootName(rootName);
  return sInt->CreateCallback();
}


void vtkPVWindow::WriteData()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char filename[256];
  int numProcs;

  if (!this->CurrentPVData)
    {
    return;
    }

  numProcs = 1;
  if (pvApp->GetController())
    {
    numProcs = pvApp->GetController()->GetNumberOfProcesses();
    }

  if (numProcs == 1)
    {
    this->Script("tk_getSaveFile -filetypes {{{VTK files} {.vtk}}} -defaultextension .vtk -initialfile data.vtk");
    sprintf(filename, "%s", this->Application->GetMainInterp()->result);
  
    if (strcmp(filename, "") == 0)
      {
      return;
      }
  
    pvApp->MakeTclObject("vtkDataSetWriter", "writer");
    pvApp->BroadcastScript("writer SetFileName %s", filename);
    pvApp->BroadcastScript("writer SetInput %s",
                           this->GetCurrentPVData()->GetVTKDataTclName());
    pvApp->BroadcastScript("writer SetFileTypeToBinary");
    pvApp->BroadcastScript("writer Write");
    pvApp->BroadcastScript("writer Delete");
    }
  else
    {
    int ghostLevel;
    int idx;

    this->Script("tk_getSaveFile -filetypes {{{PVTK files} {.pvtk}}} -defaultextension .pvtk -initialfile data.pvtk");
    sprintf(filename, "%s", this->Application->GetMainInterp()->result);
    if (strcmp(filename, "") == 0)
      {
      return;
      }
  
    // See if the user wants to save any ghost levels.
    this->Script("tk_dialog .ghostLevelDialog {Ghost Level Selection} {How many ghost levels would you like to save?} {} 0 0 1 2");
    ghostLevel = this->GetIntegerResult(pvApp);
    if (ghostLevel == -1)
      {
      return;
      }

    pvApp->BroadcastScript("vtkPDataSetWriter writer");
    pvApp->BroadcastScript("writer SetFileName %s", filename);
    pvApp->BroadcastScript("writer SetInput %s",
                           this->GetCurrentPVData()->GetVTKDataTclName());
    pvApp->BroadcastScript("writer SetFileTypeToBinary");
    pvApp->BroadcastScript("writer SetNumberOfPieces %d", numProcs);
    pvApp->BroadcastScript("writer SetGhostLevel %d", ghostLevel);
    this->Script("writer SetStartPiece 0");
    this->Script("writer SetEndPiece 0");
    for (idx = 1; idx < numProcs; ++idx)
      {
      pvApp->RemoteScript(idx, "writer SetStartPiece %d", idx);
      pvApp->RemoteScript(idx, "writer SetEndPiece %d", idx);
      }
    pvApp->BroadcastScript("writer Write");
    pvApp->BroadcastScript("writer Delete");
    }
}

//============================================================================
// These methods have things in common, 
// and may in the future use the same methods.

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

  *file << "package require vtktcl_interactor\n\n"
        << "# create a rendering window and renderer\n";
  
  this->GetMainView()->SaveInTclScript(file);
  
  // Loop through sources ...
  sources = this->GetSources();
  sources->InitTraversal();

  int numSources = sources->GetNumberOfItems();
  int sourceCount = 0;

  while (sourceCount < numSources)
    {
    pvs = (vtkPVSource*)sources->GetItemAsObject(sourceCount);
    if (pvs->IsA("vtkPVArrayCalculator"))
      {
      ((vtkPVArrayCalculator*)pvs)->SaveInTclScript(file);
      }
    else if (pvs->IsA("vtkPVContour"))
      {
      ((vtkPVContour*)pvs)->SaveInTclScript(file);
      }
    else if (pvs->IsA("vtkPVGlyph3D"))
      {
      ((vtkPVGlyph3D*)pvs)->SaveInTclScript(file);
      }
    else if (pvs->IsA("vtkPVThreshold"))
      {
      ((vtkPVThreshold*)pvs)->SaveInTclScript(file);
      }
    else if (pvs->IsA("vtkPVProbe"))
      {
      ((vtkPVProbe*)pvs)->SaveInTclScript(file);
      }
    else
      {
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
void vtkPVWindow::SaveWorkspace()
{
  ofstream *file;
  vtkCollection *sources;
  vtkPVSource *pvs;
  char *filename;
  
  this->Script("tk_getSaveFile -filetypes {{{ParaView Files} {.pv}} {{All Files} {.*}}} -defaultextension .tcl");
  filename = this->Application->GetMainInterp()->result;
  
  if (strcmp(filename, "") == 0)
    {
    return;
    }
  
  file = new ofstream(filename, ios::out);
  if (file->fail())
    {
    vtkErrorMacro("Could not open file pipeline.pv");
    delete file;
    file = NULL;
    return;
    }

  *file << "# ParaView Workspace Version 0.1\n\n";

  
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


//============================================================================





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
    this->EnableFilterButtons();
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
  else
    {
    this->DisableFilterButtons();
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
    this->ShowCurrentSourceProperties();
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
vtkKWCompositeCollection* vtkPVWindow::GetGlyphSources()
{
  return this->GlyphSources;
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
void vtkPVWindow::DisableFilterButtons()
{
  this->Script("%s configure -state disabled",
               this->CalculatorButton->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->CutPlaneButton->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->ClipPlaneButton->GetWidgetName());
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
void vtkPVWindow::EnableFilterButtons()
{
  this->Script("%s configure -state normal",
               this->CalculatorButton->GetWidgetName());
  this->Script("%s configure -state normal",
               this->CutPlaneButton->GetWidgetName());
  this->Script("%s configure -state normal",
               this->ClipPlaneButton->GetWidgetName());
  this->Script("%s configure -state normal",
               this->ThresholdButton->GetWidgetName());
  this->Script("%s configure -state normal",
               this->ContourButton->GetWidgetName());
  this->Script("%s configure -state normal",
               this->GlyphButton->GetWidgetName());
  this->Script("%s configure -state normal",
               this->ProbeButton->GetWidgetName());

}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::CalculatorCallback()
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
  int numMenus, i;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot determine output type.");
    return NULL;
    }
  outputDataType = current->GetVTKData()->GetClassName();
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Calculator", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkArrayCalculator", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
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
  
  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      calc->GetVTKSourceTclName(), current->GetVTKDataTclName());

  calc->Delete();
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
  this->DisableFilterButtons();

  return calc;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::CutPlaneCallback()
{
  static int instanceCount = 1;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVCutPlane *cutPlane;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  const char* outputDataType;
  int numMenus, i;
  vtkPVData *current;
  
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("No input.");
    return NULL;
    }

  // Determine the output type.
  outputDataType = "vtkPolyData";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "CutPlane", instanceCount);
  // Create the object through tcl on all processes.
  // I would like to get rid of the pointer, and do everything
  // through the tcl name.
  // VTKCutPlane will be going away, and we will create the 
  // implicit function here.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkCutPlane", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  
  cutPlane = vtkPVCutPlane::New();
  cutPlane->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  cutPlane->SetApplication(pvApp);
  cutPlane->SetVTKSource(s, tclName);
  cutPlane->SetNthPVInput(0, current);
  cutPlane->SetName(tclName);
  cutPlane->SetInterface(this->CutPlaneInterface);

  this->GetMainView()->AddComposite(cutPlane);
  cutPlane->CreateProperties();
  cutPlane->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(cutPlane);

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "CutPlane", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  cutPlane->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", cutPlane->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      cutPlane->GetVTKSourceTclName(), current->GetVTKDataTclName());

  cutPlane->Delete();
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
  this->DisableFilterButtons();

  return cutPlane;
}


//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::ThresholdCallback()
{
  static int instanceCount = 1;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVThreshold *threshold;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  const char* outputDataType;
  int numMenus, i;
  vtkPVData *current;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot threshold; no input");
    return NULL;
    }
  outputDataType = "vtkUnstructuredGrid";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Threshold", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkThreshold", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  
  threshold = vtkPVThreshold::New();
  threshold->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  threshold->SetApplication(pvApp);
  threshold->SetVTKSource(s, tclName);
  threshold->SetNthPVInput(0, current);
  threshold->SetName(tclName);
  threshold->SetInterface(this->ThresholdInterface);

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
  
  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      threshold->GetVTKSourceTclName(), current->GetVTKDataTclName());

  threshold->Delete();
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
  this->DisableFilterButtons();

  return threshold;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::ClipPlaneCallback()
{
  static int instanceCount = 1;
  char tclName[256];
  vtkSource *s;
  vtkDataSet *d;
  vtkPVClipPlane *clipPlane;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVData *pvd;
  const char* outputDataType;
  int numMenus, i;
  vtkPVData *current;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot threshold; no input");
    return NULL;
    }
  
  // Determine the output type.
  outputDataType = "vtkUnstructuredGrid";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "ClipPlane", instanceCount);
  // Create the object through tcl on all processes.
  // I would like to get rid of the pointer, and do everything
  // through the tcl name.
  // vtkClipPlane will be going away, and we will create the 
  // implicit function here.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkClipPlane", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  
  clipPlane = vtkPVClipPlane::New();
  clipPlane->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  clipPlane->SetApplication(pvApp);
  clipPlane->SetVTKSource(s, tclName);
  clipPlane->SetNthPVInput(0, current);
  clipPlane->SetName(tclName);
  clipPlane->SetInterface(this->ClipPlaneInterface);

  this->GetMainView()->AddComposite(clipPlane);
  clipPlane->CreateProperties();
  clipPlane->CreateInputList("vtkDataSet");
  this->SetCurrentPVSource(clipPlane);

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", "ClipPlane", instanceCount);
  // Create the object through tcl on all processes.

  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  clipPlane->SetNthPVOutput(0, pvd);
  pvApp->BroadcastScript("%s SetOutput %s", clipPlane->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      clipPlane->GetVTKSourceTclName(), current->GetVTKDataTclName());

  clipPlane->Delete();
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
  this->DisableFilterButtons();

  return clipPlane;
}


//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::ContourCallback()
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
  int numMenus, i;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot determine output type");
    return NULL;
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
    return NULL;
    }
  
  contour = vtkPVContour::New();
  contour->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  contour->SetApplication(pvApp);
  contour->SetVTKSource(s, tclName);
  contour->SetNthPVInput(0, current);
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
  
  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      contour->GetVTKSourceTclName(), current->GetVTKDataTclName());

  contour->Delete();
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
  this->DisableFilterButtons();

  // It has not really been deleted.
  return contour;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::GlyphCallback()
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
  int numMenus, i;
  
  // Before we do anything, let's see if we can determine the output type.
  current = this->GetCurrentPVData();
  if (current == NULL)
    {
    vtkErrorMacro("Cannot determine output type");
    return NULL;
    }
  outputDataType = "vtkPolyData";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Glyph", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkGlyph3D", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  
  glyph = vtkPVGlyph3D::New();
  glyph->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  glyph->SetApplication(pvApp);
  glyph->SetVTKSource(s, tclName);
  glyph->SetNthPVInput(0, current);
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
  
  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      glyph->GetVTKSourceTclName(), current->GetVTKDataTclName());

  glyph->Delete();
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
  this->DisableFilterButtons();

  return glyph;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVWindow::ProbeCallback()
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
    return NULL;
    }
  outputDataType = "vtkPolyData";
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", "Probe", instanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject("vtkProbeFilter", tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }

  pvApp->BroadcastScript("%s SetSpatialMatch 2", tclName);
  
  probe = vtkPVProbe::New();
  probe->SetPropertiesParent(this->GetMainView()->GetPropertiesParent());
  probe->SetApplication(pvApp);
  probe->SetVTKSource(s, tclName);
  probe->SetProbeSourceTclName(current->GetVTKDataTclName());
  probe->SetPVProbeSource(current);
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

  // Push along the extent translator (for consistent pieces).
  pvApp->BroadcastScript(
    "%s SetExtentTranslator [%s GetExtentTranslator]",
    pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      probe->GetVTKSourceTclName(), current->GetVTKDataTclName());

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
  this->DisableFilterButtons();

  return probe;
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
void vtkPVWindow::ShowAnimationProperties()
{
  this->AnimationInterface->UpdateSourceMenu();

  // Try to find a good default value for the source.
  if (this->AnimationInterface->GetPVSource() == NULL)
    {
    vtkPVSource *pvs = this->GetCurrentPVSource();
    if (pvs == NULL && this->GetSources()->GetNumberOfItems() > 0)
      {
      pvs = (vtkPVSource*)this->GetSources()->GetItemAsObject(0);
      }
    this->AnimationInterface->SetPVSource(pvs);
    }

  // What does this do?
  this->ShowProperties();
  
  // We need to update the properties-menu radio button too!
  this->GetMenuProperties()->CheckRadioButton(
    this->GetMenuProperties(), "Radio", 3);

  // Get rid of the page already packed.
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->MainView->GetPropertiesParent()->GetWidgetName());
  // Put our page in.
  this->Script("pack %s -side top -expand t -fill x -ipadx 3 -ipady 3",
               this->AnimationInterface->GetWidgetName());
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
vtkPVSourceInterface *vtkPVWindow::GetSourceInterface(const char *className)
{
  vtkPVSourceInterface *sInt;  
  this->SourceInterfaces->InitTraversal();

  while ( (sInt = (vtkPVSourceInterface*)this->SourceInterfaces->GetNextItemAsObject()) )
    {
    if (strcmp(sInt->GetSourceClassName(), className) == 0)
      {
      return sInt;
      }
    }
  return NULL;
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
  sInt->SetSourceClassName("vtkPDataSetReader");
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
  
  // Setup our built in source interfaces.
  this->ReadSourceInterfacesFromString(vtkPVWindow::StandardSourceInterfaces);
  this->ReadSourceInterfacesFromString(vtkPVWindow::StandardFilterInterfaces);
  
  // A list of standard directories in which to find interfaces.  The
  // first directory in this list that is found is the only one used.
  static const char* standardDirectories[] =
    {
#ifdef VTK_PV_BINARY_CONFIG_DIR
      VTK_PV_BINARY_CONFIG_DIR,
#endif
#ifdef VTK_PV_SOURCE_CONFIG_DIR
      VTK_PV_SOURCE_CONFIG_DIR,
#endif
#ifdef VTK_PV_INSTALL_CONFIG_DIR
      VTK_PV_INSTALL_CONFIG_DIR,
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
    // Don't complain for now.  We can choose desired behavior later.
    // vtkWarningMacro("Could not find any directories for standard interface files.");
    }

  char* str = getenv("PV_INTERFACE_PATH");
  if (str)
    {
    this->ReadSourceInterfacesFromDirectory(str);
    }

}

//----------------------------------------------------------------------------
void vtkPVWindow::ReadSourceInterfacesFromString(const char* str)
{
  vtkPVSourceInterfaceParser *parser = vtkPVSourceInterfaceParser::New();
  parser->SetPVApplication(this->GetPVApplication());
  parser->SetPVWindow(this);
  parser->SetSourceInterfaces(this->SourceInterfaces);
  parser->ParseString(str);
  parser->Delete();
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


// Define the standard source interfaces.
const char* vtkPVWindow::StandardSourceInterfaces =
"<Interfaces>\n"
"<Source class=\"vtkArrowSource\" root=\"Arrow\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"TipResolution\" type=\"int\" help=\"Set the number of faces on the tip.\"/>\n"
"  <Scalar name=\"TipRadius\" type=\"float\" help=\"Set the radius of the widest part of the tip.\"/>\n"
"  <Scalar name=\"TipLength\" type=\"float\" help=\"Set the length of the tip (the whole arrow is length 1)\"/>\n"
"  <Scalar name=\"ShaftResolution\" type=\"int\" help=\"Set the number of faces on shaft\"/>\n"
"  <Scalar name=\"ShaftRadius\" type=\"float\" help=\"Set the radius of the shaft\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkAxes\" root=\"Axes\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Scale\" set=\"SetScaleFactor\" get=\"GetScaleFactor\" type=\"float\" help=\"Set the size of the axes\"/>\n"
"  <Vector name=\"Origin\" type=\"float\" length=\"3\" help=\"Set the x, y, z coordinates of the origin of the axes\"/>\n"
"  <Boolean name=\"Symmetric\" help=\"Select whether to display the negative axes\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkConeSource\" root=\"Cone\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Resolution\" type=\"int\" help=\"Set the number of faces on this cone\"/>\n"
"  <Scalar name=\"Radius\" type=\"float\" help=\"Set the radius of the widest part of the cone\"/>\n"
"  <Scalar name=\"Height\" type=\"float\" help=\"Set the height of the cone\"/>\n"
"  <Boolean name=\"Capping\" help=\"Set whether to draw the base of the cone\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkCubeSource\" root=\"Cube\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"XLength\" type=\"int\" help=\"The length of the cube in the x direction.\"/>\n"
"  <Scalar name=\"YLength\" type=\"int\" help=\"The length of the cube in the y direction.\"/>\n"
"  <Scalar name=\"ZLength\" type=\"int\" help=\"The length of the cube in the z direction.\"/>\n"
"  <Vector name=\"Center\" type=\"float\" length=\"3\" help=\"Set the center of the cube.\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkCylinderSource\" root=\"Cyl\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Resolution\" type=\"int\" help=\"The number of facets used to define the cylinder.\"/>\n"
"  <Scalar name=\"Height\" type=\"float\" help=\"The height of the cylinder (along the y axis).\"/>\n"
"  <Scalar name=\"Radius\" type=\"float\" help=\"The radius of the cylinder.\"/>\n"
"  <Vector name=\"Center\" type=\"float\" length=\"3\" help=\"Set the center of the cylinder.\"/>\n"
"  <Boolean name=\"Capping\" help=\"Set whether to draw the ends of the cylinder\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkImageMandelbrotSource\" root=\"Fractal\" output=\"vtkImageData\">\n"
"  <Vector name=\"Extent\" set=\"SetWholeExtent\" get=\"GetWholeExtent\" type=\"int\" length=\"6\" help=\"Set the min and max values of the data in each dimension\"/>\n"
"  <Vector name=\"SubSpace\" set=\"SetProjectionAxes\" get=\"GetProjectionAxes\" type=\"int\" length=\"3\" help=\"Choose which axes of the data set to display\"/>\n"
"  <Vector name=\"Origin\" set=\"SetOriginCX\" get=\"GetOriginCX\" type=\"float\" length=\"4\" help=\"Set the imaginary and real values for C (constant) and X (initial value)\"/>\n"
"  <Vector name=\"Spacing\" set=\"SetSampleCX\" get=\"GetSampleCX\" type=\"float\" length=\"4\" help=\"Set the inaginary and real values for the spacing for C (constant) and X (initial value)\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkImageReader\" root=\"ImageRead\" output=\"vtkImageData\">\n"
"  <String name=\"FilePrefix\" help=\"Set the prefix for the files for this image data.\"/>\n"
"  <String name=\"FilePattern\" help=\"Set the format string.\"/>\n"
"  <Scalar name=\"ScalarType\" set=\"SetDataScalarType\" get=\"GetDataScalarType\" type=\"int\" help=\"Set the scalar type for the data: unsigned char (3), short (4), unsigned short (5), int (6), float (10), double(11)\"/>\n"
"  <Vector name=\"Extent\" set=\"SetDataExtent\" get=\"GetDataExtent\" type=\"int\" length=\"6\" help=\"Set the min and max values of the data in each dimension\"/>\n"
"</Source>  \n"
"\n"
"<Source class=\"vtkOutlineCornerSource\" root=\"Corners\" output=\"vtkPolyData\">\n"
"  <Vector name=\"Bounds\" type=\"float\" length=\"6\" help=\"Bounds of the outline.\"/>\n"
"  <Scalar name=\"CornerFactor\" type=\"float\" help=\"The relative size of the corners to the length of the corresponding bounds. (0.001 -> 0.5)\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkParticleReader\" root=\"Particle\" output=\"vtkPolyData\">\n"
"  <File name=\"FileName\" extension=\"*\" help=\"Set the file to read.\"/>\n"
"</Source>  \n"
"\n"
"<Source class=\"vtkPLOT3DReader\" root=\"Plot3D\" output=\"vtkStructuredGrid\">\n"
"<File name=\"XYZFileName\" extension=\"bin\" help=\"Set the geometry file to read.\"/>\n"  
"<File name=\"QFileName\" extension=\"bin\" help=\"Set the data file to read.\"/>\n"
"<Scalar name=\"ScalarFunctionNumber\" type=\"int\" help=\"PLOT3D number for scalars\"/>\n"
"<Scalar name=\"VectorFunctionNumber\" type=\"int\" help=\"PLOT3D number for vectors\"/>\n"
"</Source>\n"
"<Source class=\"vtkPOPReader\" root=\"POPReader\" output=\"vtkStructuredGrid\">\n"
"  <File name=\"FileName\" extension=\"pop\" help=\"Select the file for the data set\"/>\n"
"  <Scalar name=\"Radius\" type=\"float\" help=\"Set the radius of the data set\"/>\n"
"  <Vector name=\"ClipExtent\" type=\"int\" length=\"6\" help=\"For reading a smaller extent.\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkSphereSource\" root=\"Sphere\" output=\"vtkPolyData\">\n"
"  <Vector name=\"Center\" type=\"float\" length=\"3\" help=\"Set the coordinates for the center of the sphere.\"/>\n"
"  <Scalar name=\"Radius\" type=\"float\" help=\"Set the radius of the sphere\"/>\n"
"  <Scalar name=\"Theta Resolution\" set=\"SetThetaResolution\" get=\"GetThetaResolution\" type=\"int\" help=\"Set the number of points in the longitude direction (ranging from Start Theta to End Theta)\"/>\n"
"  <Scalar name=\"Start Theta\" set=\"SetStartTheta\" get=\"GetStartTheta\" type=\"float\" help=\"Set the starting angle in the longitude direction\"/>\n"
"  <Scalar name=\"End Theta\" set=\"SetEndTheta\" get=\"GetEndTheta\" type=\"float\" help=\"Set the ending angle in the longitude direction\"/>\n"
"  <Scalar name=\"Phi Resolution\" set=\"SetPhiResolution\" get=\"GetPhiResolution\" type=\"int\" help=\"Set the number of points in the latitude direction (ranging from Start Phi to End Phi)\"/>\n"
"  <Scalar name=\"Start Phi\" set=\"SetStartPhi\" get=\"GetStartPhi\" type=\"float\" help=\"Set the starting angle in the latitude direction\"/>\n"
"  <Scalar name=\"End Phi\" set=\"SetEndPhi\" get=\"GetEndPhi\" type=\"float\" help=\"Set the ending angle in the latitude direction\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkSTLReader\" root=\"STLReader\" output=\"vtkPolyData\">\n"
"  <File name=\"FileName\" extension=\"stl\" help=\"Select the data file for the STL data set\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkSuperquadricSource\" root=\"SQuad\" output=\"vtkPolyData\">\n"
"  <Vector name=\"Center\" type=\"float\" length=\"3\" help=\"Set the center of the superquadric\"/>\n"
"  <Vector name=\"Scale\" type=\"float\" length=\"3\" help=\"Set the scale of the superquadric\"/>\n"
"  <Scalar name=\"ThetaResolution\" type=\"int\" help=\"The number of points in the longitude direction.\"/>\n"
"  <Scalar name=\"PhiResolution\" type=\"int\" help=\"The number of points in the latitude direction.\"/>\n"
"  <Scalar name=\"Thickness\" type=\"float\" help=\"Changing thickness maintains the outside diameter of the toroid.\"/>\n"
"  <Scalar name=\"ThetaRoundness\" type=\"float\" help=\"Values range from 0 (rectangular) to 1 (circular) to higher order.\"/>\n"
"  <Scalar name=\"PhiRoundness\" type=\"float\" help=\"Values range from 0 (rectangular) to 1 (circular) to higher order.\"/>\n"
"  <Scalar name=\"Size\" type=\"float\" help=\"Isotropic size\"/>\n"
"  <Boolean name=\"Toroidal\" help=\"Whether or not the superquadric is toroidal or ellipsoidal\"/>\n"
"</Source>\n"
"\n"
"<Source class=\"vtkVectorText\" root=\"VectorText\" output=\"vtkPolyData\">\n"
"  <String name=\"Text\" help=\"Enter the text to display\"/>\n"
"</Source>\n"
"\n"
"</Interfaces>\n";


// Define the standard filter interfaces.
const char* vtkPVWindow::StandardFilterInterfaces=
"<Interfaces>\n"
"\n"
"<Filter class=\"vtkBrownianPoints\" root=\"BPts\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Scalar name=\"MinimumSpeed\" type=\"float\" help=\"The minimum size of the random point vectors generated.\"/>\n"
"  <Scalar name=\"MaximumSpeed\" type=\"float\" help=\"The maximum size of the random point vectors generated.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkButterflySubdivisionFilter\" root=\"BFSubDiv\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"NumberOfSubdivisions\" type=\"int\" help=\"Each subdivision changes single triangles into four triangles..\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkCellCenters\" root=\"Centers\" input=\"vtkDataSet\" output=\"vtkPolyData\">\n"
"  <Boolean name=\"VertexCells\" help=\"Generate vertex as geometry of just points.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkCellDataToPointData\" root=\"CellToPoint\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Boolean name=\"PassCellData\" help=\" Control whether the input cell data is to be passed to the output. If on, then the input cell data is passed through to the output; otherwise, only generated point data is placed into the output.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkCellDerivatives\" root=\"CellDeriv\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Selection name=\"VectorMode\" help=\"Value to put in vector cell data\">\n"
"    <Choice name=\"Pass\" value=\"0\"/>\n"
"    <Choice name=\"Vorticity\" value=\"1\"/>\n"
"    <Choice name=\"Gradient\" value=\"2\"/>\n"
"  </Selection>\n"
"  <Selection name=\"TensorMode\" help=\"Value to put in tensaor cell data\">\n"
"      <Choice name=\"Pass\" value=\"0\"/>\n"
"      <Choice name=\"Strain\" value=\"1\"/>\n"
"      <Choice name=\"Gradient\" value=\"2\"/>\n"
"  </Selection>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkCleanPolyData\" root=\"CleanPD\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Boolean name=\"PieceInvariant\" help=\"Turn this off if you do not want pieces or do not mind seams.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkClipPlane\" root=\"ClipPlane\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Vector name=\"Origin\" type=\"float\" length=\"3\" help=\"Set the origin of the clipping plane\"/>\n"
"  <Vector name=\"Normal\" type=\"float\" length=\"3\" help=\"Set the normal of the clipping plane\"/>\n"
"  <Scalar name=\"Value\" type=\"float\" help=\"Set the clipping value of the implicit plane\"/>\n"
"  <Boolean name=\"InsideOut\" help=\"Select which &quot;side&quot; of the data set to clip away\"/>\n"
"  <Boolean name=\"GenerateClipScalars\" help=\"If this flag is enabled, then the output scalar values will be interpolated from the implicit function values, and not the input scalar data\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkClipDataSet\" root=\"ClipDS\" input=\"vtkDataSet\" output=\"vtkUnstructuredGrid\" default=\"scalars\">\n"
"  <Scalar name=\"Value\" type=\"float\" help=\"Set the scalar value to clip by\"/>\n"
"  <Boolean name=\"InsideOut\" help=\"Select which &quot;side&quot; of the data set to clip away\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkClipPolyData\" root=\"ClipScalars\" input=\"vtkPolyData\" output=\"vtkPolyData\" default=\"scalars\">\n"
"  <Scalar name=\"Value\" type=\"float\" help=\"Set the scalar value to clip by\"/>\n"
"  <Boolean name=\"InsideOut\" help=\"Select which &quot;side&quot; of the data set to clip away\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkCutMaterial\" root=\"CutMaterial\" input=\"vtkDataSet\" output=\"vtkPolyData\">\n"
"  <String name=\"MaterialArray\" set=\"SetMaterialArrayName\" get=\"GetMaterialArrayName\" help=\"Enter the array name of the cell array containing the material values\"/>\n"
"  <Scalar name=\"Material\" type=\"int\" help=\"Set the value of the material to probe\"/>\n"
"  <String name=\"Array\" set=\"SetArrayName\" get=\"GetArrayName\" help=\"Set the array name of the array to cut\"/>\n"
"  <Vector name=\"UpVector\" type=\"float\" length=\"3\" help=\"Specify the normal vector of the plane to cut by\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkDataSetSurfaceFilter\" root=\"Surface\" input=\"vtkDataSet\" output=\"vtkPolyData\"/>\n"
"\n"
"<Filter class=\"vtkDataSetTriangleFilter\" root=\"Tetra\" input=\"vtkDataSet\" output=\"vtkUnstructuredGrid\"/>\n"
"\n"
"<Filter class=\"vtkDecimatePro\" root=\"Deci\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"TargetReduction\" type=\"float\" help=\"Value between 0 and 1. Desired reduction of the total number of triangles.\"/>\n"
"  <Boolean name=\"PreserveTopology\" help=\"If off, better reduction can occur, but model may break up.\"/>\n"
"  <Scalar name=\"FeatureAngle\" type=\"float\" help=\"Topology can be split along features.\"/>\n"
"  <Boolean name=\"BoundaryVertexDeletion\" help=\"If off, decimate will not remove points on the boundary.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkDelaunay2D\" root=\"Del2D\" input=\"vtkPointSet\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Alpha\" type=\"float\" help=\"Distance value to control output of this filter.\"/>\n"
"  <Boolean name=\"BoundingTriangulation\" help=\"Include bounding triagulation or not.\"/>\n"
"  <Scalar name=\"Offset\" type=\"float\" help=\"Multiplier for initial triagulation size.\"/>\n"
"  <Scalar name=\"Tolerance\" type=\"float\" help=\"A value 0->1 for merginge close points.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkDelaunay3D\" root=\"Del3D\" input=\"vtkPointSet\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Alpha\" type=\"float\" help=\"Distance value to control output of this filter.\"/>\n"
"  <Boolean name=\"BoundingTriangulation\" help=\"Include bounding triagulation or not.\"/>\n"
"  <Scalar name=\"Offset\" type=\"float\" help=\"Multiplier for initial triagulation size.\"/>\n"
"  <Scalar name=\"Tolerance\" type=\"float\" help=\"A value 0->1 for merginge close points.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkElevationFilter\" root=\"Elevation\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Vector name=\"LowPoint\" type=\"float\" length=\"3\" help=\"Set the minimum point for the elevation\"/>\n"
"  <Vector name=\"HighPoint\" type=\"float\" length=\"3\" help=\"Set the maximum point for the elevation\"/>\n"
"  <Vector name=\"ScalarRange\" type=\"float\" length=\"2\" help=\"Set the range of scalar values to generate\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkExtractEdges\" root=\"ExtractEdges\" input=\"vtkDataSet\" output=\"vtkPolyData\"/>\n"
"\n"
"<Filter class=\"vtkExtractGrid\" root=\"ExtractGrid\" input=\"vtkStructuredGrid\" output=\"vtkStructuredGrid\">\n"
"  <Extent name=\"VOI\" help=\"Set the min/max values of the volume of interest (VOI)\"/>\n"
"  <Vector name=\"SampleRate\" type=\"int\" length=\"3\" help=\"Set the sampling rate for each dimension\"/>\n"
"  <Boolean name=\"IncludeBoundary\" help=\"Select whether to always include the boundary of the grid in the output\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkImageClip\" root=\"ImageClip\" input=\"vtkImageData\" output=\"vtkImageData\">\n"
"  <Extent name=\"OutputWholeExtent\" help=\"Set the min/max extents in each dimension of the output\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkImageContinuousDilate3D\" root=\"ContDilate\" input=\"vtkImageData\" output=\"vtkImageData\">\n"
"  <Vector name=\"KernelSize\" type=\"int\" length=\"3\" help=\"Radius of elliptical footprint.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkImageContinuousErode3D\" root=\"ContErode\" input=\"vtkImageData\" output=\"vtkImageData\">\n"
"  <Vector name=\"KernelSize\" type=\"int\" length=\"3\" help=\"Radius of elliptical footprint.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkImageGradient\" root=\"ImageGradient\" input=\"vtkImageData\" output=\"vtkImageData\">\n"
"  <Selection name=\"Dimensionality\" help=\"Select whether to perform a 2d or 3d gradient\">\n"
"    <Choice name=\"2\" value=\"0\"/>\n"
"    <Choice name=\"3\" value=\"1\"/>\n"
"  </Selection>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkImageGaussianSmooth\" root=\"GaussSmooth\" input=\"vtkImageData\" output=\"vtkImageData\">\n"
"  <Vector name=\"StandardDeviations\" type=\"float\" length=\"3\" help=\"STD for x, y and z axes.\"/>\n"
"  <Vector name=\"RadiusFactors\" type=\"float\" length=\"3\" help=\"Kernel size is this factor times STD.\"/>\n"
"  <Selection name=\"Dimensionality\" help=\"Select whether to perform a 2d or 3d gradient\">\n"
"    <Choice name=\"2\" value=\"0\"/>\n"
"    <Choice name=\"3\" value=\"1\"/>\n"
"  </Selection>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkInhibitPoints\" root=\"InhibPts\" input=\"vtkDataSet\" output=\"vtkPolyData\" default=\"vectors\">\n"
"  <Scalar name=\"Scale\" type=\"float\" help=\"Set the size of the inihibition neighborhood.\"/>\n"
"  <Scalar name=\"MagnitudeThreshold\" type=\"float\" help=\"Vectors smaller that this are not considered.\"/>\n"
"  <Boolean name=\"GenerateVertices\" help=\"Flag to create vertex cells\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkPLinearExtrusionFilter\" root=\"LinExtrude\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"ScaleFactor\" type=\"float\" help=\"Set the extrusion scale factor\"/>\n"
"  <Vector name=\"Vector\" type=\"float\" length=\"3\" help=\"Set the direction for the extrusion\"/>\n"
"  <Boolean name=\"Capping\" help=\"Select whether to draw endcaps\"/>\n"
"  <Boolean name=\"PieceInvariant\" help=\"Turn this off if you do want to process ghost levels and do not mind seams.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkLinearSubdivisionFilter\" root=\"LinSubDiv\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"NumberOfSubdivisions\" type=\"int\" help=\"Each subdivision changes single triangles into four triangles.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkLoopSubdivisionFilter\" root=\"LoopSubDiv\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"NumberOfSubdivisions\" type=\"int\" help=\"Each subdivision changes single triangles into four triangles.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkMaskPoints\" root=\"MaskPts\" input=\"vtkDataSet\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"OnRatio\" type=\"int\" help=\"The ratio of points retained.\"/>\n"
"  <Scalar name=\"MaxPoints\" set=\"SetMaximumNumberOfPoints\" get=\"GetMaximumNumberOfPoints\" type=\"int\" help=\"Limit the number of points.\"/>\n"
"  <Scalar name=\"Offset\" type=\"int\" help=\"Start with this point.\"/>\n"
"  <Boolean name=\"Random\" set=\"SetRandomMode\" get=\"GetRandomMode\" help=\"Select whether to randomly select points, or subsample regularly.\"/>\n"
"  <Boolean name=\"GenerateVertices\" help=\"Convienience feature to display points.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkOBBDicer\" root=\"Dicer\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Boolean name=\"FieldData\" help=\"Generate point scalars or a field array.\"/>\n"
"  <Scalar name=\"NumberOfPointsPerPiece\" type=\"int\" help=\"Controls piece size.\"/>\n"
"  <Scalar name=\"NumberOfPieces\" type=\"int\" help=\"Controls number of pieces generated.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkOutlineCornerFilter\" root=\"COutline\" input=\"vtkDataSet\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"CornerFactor\" type=\"float\" help=\"The relative size of the corners to the length of the corresponding bounds. (0.001 -> 0.5)\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkPieceScalars\" root=\"ColorPieces\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Boolean name=\"Random\" set=\"SetRandomMode\" get=\"GetRandomMode\" help=\"Select whether to use random colors for the various pieces\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkPointDataToCellData\" root=\"PtToCell\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Boolean name=\"PassPointData\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkPPolyDataNormals\" root=\"PDNormals\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"FeatureAngle\" type=\"float\" help=\"Points are duplicated along features over this angle (0->180)\"/>\n"
"  <Boolean name=\"Splitting\" help=\"Turn on/off the splitting of sharp edges.\"/>\n"
"  <Boolean name=\"Consistency\" help=\"Turn on/off the enforcement of consistent polygon ordering.\"/>\n"
"  <Boolean name=\"Consistency\" help=\"Turn on/off the enforcement of consistent polygon ordering.\"/>\n"
"  <Boolean name=\"CellNormals\" set=\"SetComputeCellNormals\" get=\"GetComputeCellNormals\" help=\"Turn on/off the computation of cell normals.\"/>\n"
"  <Boolean name=\"Consistency\" help=\"Turn on/off the enforcement of consistent polygon ordering.\"/>\n"
"  <Boolean name=\"FlipNormals\" help=\"Flipping reverves the meaning of front and back.\"/>\n"
"  <Boolean name=\"NonManifold\" set=\"SetNonManifoldTraversal\" get=\"GetNonManifoldTraversal\" help=\"Turn on/off traversal across non-manifold edges. This will prevent problems where the consistency of polygonal ordering is corrupted due to topological loops.\"/>\n"
"  <Boolean name=\"PieceInvariant\" help=\"Turn this off if you do not want to process ghost levels and do not mind seams.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkPolyDataStreamer\" root=\"Stream\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"NumberOfDivisions\" set=\"SetNumberOfStreamDivisions\" get=\"GetNumberOfStreamDivisions\"\n"
"          type=\"int\" help=\"Set the number of pieces to divide the data set into for streaming\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkReverseSense\" root=\"RevSense\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Boolean name=\"CellOrder\" set=\"SetReverseCells\" get=\"GetReverseCells\"/>\n"
"  <Boolean name=\"ReverseNormals\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkRibbonFilter\" root=\"Ribbon\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Width\" type=\"float\" help=\"The half width of the ribbon (or minimum).\"/>\n"
"  <Scalar name=\"Angle\" type=\"float\" help=\"The offset angle of the ribbon from the line normal (0->360).\"/>\n"
"  <Vector name=\"DefaultNormal\" type=\"float\" length=\"3\" help=\"If no normals are supplied\"/>\n"
"  <Boolean name=\"VaryWidth\" help=\"Turn on/off the variation of ribbon width with scalar value.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkRotationalExtrusionFilter\" root=\"RotExtrude\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"Resolution\" type=\"int\"/>\n"
"  <Boolean name=\"Capping\"/>\n"
"  <Scalar name=\"Angle\" type=\"float\" help=\"Set the angle of rotation.\"/>\n"
"  <Scalar name=\"Translation\" type=\"float\" help=\"The total amount of translation along the z-axis.\"/>\n"
"  <Scalar name=\"DeltaRadius\" type=\"float\" help=\"The change in radius during sweep process.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkQuadricClustering\" root=\"QC\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Vector name=\"Spacing\" set=\"SetDivisionSpacing\" get=\"GetDivisionSpacing\" type=\"float\" length=\"3\" help=\"Set the spacing of the bins in each dimension\"/>\n"
"  <Boolean name=\"UseInputPoints\" help=\"Select whether to use points from the input in the output or to calculate optimum representative points for each bin\"/>\n"
"  <Boolean name=\"UseFeatureEdges\" help=\"Select whether to use feature edge quadrics to match up the boundaries between pieces\"/>\n"
"  <Boolean name=\"UseFeaturePoints\" help=\"Select whether to use feature point quadrics to align piece boundaries\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkRectilinearGridGeometryFilter\" root=\"GridGeom\" input=\"vtkRectilinearGrid\" output=\"vtkPolyData\">\n"
"  <Extent name=\"Extent\" help=\"Set the min/max extents of the grid\"/>\n"
"</Filter>\n"
"<Filter class=\"vtkShrinkFilter\" root=\"Shrink\" input=\"vtkDataSet\" output=\"vtkUnstructuredGrid\">\n"
"  <Scalar name=\"ShrinkFactor\" type=\"float\" help=\"Set the amount to shrink by\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkShrinkPolyData\" root=\"ShrinkPD\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"ShrinkFactor\" type=\"float\" help=\"Set the amount to shrink by\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkSimpleFieldDataToAttributeDataFilter\" root=\"FieldToAttr\" input=\"vtkDataSet\" output=\"vtkDataSet\">\n"
"  <Selection name=\"Attribute\" help=\"Select whether the array contains scalars or vectors\">\n"
"    <Choice name=\"Scalars\" value=\"0\"/>\n"
"    <Choice name=\"Vectors\" value=\"1\"/>\n"
"  </Selection>\n"
"  <String name=\"Field Name\" set=\"SetFieldName\" get=\"GetFieldName\" help=\"Set the name of the array containing the data to operate on\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkSmoothPolyDataFilter\" root=\"Smooth\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"NumberOfIterations\" type=\"int\" help=\"More iterations produces better smoothing.\"/>\n"
"  <Scalar name=\"Convergence\" type=\"float\" help=\"Smooting factor (0->1).\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkStructuredGridGeometryFilter\" root=\"GridGeom\" input=\"vtkStructuredGrid\" output=\"vtkPolyData\">\n"
"  <Vector name=\"Extent\" type=\"int\" length=\"6\" help=\"Set the min/max extents of the grid\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkStructuredGridOutlineFilter\" root=\"GOutline\" input=\"vtkStructuredGrid\" output=\"vtkPolyData\" />\n"
"\n"
"<Filter class=\"vtkTriangleFilter\" root=\"Tri\" input=\"vtkPolyData\" output=\"vtkPolyData\"/>\n"
"\n"
"<Filter class=\"vtkTubeFilter\" root=\"Tuber\" input=\"vtkPolyData\" output=\"vtkPolyData\">\n"
"  <Scalar name=\"NumberOfSides\" type=\"int\" help=\"Set the number of sides for the tube\"/>\n"
"  <Boolean name=\"Capping\" help=\"Select whether to draw endcaps on the tube\"/>\n"
"  <Scalar name=\"Radius\" type=\"float\" help=\"Set the radius of the tube\"/>\n"
"  <Selection name=\"VaryRadius\" help=\"Select whether/how to vary the radius of the tube\">\n"
"    <Choice name=\"Off\" value=\"0\"/>\n"
"    <Choice name=\"ByScalar\" value=\"1\"/>\n"
"    <Choice name=\"ByVector\" value=\"2\"/>\n"
"  </Selection>\n"
"  <Scalar name=\"RadiusFactor\" type=\"float\" help=\"Set the maximum tube radius in terms of a multiple of the minimum radius\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkWarpScalar\" root=\"WarpS\" input=\"vtkPointSet\" output=\"vtkPointSet\" default=\"scalars\">\n"
"  <Scalar name=\"ScaleFactor\" type=\"float\" help=\"Displacement is vector times scale.\"/>\n"
"  <Vector name=\"Normal\" type=\"float\" length=\"3\" help=\"Warp direction.\"/>\n"
"  <Boolean name=\"UseNormal\" help=\"Use instance model normals rather than instance normal.\"/>\n"
"  <Boolean name=\"XYPlane\" help=\"Z value is used to warp the surface, scalars to color surface.\"/>\n"
"</Filter>\n"
"\n"
"<Filter class=\"vtkWarpVector\" root=\"WarpV\" input=\"vtkPointSet\" output=\"vtkPointSet\" default=\"vectors\">\n"
"  <Scalar name=\"ScaleFactor\" type=\"float\" help=\"Displacement is vector times scale.\"/>\n"
"</Filter>\n"
"\n"
"</Interfaces>\n";
