/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLODRenderModuleUI.h"
#include "vtkPVLODRenderModule.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWScale.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWWindowCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVApplicationSettingsInterface.h"
#include "vtkPVCameraIcon.h"
#include "vtkPVConfig.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVInteractorStyleControl.h"
#include "vtkPVNavigationWindow.h"
#include "vtkPVProcessModule.h"
#include "vtkPVLODRenderModuleUI.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceList.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVRenderView.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODRenderModuleUI);
vtkCxxRevisionMacro(vtkPVLODRenderModuleUI, "1.12");

int vtkPVLODRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVLODRenderModuleUI::vtkPVLODRenderModuleUI()
{
  this->CommandFunction = vtkPVLODRenderModuleUICommand;
  
  this->LODRenderModule = NULL;

  this->RenderInterruptsEnabledCheck = vtkKWCheckButton::New();

  this->LODFrame = vtkKWLabeledFrame::New();
 
  this->LODScalesFrame = vtkKWWidget::New();
  this->LODThresholdLabel = vtkKWLabel::New();
  this->LODCheck = vtkKWCheckButton::New();
  this->LODThresholdScale = vtkKWScale::New();
  this->LODThresholdValue = vtkKWLabel::New();
  this->LODResolutionLabel = vtkKWLabel::New();
  this->LODResolutionScale = vtkKWScale::New();
  this->LODResolutionValue = vtkKWLabel::New();

  this->LODThreshold = 5.0;
  this->LODResolution = 50;

  this->RenderInterruptsEnabled = 1;
}


