/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModuleUI.cxx
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
#include "vtkPVCompositeRenderModuleUI.h"
#include "vtkPVCompositeRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkKWFrame.h"
#include "vtkTimerLog.h"
#include "vtkPVRenderView.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositeRenderModuleUI);
vtkCxxRevisionMacro(vtkPVCompositeRenderModuleUI, "1.1");

int vtkPVCompositeRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVCompositeRenderModuleUI::vtkPVCompositeRenderModuleUI()
{
  this->CommandFunction = vtkPVCompositeRenderModuleUICommand;
  
  this->CompositeRenderModule = NULL;

  this->UseReductionFactor = 1;
  this->ParallelRenderParametersFrame = vtkKWLabeledFrame::New();

  this->CompositeWithFloatCheck = vtkKWCheckButton::New();
  this->CompositeWithRGBACheck = vtkKWCheckButton::New();
  this->CompositeCompressionCheck = vtkKWCheckButton::New();

  this->CollectThresholdLabel = vtkKWLabel::New();
  this->CollectThresholdScale = vtkKWScale::New();
  this->CollectThresholdValue = vtkKWLabel::New();

  this->CollectThreshold = 4.0;

  this->CompositeWithFloatFlag = 0;
  this->CompositeWithRGBAFlag = 0;
  this->CompositeCompressionFlag = 1;
}


