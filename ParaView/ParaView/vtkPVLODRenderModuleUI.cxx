/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModuleUI.cxx
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
#include "vtkKWTkUtilities.h"
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
vtkCxxRevisionMacro(vtkPVLODRenderModuleUI, "1.2");

int vtkPVLODRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVLODRenderModuleUI::vtkPVLODRenderModuleUI()
{
  this->CommandFunction = vtkPVLODRenderModuleUICommand;
  
  this->LODRenderModule = NULL;
  this->UseReductionFactor = 1;

  this->ParallelRenderParametersFrame = vtkKWLabeledFrame::New();

  this->InterruptRenderCheck = vtkKWCheckButton::New();
  this->CompositeWithFloatCheck = vtkKWCheckButton::New();
  this->CompositeWithRGBACheck = vtkKWCheckButton::New();
  this->CompositeCompressionCheck = vtkKWCheckButton::New();

  this->LODFrame = vtkKWLabeledFrame::New();
 
  this->LODScalesFrame = vtkKWWidget::New();
  this->LODThresholdLabel = vtkKWLabel::New();
  this->LODThresholdScale = vtkKWScale::New();
  this->LODThresholdValue = vtkKWLabel::New();
  this->LODResolutionLabel = vtkKWLabel::New();
  this->LODResolutionScale = vtkKWScale::New();
  this->LODResolutionValue = vtkKWLabel::New();
  this->CollectThresholdLabel = vtkKWLabel::New();
  this->CollectThresholdScale = vtkKWScale::New();
  this->CollectThresholdValue = vtkKWLabel::New();

  this->LODThreshold = 2.0;
  this->LODResolution = 50;
  this->CollectThreshold = 2.0;
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

    if (this->LODRenderModule->GetComposite() || pvapp->GetClientMode())
      {
      pvapp->SetRegisteryValue(2, "RunTime", "CollectThreshold", "%f",
                               this->CollectThreshold);
      pvapp->SetRegisteryValue(2, "RunTime", "InterruptRender", "%d",
                               this->InterruptRenderCheck->GetState());
      pvapp->SetRegisteryValue(2, "RunTime", "UseFloatInComposite", "%d",
                               this->CompositeWithFloatCheck->GetState());
      pvapp->SetRegisteryValue(2, "RunTime", "UseRGBAInComposite", "%d",
                               this->CompositeWithRGBACheck->GetState());
      pvapp->SetRegisteryValue(2, "RunTime", "UseCompressionInComposite", "%d",
                               this->CompositeCompressionCheck->GetState());
      }
    }

  this->LODFrame->Delete();
  this->LODFrame = NULL;

  this->ParallelRenderParametersFrame->Delete();
  this->ParallelRenderParametersFrame = 0;

  this->InterruptRenderCheck->Delete();
  this->InterruptRenderCheck = NULL;

  this->CompositeWithFloatCheck->Delete();
  this->CompositeWithFloatCheck = NULL;

  this->CompositeWithRGBACheck->Delete();
  this->CompositeWithRGBACheck = NULL;
  
  this->CompositeCompressionCheck->Delete();
  this->CompositeCompressionCheck = NULL;

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
  this->LODResolutionScale->Delete();
  this->LODResolutionScale = NULL;
  this->LODResolutionValue->Delete();
  this->LODResolutionValue = NULL;

  this->CollectThresholdLabel->Delete();
  this->CollectThresholdLabel = NULL;
  this->CollectThresholdScale->Delete();
  this->CollectThresholdScale = NULL;
  this->CollectThresholdValue->Delete();
  this->CollectThresholdValue = NULL;

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
    vtkErrorMacro("RenderView already created");
    return;
    }
  
  // Must set the application
  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  this->SetApplication(app);

  // Create this widgets frame.
  this->Script("frame %s -bd 0",this->GetWidgetName());

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

  this->LODThresholdScale->SetParent(this->LODScalesFrame);
  this->LODThresholdScale->Create(this->Application, 
                                  "-resolution 0.1 -orient horizontal");
  this->LODThresholdScale->SetRange(0.0, 6.0);
  this->LODThresholdScale->SetResolution(0.1);

  this->LODThresholdValue->SetParent(this->LODScalesFrame);
  this->LODThresholdValue->Create(this->Application, "-anchor w");

  if (pvapp &&
      pvapp->GetRegisteryValue(2, "RunTime", "LODThreshold", 0))
    {
    this->SetLODThreshold(
      pvapp->GetFloatRegisteryValue(2, "RunTime", "LODThreshold"));
    }
  else
    {
    this->SetLODThreshold(this->LODThreshold);
    }

  this->LODThresholdScale->SetValue(this->CollectThreshold);
  this->LODThresholdScale->SetCommand(this, 
                                      "LODThresholdScaleCallback");
  this->LODThresholdScale->SetBalloonHelpString(
    "This slider determines whether to use decimated models "
    "during interaction.  Threshold critera is based on size "
    "of geometry in mega bytes.  "
    "Left: Always use full resolution. Right: Always use decimated models.");    

  int row = 0;

  pvapp->Script("grid %s -row %d -column 1 -sticky news", 
                this->LODThresholdValue->GetWidgetName(), row++);
  pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                this->LODThresholdLabel->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 1 -sticky news", 
                this->LODThresholdScale->GetWidgetName(), row++);
  
  pvapp->Script("grid columnconfigure %s 1 -weight 1",
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
    this->SetLODResolution(
      pvapp->GetIntRegisteryValue(2, "RunTime", "LODResolution"));
    }
  else
    {
    this->SetLODResolution(this->LODResolution);
    }

  this->LODResolutionScale->SetValue(150 - this->LODResolution);
  this->LODResolutionScale->SetCommand(this, "LODResolutionLabelCallback");
  this->LODResolutionScale->SetEndCommand(this, "LODResolutionScaleCallback");
  this->LODResolutionScale->SetBalloonHelpString(
    "This slider determines the resolution of the decimated level-of-detail "
    "models. The value is the dimension for each axis in the quadric clustering "
    "algorithm."
    "\nLeft: Use slow high-resolution models. "
    "Right: Use fast simple models .");

  pvapp->Script("grid %s -row %d -column 1 -sticky news", 
                this->LODResolutionValue->GetWidgetName(), row++);
  pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                this->LODResolutionLabel->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 1 -sticky news", 
                this->LODResolutionScale->GetWidgetName(), row++);

  // LOD parameters: rendering interrupts

  this->InterruptRenderCheck->SetParent(this->LODFrame->GetFrame());
  this->InterruptRenderCheck->Create(this->Application, 
                                     "-text \"Allow rendering interrupts\"");
  this->InterruptRenderCheck->SetCommand(this, "InterruptRenderCheckCallback");
  
  if (pvapp && pvapp->GetRegisteryValue(2, "RunTime", 
                                        "InterruptRender", 0))
    {
    this->InterruptRenderCheck->SetState(
      pvapp->GetIntRegisteryValue(2, "RunTime", "InterruptRender"));
    }
  else
    {
    this->InterruptRenderCheck->SetState(1);
    }
  this->InterruptRenderCheckCallback();
  this->InterruptRenderCheck->SetBalloonHelpString(
    "Toggle the use of  render interrupts (when using MPI, this uses "
    "asynchronous messaging). When off, renders can not be interrupted.");

  // LOD parameters: pack

  this->Script("pack %s -side top -fill x -expand t -anchor w",
               this->LODScalesFrame->GetWidgetName());
  this->Script("pack %s -side top -anchor w",
               this->InterruptRenderCheck->GetWidgetName());

  // LOD parameters: collection threshold
  // Conditional interface should really be part of a module. !!!!
  if ((pvapp->GetClientMode() || pvapp->GetProcessModule()->GetNumberOfPartitions() > 1) &&
      !pvapp->GetUseRenderingGroup())
    {
    // Determines when geometry is collected to process 0 for rendering.

    this->CollectThresholdLabel->SetParent(this->LODScalesFrame);
    this->CollectThresholdLabel->Create(this->Application, "-anchor w");
    this->CollectThresholdLabel->SetLabel("Collection threshold:");

    this->CollectThresholdScale->SetParent(this->LODScalesFrame);
    this->CollectThresholdScale->Create(this->Application,
                                        "-orient horizontal");
    this->CollectThresholdScale->SetRange(0.0, 6.0);
    this->CollectThresholdScale->SetResolution(0.1);

    this->CollectThresholdValue->SetParent(this->LODScalesFrame);
    this->CollectThresholdValue->Create(this->Application, "-anchor w");
    if (pvapp &&
        pvapp->GetRegisteryValue(2, "RunTime", "CollectThreshold", 0))
      {
      this->SetCollectThreshold(
        pvapp->GetFloatRegisteryValue(2, "RunTime", "CollectThreshold"));
      }
    else
      {
      this->SetCollectThreshold(this->CollectThreshold);
      }

    this->CollectThresholdScale->SetValue(this->CollectThreshold);
    this->CollectThresholdScale->SetCommand(this, 
                                            "CollectThresholdScaleCallback");
    this->CollectThresholdScale->SetBalloonHelpString(
      "This slider determines when models are collected to process 0 for "
      "local rendering. Threshold critera is based on size of model in mega "
      "bytes.  "
      "Left: Always leave models distributed. Right: Move even large models "
      "to process 0.");    

    pvapp->Script("grid %s -row %d -column 1 -sticky news", 
                  this->CollectThresholdValue->GetWidgetName(), row++);
    pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                  this->CollectThresholdLabel->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 1 -sticky news", 
                  this->CollectThresholdScale->GetWidgetName(), row++);
    }
  // Parallel rendering parameters
  // Conditional interface should really be part of a module !!!!!!
  if (pvapp->GetProcessModule()->GetNumberOfPartitions() > 1 && 
      this->LODRenderModule->GetComposite())
    {
    this->ParallelRenderParametersFrame->SetParent(this); 
    this->ParallelRenderParametersFrame->ShowHideFrameOn();
    this->ParallelRenderParametersFrame->Create(this->Application,0);
    this->ParallelRenderParametersFrame->SetLabel(
      "Parallel Rendering Parameters");

    this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
                 this->ParallelRenderParametersFrame->GetWidgetName());
  
    this->CompositeWithFloatCheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeWithFloatCheck->Create(this->Application, 
                                          "-text \"Composite with floats\"");

    this->CompositeWithRGBACheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeWithRGBACheck->Create(this->Application, 
                                         "-text \"Composite RGBA\"");

    this->CompositeCompressionCheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeCompressionCheck->Create(this->Application, 
                                            "-text \"Composite compression\"");
  
    this->CompositeWithFloatCheck->SetCommand(this, 
                                              "CompositeWithFloatCallback");
    if (pvapp && 
        pvapp->GetRegisteryValue(2, "RunTime", "UseFloatInComposite", 0))
      {
      this->CompositeWithFloatCheck->SetState(
        pvapp->GetIntRegisteryValue(2, "RunTime", "UseFloatInComposite"));
      this->CompositeWithFloatCallback();
      }
    else
      {
      this->CompositeWithFloatCheck->SetState(0);
      }
    this->CompositeWithFloatCheck->SetBalloonHelpString(
      "Toggle the use of char/float values when compositing. "
      "If rendering defects occur, try turning this on.");
  
    this->CompositeWithRGBACheck->SetCommand(this, "CompositeWithRGBACallback");
    if (pvapp && pvapp->GetRegisteryValue(2, "RunTime", 
                                          "UseRGBAInComposite", 0))
      {
      this->CompositeWithRGBACheck->SetState(
        pvapp->GetIntRegisteryValue(2, "RunTime", "UseRGBAInComposite"));
      this->CompositeWithRGBACallback();
      }
    else
      {
      this->CompositeWithRGBACheck->SetState(0);
      }
    this->CompositeWithRGBACheck->SetBalloonHelpString(
      "Toggle the use of RGB/RGBA values when compositing. "
      "This is here to bypass some bugs in some graphics card drivers.");

    this->CompositeCompressionCheck->SetCommand(this, 
                                               "CompositeCompressionCallback");
    if (pvapp && 
        pvapp->GetRegisteryValue(2, "RunTime", "UseCompressionInComposite", 0))
      {
      this->CompositeCompressionCheck->SetState(
        pvapp->GetIntRegisteryValue(2, "RunTime", 
                                       "UseCompressionInComposite"));
      this->CompositeCompressionCallback();
      }
    else
      {
      this->CompositeCompressionCheck->SetState(1);
      }
    this->CompositeCompressionCheck->SetBalloonHelpString(
      "Toggle the use of run length encoding when compositing. "
      "This is here to compare performance.  "
      "It should not change the final rendered image.");
  
    this->Script("pack %s %s %s -side top -anchor w",
                 this->CompositeWithFloatCheck->GetWidgetName(),
                 this->CompositeWithRGBACheck->GetWidgetName(),
                 this->CompositeCompressionCheck->GetWidgetName());
    }

}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVLODRenderModuleUI::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::LODThresholdScaleCallback()
{
  float value = this->LODThresholdScale->GetValue();
  float threshold;

  // Value should be between 0 and 18.
  // producing threshold between 65Mil and 1.
  //threshold = static_cast<int>(exp(18.0 - value));
  threshold = value;  

  // Use internal method so we do not reset the slider.
  // I do not know if it would cause a problem, but ...
  this->SetLODThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %d.", 
                                  threshold);
  this->AddTraceEntry("$kw(%s) SetLODThreshold %d",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetLODThreshold(float threshold)
{
  float value;
  
  //if (threshold <= 0.0)
  //  {
  //  value = VTK_LARGE_FLOAT;
  //  }
  //else
  //  {
  //  value = 18.0 - log((double)(threshold));
  //  }
  value = threshold;
  this->LODThresholdScale->SetValue(value);

  this->SetLODThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %d.", 
                                  threshold);
  this->AddTraceEntry("$kw(%s) SetLODThreshold %d",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetLODThresholdInternal(float threshold)
{
  char str[256];

  sprintf(str, "%.1f MBytes", threshold);
  this->LODThresholdValue->SetLabel(str);
  this->LODRenderModule->SetLODThreshold(threshold);

  this->LODThreshold = threshold;
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
  this->AddTraceEntry("$kw(%s) SetLODResolution %d",
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
  this->AddTraceEntry("$kw(%s) SetLODResolution %d",
                      this->GetTclName(), value);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetLODResolutionInternal(int resolution)
{
  vtkPVApplication *pvApp;
  char str[256];

  sprintf(str, "%dx%dx%d", resolution, resolution, resolution);
  this->LODResolutionValue->SetLabel(str);

  this->LODResolution = resolution;
 
  pvApp = this->GetPVApplication();
  this->LODRenderModule->SetLODResolution(resolution);
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CollectThresholdScaleCallback()
{
  float threshold = this->CollectThresholdScale->GetValue();

  // Use internal method so we do not reset the slider.
  // I do not know if it would cause a problem, but ...
  this->SetCollectThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  this->AddTraceEntry("$kw(%s) SetCollectThreshold %f",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetCollectThreshold(float threshold)
{
  this->CollectThresholdScale->SetValue(threshold);

  this->SetCollectThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  this->AddTraceEntry("$kw(%s) SetCollectThreshold %f",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::SetCollectThresholdInternal(float threshold)
{
  vtkPVApplication *pvApp;
  char str[256];

  sprintf(str, "%.1f MBytes", threshold);
  this->CollectThresholdValue->SetLabel(str);

  this->CollectThreshold = threshold;

  // This will cause collection to be re evaluated.
  pvApp = this->GetPVApplication();
  pvApp->SetTotalVisibleMemorySizeValid(0);
  this->LODRenderModule->SetCollectThreshold(threshold);

}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::InterruptRenderCheckCallback()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  pvApp->GetRenderModule()->SetRenderInterruptsEnabled(
                                   this->InterruptRenderCheck->GetState());
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CompositeWithFloatCallback()
{
  int val = this->CompositeWithFloatCheck->GetState();
  this->CompositeWithFloatCallback(val);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CompositeWithFloatCallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeWithFloatCallback %d", 
                      this->GetTclName(), val);
  if ( this->CompositeWithFloatCheck->GetState() != val )
    {
    this->CompositeWithFloatCheck->SetState(val);
    }
 
  if (this->LODRenderModule->GetComposite())
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseChar %d",
                                              this->LODRenderModule->GetCompositeTclName(),
                                              !val);
    // Limit of composite manager.
    if (val != 0) // float
      {
      this->CompositeWithRGBACheck->SetState(1);
      }
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    }

  if (this->CompositeWithFloatCheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as floats.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as unsigned char.");
    }

}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CompositeWithRGBACallback()
{
  int val = this->CompositeWithRGBACheck->GetState();
  this->CompositeWithRGBACallback(val);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CompositeWithRGBACallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeWithRGBACallback %d", 
                      this->GetTclName(), val);
  if ( this->CompositeWithRGBACheck->GetState() != val )
    {
    this->CompositeWithRGBACheck->SetState(val);
    }
  if (this->LODRenderModule->GetComposite())
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseRGB %d",
                                              this->LODRenderModule->GetCompositeTclName(),
                                              !val);
    // Limit of composite manager.
    if (val != 1) // RGB
      {
      this->CompositeWithFloatCheck->SetState(0);
      }
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    }

  if (this->CompositeWithRGBACheck->GetState())
    {
    vtkTimerLog::MarkEvent("--- Use RGBA pixels to get color buffers.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Use RGB pixels to get color buffers.");
    }
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CompositeCompressionCallback()
{
  int val = this->CompositeCompressionCheck->GetState();
  this->CompositeCompressionCallback(val);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::CompositeCompressionCallback(int val)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->AddTraceEntry("$kw(%s) CompositeCompressionCallback %d", 
                      this->GetTclName(), val);
  if ( this->CompositeCompressionCheck->GetState() != val )
    {
    this->CompositeCompressionCheck->SetState(val);
    }
  if (this->LODRenderModule->GetComposite())
    {
    if (val)
      {
      pvApp->BroadcastScript("vtkCompressCompositer pvTemp");
      }
    else
      {
      pvApp->BroadcastScript("vtkTreeCompositer pvTemp");
      }
    pvApp->BroadcastScript("%s SetCompositer pvTemp", 
                           this->LODRenderModule->GetCompositeTclName());
    pvApp->BroadcastScript("pvTemp Delete");
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable compression when compositing.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable compression when compositing.");
    }

}



//----------------------------------------------------------------------------
void vtkPVLODRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;
}