//----------------------------------------------------------------------------
vtkPVLODRenderModuleUI::~vtkPVLODRenderModuleUI()
{
  // Save UI values in registry.
  vtkPVApplication* pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegisteryValue(2, "RunTime", "LODThreshold", "%f",
                             this->LODThreshold);
    pvapp->SetRegisteryValue(2, "RunTime", "LODResolution", "%d",
                             this->LODResolution);
    pvapp->SetRegisteryValue(2, "RunTime", "RenderInterruptsEnabled", "%d",
                             this->RenderInterruptsEnabled);

    }

  this->LODFrame->Delete();
  this->LODFrame = NULL;

  this->RenderInterruptsEnabledCheck->Delete();
  this->RenderInterruptsEnabledCheck = NULL;

  this->LODScalesFrame->Delete();
  this->LODScalesFrame = NULL;

  this->LODThresholdLabel->Delete();
  this->LODThresholdLabel = NULL;
  this->LODThresholdScale->Delete();
  this->LODThresholdScale = NULL;
  this->LODThresholdValue->Delete();
  this->LODThresholdValue = NULL;

  this->LODResolutionLabel->Delete();
  this->LODResolutionLabel = NULL;
  this->LODCheck->Delete();
  this->LODCheck = NULL;
  this->LODResolutionScale->Delete();
  this->LODResolutionScale = NULL;
  this->LODResolutionValue->Delete();
  this->LODResolutionValue = NULL;

  if (this->LODRenderModule)
    {
    this->LODRenderModule->UnRegister(this);
    this->LODRenderModule = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetRenderModule(vtkPVRenderModule* rm)
{
  if (this->LODRenderModule)
    {
    this->LODRenderModule->UnRegister(this);
    this->LODRenderModule = NULL;
    }
  this->LODRenderModule = vtkPVLODRenderModule::SafeDownCast(rm);
  if (this->LODRenderModule)
    {
    this->LODRenderModule->Register(this);
    }

  if (rm != NULL && this->LODRenderModule == NULL)
    {
    vtkErrorMacro("Expecting a LODRenderModule.");
    }
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  vtkPVApplication *pvapp = vtkPVApplication::SafeDownCast(app);
  
  if (this->Application)
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }

  this->Superclass::Create(app, NULL);  

  // LOD parameters
  this->LODFrame->SetParent(this);
  this->LODFrame->ShowHideFrameOn();
  this->LODFrame->Create(this->Application,0);
  this->LODFrame->SetLabel("LOD Parameters");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->LODFrame->GetWidgetName());

  // LOD parameters: the frame that will pack all scales
  this->LODScalesFrame->SetParent(this->LODFrame->GetFrame());
  this->LODScalesFrame->Create(this->Application, "frame", "");

  // LOD parameters: threshold
  this->LODThresholdLabel->SetParent(this->LODScalesFrame);
  this->LODThresholdLabel->Create(this->Application, "-anchor w");
  this->LODThresholdLabel->SetLabel("LOD threshold:");

  this->LODCheck->SetParent(this->LODScalesFrame);
  this->LODCheck->Create(this->Application, "");
  this->LODCheck->SetCommand(this, "LODCheckCallback");

  this->LODThresholdScale->SetParent(this->LODScalesFrame);
  this->LODThresholdScale->Create(this->Application, 
                                  "-resolution 0.1 -orient horizontal");
  this->LODThresholdScale->SetRange(0.0, 100.0);
  this->LODThresholdScale->SetResolution(0.1);


  this->LODThresholdValue->SetParent(this->LODScalesFrame);
  this->LODThresholdValue->Create(this->Application, "-anchor w");

  if (pvapp &&
      pvapp->GetRegisteryValue(2, "RunTime", "LODThreshold", 0))
    {
    this->LODThreshold = 
      pvapp->GetFloatRegisteryValue(2, "RunTime", "LODThreshold");
    }
  this->SetLODThreshold(this->LODThreshold);
  this->LODThresholdScale->SetValue(this->LODThreshold);
  this->LODThresholdScale->SetCommand(this, 
                                      "LODThresholdLabelCallback");
  this->LODThresholdScale->SetEndCommand(this, 
                                         "LODThresholdScaleCallback");
  this->LODThresholdScale->SetBalloonHelpString(
    "This slider determines whether to use decimated models "
    "during interaction.  Threshold critera is based on size "
    "of geometry in mega bytes.  "
    "Left: Always use full resolution. Right: Always use decimated models.");    

  int row = 0;

  pvapp->Script("grid %s -row %d -column 2 -sticky nws", 
                this->LODThresholdValue->GetWidgetName(), row++);
  pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                this->LODThresholdLabel->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 1 -sticky nes", 
                this->LODCheck->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                this->LODThresholdScale->GetWidgetName(), row++);
  
  pvapp->Script("grid columnconfigure %s 2 -weight 1",
                this->LODThresholdScale->GetParent()->GetWidgetName());

  // LOD parameters: resolution

  this->LODResolutionLabel->SetParent(this->LODScalesFrame);
  this->LODResolutionLabel->Create(this->Application, "-anchor w");
  this->LODResolutionLabel->SetLabel("LOD resolution:");

  this->LODResolutionScale->SetParent(this->LODScalesFrame);
  this->LODResolutionScale->Create(this->Application, "-orient horizontal");
  this->LODResolutionScale->SetRange(10, 160);
  this->LODResolutionScale->SetResolution(1.0);

  this->LODResolutionValue->SetParent(this->LODScalesFrame);
  this->LODResolutionValue->Create(this->Application, "-anchor w");

  if (pvapp &&
      pvapp->GetRegisteryValue(2, "RunTime", "LODResolution", 0))
    {
    this->LODResolution =
      pvapp->GetIntRegisteryValue(2, "RunTime", "LODResolution");
    }
  this->SetLODResolution(this->LODResolution);
  this->LODResolutionScale->SetValue(150 - this->LODResolution);
  this->LODResolutionScale->SetCommand(this, "LODResolutionLabelCallback");
  this->LODResolutionScale->SetEndCommand(this, "LODResolutionScaleCallback");
  this->LODResolutionScale->SetBalloonHelpString(
    "This slider determines the resolution of the decimated level-of-detail "
    "models. The value is the dimension for each axis in the quadric clustering "
    "algorithm."
    "\nLeft: Use slow high-resolution models. "
    "Right: Use fast simple models .");

  pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                this->LODResolutionValue->GetWidgetName(), row++);
  pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                this->LODResolutionLabel->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                this->LODResolutionScale->GetWidgetName(), row++);

  pvapp->Script("grid columnconfigure %s 2 -weight 1",
                this->LODResolutionScale->GetParent()->GetWidgetName());

  // LOD parameters: rendering interrupts

  this->RenderInterruptsEnabledCheck->SetParent(this->LODFrame->GetFrame());
  this->RenderInterruptsEnabledCheck->Create(this->Application, 
                                             "-text \"Allow rendering interrupts\"");
  this->RenderInterruptsEnabledCheck->SetCommand(this, "RenderInterruptsEnabledCheckCallback");
  
  if (pvapp && pvapp->GetRegisteryValue(2, "RunTime", 
                                        "RenderInterruptsEnabled", 0))
    {
    this->RenderInterruptsEnabled = 
      pvapp->GetIntRegisteryValue(2, "RunTime", "RenderInterruptsEnabled");
    }
  this->RenderInterruptsEnabledCheck->SetState(this->RenderInterruptsEnabled);
  // This call just forwards the value to the render module.
  this->RenderInterruptsEnabledCheckCallback();

  this->RenderInterruptsEnabledCheck->SetBalloonHelpString(
    "Toggle the use of  render interrupts (when using MPI, this uses "
    "asynchronous messaging). When off, renders can not be interrupted.");

  // LOD parameters: pack

  this->Script("pack %s -side top -fill x -expand t -anchor w",
               this->LODScalesFrame->GetWidgetName());
  this->Script("pack %s -side top -anchor w",
               this->RenderInterruptsEnabledCheck->GetWidgetName());


}