//----------------------------------------------------------------------------
vtkPVCompositeRenderModuleUI::~vtkPVCompositeRenderModuleUI()
{
  // Save UI values in registry.
  vtkPVApplication* pvapp = this->GetPVApplication();
  if (pvapp)
    {
    if (this->CompositeRenderModule->GetComposite() || pvapp->GetClientMode())
      {
      pvapp->SetRegisteryValue(2, "RunTime", "CollectThreshold", "%f",
                               this->CollectThreshold);
      pvapp->SetRegisteryValue(2, "RunTime", "RenderInterruptsEnabled", "%d",
                               this->RenderInterruptsEnabled);
      pvapp->SetRegisteryValue(2, "RunTime", "UseFloatInComposite", "%d",
                               this->CompositeWithFloatFlag);
      pvapp->SetRegisteryValue(2, "RunTime", "UseRGBAInComposite", "%d",
                               this->CompositeWithRGBAFlag);
      pvapp->SetRegisteryValue(2, "RunTime", "UseCompressionInComposite", "%d",
                               this->CompositeCompressionFlag);
      }
    }

  this->ParallelRenderParametersFrame->Delete();
  this->ParallelRenderParametersFrame = 0;

  this->CompositeWithFloatCheck->Delete();
  this->CompositeWithFloatCheck = NULL;

  this->CompositeWithRGBACheck->Delete();
  this->CompositeWithRGBACheck = NULL;
  
  this->CompositeCompressionCheck->Delete();
  this->CompositeCompressionCheck = NULL;

  this->CollectThresholdLabel->Delete();
  this->CollectThresholdLabel = NULL;
  this->CollectThresholdScale->Delete();
  this->CollectThresholdScale = NULL;
  this->CollectThresholdValue->Delete();
  this->CollectThresholdValue = NULL;

  if (this->CompositeRenderModule)
    {
    this->CompositeRenderModule->UnRegister(this);
    this->CompositeRenderModule = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetRenderModule(vtkPVRenderModule* rm)
{
  // Super class has a duplicate pointer.
  this->Superclass::SetRenderModule(rm);

  if (this->CompositeRenderModule)
    {
    this->CompositeRenderModule->UnRegister(this);
    this->CompositeRenderModule = NULL;
    }
  this->CompositeRenderModule = vtkPVCompositeRenderModule::SafeDownCast(rm);
  if (this->CompositeRenderModule)
    {
    this->CompositeRenderModule->Register(this);
    }

  if (rm != NULL && this->CompositeRenderModule == NULL)
    {
    vtkErrorMacro("Expecting a CompositeRenderModule.");
    }
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  vtkPVApplication *pvapp = vtkPVApplication::SafeDownCast(app);
  // Skip over LOD res and threshold.
  int row = 4;
  
  if (this->Application)
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }
  this->Superclass::Create(app, NULL);

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
    this->CollectThresholdScale->SetRange(0.0, 20.0);
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
      this->CompositeRenderModule->GetComposite())
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
      this->CompositeWithFloatFlag =
        pvapp->GetIntRegisteryValue(2, "RunTime", "UseFloatInComposite");
      }
    this->CompositeWithFloatCheck->SetState(this->CompositeWithFloatFlag);
    this->CompositeWithFloatCallback();
    this->CompositeWithFloatCheck->SetBalloonHelpString(
      "Toggle the use of char/float values when compositing. "
      "If rendering defects occur, try turning this on.");
  
    this->CompositeWithRGBACheck->SetCommand(this, "CompositeWithRGBACallback");
    if (pvapp && pvapp->GetRegisteryValue(2, "RunTime", 
                                          "UseRGBAInComposite", 0))
      {
      this->CompositeWithRGBAFlag =
        pvapp->GetIntRegisteryValue(2, "RunTime", "UseRGBAInComposite");
      }
    this->CompositeWithRGBACheck->SetState(this->CompositeWithRGBAFlag);
    this->CompositeWithRGBACallback();
    this->CompositeWithRGBACheck->SetBalloonHelpString(
      "Toggle the use of RGB/RGBA values when compositing. "
      "This is here to bypass some bugs in some graphics card drivers.");

    this->CompositeCompressionCheck->SetCommand(this, 
                                               "CompositeCompressionCallback");
    if (pvapp && 
        pvapp->GetRegisteryValue(2, "RunTime", "UseCompressionInComposite", 0))
      {
      this->CompositeCompressionFlag = 
        pvapp->GetIntRegisteryValue(2, "RunTime", "UseCompressionInComposite");
      }
    this->CompositeCompressionCheck->SetState(this->CompositeCompressionFlag);
    this->CompositeCompressionCallback();
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
void vtkPVCompositeRenderModuleUI::CollectThresholdScaleCallback()
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
void vtkPVCompositeRenderModuleUI::SetCollectThreshold(float threshold)
{
  this->CollectThresholdScale->SetValue(threshold);

  this->SetCollectThresholdInternal(threshold);

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  this->AddTraceEntry("$kw(%s) SetCollectThreshold %f",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetCollectThresholdInternal(float threshold)
{
  char str[256];

  sprintf(str, "%.1f MBytes", threshold);
  this->CollectThresholdValue->SetLabel(str);

  this->CollectThreshold = threshold;

  // This will cause collection to be re evaluated.
  this->CompositeRenderModule->SetTotalVisibleMemorySizeValid(0);
  this->CompositeRenderModule->SetCollectThreshold(threshold);

}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeWithFloatCallback()
{
  int val = this->CompositeWithFloatCheck->GetState();
  this->CompositeWithFloatCallback(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeWithFloatCallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeWithFloatCallback %d", 
                      this->GetTclName(), val);
  this->CompositeWithFloatFlag = val;
  if ( this->CompositeWithFloatCheck->GetState() != val )
    {
    this->CompositeWithFloatCheck->SetState(val);
    }
 
  if (this->CompositeRenderModule->GetComposite())
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseChar %d",
                                        this->CompositeRenderModule->GetCompositeTclName(),
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
void vtkPVCompositeRenderModuleUI::CompositeWithRGBACallback()
{
  int val = this->CompositeWithRGBACheck->GetState();
  this->CompositeWithRGBACallback(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeWithRGBACallback(int val)
{
  this->AddTraceEntry("$kw(%s) CompositeWithRGBACallback %d", 
                      this->GetTclName(), val);
  this->CompositeWithRGBAFlag = val;
  if ( this->CompositeWithRGBACheck->GetState() != val )
    {
    this->CompositeWithRGBACheck->SetState(val);
    }
  if (this->CompositeRenderModule->GetComposite())
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseRGB %d",
                                   this->CompositeRenderModule->GetCompositeTclName(),
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
void vtkPVCompositeRenderModuleUI::CompositeCompressionCallback()
{
  int val = this->CompositeCompressionCheck->GetState();
  this->CompositeCompressionCallback(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeCompressionCallback(int val)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->AddTraceEntry("$kw(%s) CompositeCompressionCallback %d", 
                      this->GetTclName(), val);
  this->CompositeCompressionFlag = val;
  if ( this->CompositeCompressionCheck->GetState() != val )
    {
    this->CompositeCompressionCheck->SetState(val);
    }
  if (this->CompositeRenderModule->GetComposite())
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
                           this->CompositeRenderModule->GetCompositeTclName());
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
void vtkPVCompositeRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;

  os << indent << "CompositeWithFloatFlag: " 
     << this->CompositeWithFloatFlag << endl;
  os << indent << "CompositeWithRGBAFlag: " 
     << this->CompositeWithRGBAFlag << endl;
  os << indent << "CompositeCompressionFlag: " 
     << this->CompositeCompressionFlag << endl;
}