//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::LODThresholdScaleCallback()
{
  float threshold = this->LODThresholdScale->GetValue();
  this->SetLODThreshold(threshold);
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::LODCheckCallback()
{
  if (this->LODCheck->GetState())
    {
    float threshold = this->LODThresholdScale->GetValue();
    this->SetLODThreshold(threshold);
    }
  else
    {
    this->SetLODThreshold(VTK_LARGE_FLOAT);
    }
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::LODThresholdLabelCallback()
{
  float threshold = this->LODThresholdScale->GetValue();
  if (threshold == VTK_LARGE_FLOAT)
    {
    this->LODThresholdValue->SetLabel("Disabled");
    }
  else
    {
    char str[256];
    sprintf(str, "%.1f MBytes", threshold);
    this->LODThresholdValue->SetLabel(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetLODThreshold(float threshold)
{
  if ( this->LODThresholdValue && this->LODThresholdValue->IsCreated() )
    {
    if (threshold == VTK_LARGE_FLOAT)
      {
      this->LODThresholdScale->EnabledOff();
      this->LODThresholdValue->EnabledOff();
      this->LODResolutionLabel->EnabledOff();
      this->LODResolutionScale->EnabledOff();
      this->LODResolutionValue->EnabledOff();
      this->LODCheck->SetState(0);
      }
    else
      {
      this->LODThresholdScale->EnabledOn();
      this->LODThresholdValue->EnabledOn();
      this->LODResolutionLabel->EnabledOn();
      this->LODResolutionScale->EnabledOn();
      this->LODResolutionValue->EnabledOn();
      this->LODCheck->SetState(1);
      this->LODThresholdScale->SetValue(threshold);
      }
    this->LODThresholdLabelCallback();
    }
    
  if ( this->LODRenderModule )
    {
    this->LODRenderModule->SetLODThreshold(threshold);
    }
  this->LODThreshold = threshold;

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %d.", 
                                  threshold);
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetLODThreshold %f}",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::LODResolutionScaleCallback()
{
  int value = static_cast<int>(this->LODResolutionScale->GetValue());
  value = 170 - value;

  // Use internal method so we do not reset the slider.
  // I do not know if it would cause a problem, but ...
  this->SetLODResolutionInternal(value);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Resolution %d.", value);
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetLODResolution %d}",
                      this->GetTclName(), value);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::LODResolutionLabelCallback()
{
  int resolution = static_cast<int>(this->LODResolutionScale->GetValue());
  resolution = 170 - resolution;

  char str[256];
  sprintf(str, "%dx%dx%d", resolution, resolution, resolution);
  this->LODResolutionValue->SetLabel(str);
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetLODResolution(int value)
{
  this->LODResolutionScale->SetValue(150 - value);

  this->SetLODResolutionInternal(value);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Resolution %d.", value);
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetLODResolution %d}",
                      this->GetTclName(), value);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetLODResolutionInternal(int resolution)
{
  char str[256];

  sprintf(str, "%dx%dx%d", resolution, resolution, resolution);
  this->LODResolutionValue->SetLabel(str);

  this->LODResolution = resolution;
 
  if ( !this->LODRenderModule )
    {
    return;
    }

  this->LODRenderModule->SetLODResolution(resolution);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::RenderInterruptsEnabledCheckCallback()
{
  this->SetRenderInterruptsEnabled(
    this->RenderInterruptsEnabledCheck->GetState());
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetRenderInterruptsEnabled(int state)
{
  if (this->RenderInterruptsEnabledCheck->GetState() != state)
    {
    this->RenderInterruptsEnabledCheck->SetState(state);
    }
  
  vtkPVApplication* pvApp = this->GetPVApplication();
  this->RenderInterruptsEnabled = state;
  
  pvApp->GetRenderModule()->SetRenderInterruptsEnabled(state);

  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetRenderInterruptsEnabled %d}",
                      this->GetTclName(),
                      this->RenderInterruptsEnabledCheck->GetState());
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SaveState(ofstream *file)
{
  this->Superclass::SaveState(file);

  // We use catches because the paraview loading the state file might not
  // have this module.
  *file << "catch {$kw(" << this->GetTclName() << ") SetLODThreshold "
        << this->GetLODThreshold() << "}" << endl;
  
  *file << "catch {$kw(" << this->GetTclName() << ") SetLODResolution "
        << this->GetLODResolution() << "}" << endl;
  
  *file << "catch {$kw(" << this->GetTclName()
        << ") SetRenderInterruptsEnabled "
        << this->GetPVApplication()->GetRenderModule()->GetRenderInterruptsEnabled()
        << "}" << endl;
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;

  os << indent << "RenderInterruptsEnabled: " 
     << this->RenderInterruptsEnabled << endl;
}
