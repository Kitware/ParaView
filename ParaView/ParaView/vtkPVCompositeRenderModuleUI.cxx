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
vtkCxxRevisionMacro(vtkPVCompositeRenderModuleUI, "1.2.2.3");

int vtkPVCompositeRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVCompositeRenderModuleUI::vtkPVCompositeRenderModuleUI()
{
  this->CommandFunction = vtkPVCompositeRenderModuleUICommand;
  
  this->CompositeRenderModule = NULL;

  this->ParallelRenderParametersFrame = vtkKWLabeledFrame::New();

  this->CompositeWithFloatCheck = vtkKWCheckButton::New();
  this->CompositeWithRGBACheck = vtkKWCheckButton::New();
  this->CompositeCompressionCheck = vtkKWCheckButton::New();

  this->CollectLabel = vtkKWLabel::New();
  this->CollectCheck = vtkKWCheckButton::New();
  this->CollectThresholdScale = vtkKWScale::New();
  this->CollectThresholdLabel = vtkKWLabel::New();
  this->CollectThreshold = 20.0;

  this->SquirtLabel = vtkKWLabel::New();
  this->SquirtCheck = vtkKWCheckButton::New();
  this->SquirtLevelScale = vtkKWScale::New();
  this->SquirtLevelLabel = vtkKWLabel::New();
  this->SquirtLevel = 3;

  this->ReductionLabel = vtkKWLabel::New();
  this->ReductionCheck = vtkKWCheckButton::New();
  this->ReductionFactorScale = vtkKWScale::New();
  this->ReductionFactorLabel = vtkKWLabel::New();
  this->ReductionFactor = 2;


  this->CompositeWithFloatFlag = 0;
  this->CompositeWithRGBAFlag = 0;
  this->CompositeCompressionFlag = 1;
}


//----------------------------------------------------------------------------
vtkPVCompositeRenderModuleUI::~vtkPVCompositeRenderModuleUI()
{
  // Save UI values in registery.
  vtkPVApplication* pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegisteryValue(2, "RunTime", "RenderInterruptsEnabled", "%d",
                             this->RenderInterruptsEnabled);
    pvapp->SetRegisteryValue(2, "RunTime", "UseFloatInComposite", "%d",
                             this->CompositeWithFloatFlag);
    pvapp->SetRegisteryValue(2, "RunTime", "UseRGBAInComposite", "%d",
                             this->CompositeWithRGBAFlag);
    pvapp->SetRegisteryValue(2, "RunTime", "UseCompressionInComposite", "%d",
                             this->CompositeCompressionFlag);

    pvapp->SetRegisteryValue(2, "RunTime", "CollectThreshold", "%f",
                             this->CollectThreshold);
    pvapp->SetRegisteryValue(2, "RunTime", "ReductionFactor", "%d",
                             this->ReductionFactor);
    pvapp->SetRegisteryValue(2, "RunTime", "SquirtLevel", "%d",
                             this->SquirtLevel);
    }

  this->ParallelRenderParametersFrame->Delete();
  this->ParallelRenderParametersFrame = 0;

  this->CompositeWithFloatCheck->Delete();
  this->CompositeWithFloatCheck = NULL;

  this->CompositeWithRGBACheck->Delete();
  this->CompositeWithRGBACheck = NULL;
  
  this->CompositeCompressionCheck->Delete();
  this->CompositeCompressionCheck = NULL;

  this->CollectLabel->Delete();
  this->CollectLabel = NULL;
  this->CollectCheck->Delete();
  this->CollectCheck = NULL;
  this->CollectThresholdScale->Delete();
  this->CollectThresholdScale = NULL;
  this->CollectThresholdLabel->Delete();
  this->CollectThresholdLabel = NULL;

  this->ReductionLabel->Delete();
  this->ReductionLabel = NULL;
  this->ReductionCheck->Delete();
  this->ReductionCheck = NULL;
  this->ReductionFactorScale->Delete();
  this->ReductionFactorScale = NULL;
  this->ReductionFactorLabel->Delete();
  this->ReductionFactorLabel = NULL;

  this->SquirtLabel->Delete();
  this->SquirtLabel = NULL;
  this->SquirtCheck->Delete();
  this->SquirtCheck = NULL;
  this->SquirtLevelScale->Delete();
  this->SquirtLevelScale = NULL;
  this->SquirtLevelLabel->Delete();
  this->SquirtLevelLabel = NULL;

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
    this->CollectLabel->SetParent(this->LODScalesFrame);
    this->CollectLabel->Create(this->Application, "-anchor w");
    this->CollectLabel->SetLabel("Collection threshold:");

    this->CollectCheck->SetParent(this->LODScalesFrame);
    this->CollectCheck->Create(this->Application, "");
    this->CollectCheck->SetState(1);
    this->CollectCheck->SetCommand(this, "CollectCheckCallback");

    this->CollectThresholdScale->SetParent(this->LODScalesFrame);
    this->CollectThresholdScale->Create(this->Application,
                                        "-orient horizontal");
    this->CollectThresholdScale->SetRange(0.1, 100.0);
    this->CollectThresholdScale->SetResolution(0.1);
    this->CollectThresholdScale->SetValue(this->CollectThreshold);
    this->CollectThresholdScale->SetCommand(this, 
                                            "CollectThresholdScaleCallback");
    this->CollectThresholdScale->SetBalloonHelpString(
      "This slider determines when models are collected to process 0 for "
      "local rendering. Threshold critera is based on size of model in mega "
      "bytes.  "
      "Left: Always leave models distributed. Right: Move even large models "
      "to process 0.");    

    this->CollectThresholdLabel->SetParent(this->LODScalesFrame);
    this->CollectThresholdLabel->Create(this->Application, "-anchor w");
    if (pvapp &&
        pvapp->GetRegisteryValue(2, "RunTime", "CollectThreshold", 0))
      {
      this->CollectThreshold = 
        pvapp->GetFloatRegisteryValue(2, "RunTime", "CollectThreshold");
      }

    // Force the set.
    float tmp = this->CollectThreshold;
    this->CollectThreshold = -1.0;
    this->SetCollectThreshold(tmp);

    pvapp->Script("grid %s -row %d -column 2 -sticky nws", 
                  this->CollectThresholdLabel->GetWidgetName(), row++);
    pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                  this->CollectLabel->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 1 -sticky nes", 
                  this->CollectCheck->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                  this->CollectThresholdScale->GetWidgetName(), row++);

    pvapp->Script("grid columnconfigure %s 2 -weight 1",
                  this->CollectThresholdScale->GetParent()->GetWidgetName());


    // Determines which reduction/subsampling factor to use.
    this->ReductionLabel->SetParent(this->LODScalesFrame);
    this->ReductionLabel->Create(this->Application, "-anchor w");
    this->ReductionLabel->SetLabel("Subsample Rate:");

    this->ReductionCheck->SetParent(this->LODScalesFrame);
    this->ReductionCheck->Create(this->Application, "");
    this->ReductionCheck->SetState(1);
    this->ReductionCheck->SetCommand(this, "ReductionCheckCallback");

    this->ReductionFactorScale->SetParent(this->LODScalesFrame);
    this->ReductionFactorScale->Create(this->Application,
                                        "-orient horizontal");
    this->ReductionFactorScale->SetRange(2, 5);
    this->ReductionFactorScale->SetResolution(1);
    this->ReductionFactorScale->SetValue(this->ReductionFactor);
    this->ReductionFactorScale->SetCommand(this, 
                                            "ReductionFactorScaleCallback");
    this->ReductionFactorScale->SetBalloonHelpString(
             "Subsampling is a compositing LOD technique. "
             "Subsampling will use larger pixels during interaction.");

    this->ReductionFactorLabel->SetParent(this->LODScalesFrame);
    this->ReductionFactorLabel->SetLabel("2 Pixels");
    this->ReductionFactorLabel->Create(this->Application, "-anchor w");
    if (pvapp &&
        pvapp->GetRegisteryValue(2, "RunTime", "ReductionFactor", 0))
      {
      this->SetReductionFactor(
        pvapp->GetIntRegisteryValue(2, "RunTime", "ReductionFactor"));
      }
    else
      {
      this->SetReductionFactor(this->ReductionFactor);
      }

    pvapp->Script("grid %s -row %d -column 2 -sticky nws", 
                  this->ReductionFactorLabel->GetWidgetName(), row++);
    pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                  this->ReductionLabel->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 1 -sticky nes", 
                  this->ReductionCheck->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                  this->ReductionFactorScale->GetWidgetName(), row++);


    // Determines whether to squirt and what compression level.
    this->SquirtLabel->SetParent(this->LODScalesFrame);
    this->SquirtLabel->Create(this->Application, "-anchor w");
    this->SquirtLabel->SetLabel("Squirt Compression:");

    this->SquirtCheck->SetParent(this->LODScalesFrame);
    this->SquirtCheck->Create(this->Application, "");
    this->SquirtCheck->SetState(1);
    this->SquirtCheck->SetCommand(this, "SquirtCheckCallback");

    this->SquirtLevelScale->SetParent(this->LODScalesFrame);
    this->SquirtLevelScale->Create(this->Application,
                                        "-orient horizontal");
    this->SquirtLevelScale->SetRange(1, 6);
    this->SquirtLevelScale->SetResolution(1);
    this->SquirtLevelScale->SetValue(this->SquirtLevel);
    this->SquirtLevelScale->SetCommand(this, 
                                            "SquirtLevelScaleCallback");

    this->SquirtLevelLabel->SetParent(this->LODScalesFrame);
    this->SquirtLevelLabel->Create(this->Application, "-anchor w");
    if (pvapp &&
        pvapp->GetRegisteryValue(2, "RunTime", "SquirtLevel", 0))
      {
      this->SquirtLevel = 
        pvapp->GetIntRegisteryValue(2, "RunTime", "SquirtLevel");
      }

    if (pvapp->GetClientMode() && ! pvapp->GetUseTiledDisplay())
      {
      this->SquirtLevelScale->SetBalloonHelpString(
        "Squirt is a combinination of runlength encoding and bit compression.");
      pvapp->Script("grid %s -row %d -column 2 -sticky nws", 
                    this->SquirtLevelLabel->GetWidgetName(), row++);
      pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                    this->SquirtLabel->GetWidgetName(), row);
      pvapp->Script("grid %s -row %d -column 1 -sticky nes", 
                    this->SquirtCheck->GetWidgetName(), row);
      pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                    this->SquirtLevelScale->GetWidgetName(), row++);
      // Force initialize.
      int tmp = this->SquirtLevel;
      this->SquirtLevel = -1;
      this->SetSquirtLevel(tmp);
      }
    else
      {
      this->SquirtLevelScale->SetBalloonHelpString(
        "Squirt only an option when running client server mode."
        "Squirt is not used for tiled displays");
      this->SquirtCheck->SetState(0);
      this->SquirtLabel->EnabledOff();
      this->SquirtCheck->EnabledOff();
      this->SquirtLevelScale->EnabledOff();
      this->SquirtLevelLabel->EnabledOff();
      }
    }
  // Parallel rendering parameters
  // Conditional interface should really be part of a module !!!!!!
  if (pvapp->GetProcessModule()->GetNumberOfPartitions() > 1)
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

  if (threshold == 0.0)
    {
    threshold = 0.1;
    }
  this->SetCollectThreshold(threshold);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CollectCheckCallback()
{
  int val = this->CollectCheck->GetState();

  if (val == 0)
    {
    this->SetCollectThreshold(0.0);
    }
  else
    {
    float threshold = this->CollectThresholdScale->GetValue();
    if (threshold == 0.0)
      {
      threshold = 1.0;
      }
    this->SetCollectThreshold(threshold);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetCollectThreshold(float threshold)
{
  if (this->CollectThreshold == threshold)
    {
    return;
    }

  if (threshold == 0.0)
    {
    this->CollectCheck->SetState(0);
    this->CollectThresholdScale->EnabledOff();
    this->CollectThresholdScale->SetWidth(2);    
    this->CollectThresholdLabel->EnabledOff();
    this->CollectThresholdLabel->SetLabel("0 MBytes");
    }
  else
    {
    char str[256];
    this->CollectCheck->SetState(1);
    this->CollectThresholdScale->EnabledOn();
    this->CollectThresholdScale->SetWidth(15);    
    this->CollectThresholdLabel->EnabledOn();
    this->CollectThresholdScale->SetValue(threshold);
    sprintf(str, "%.1f MBytes", threshold);
    this->CollectThresholdLabel->SetLabel(str);
    }

  this->CollectThreshold = threshold;


  this->SetCollectThresholdInternal(threshold);
  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  this->AddTraceEntry("$kw(%s) SetCollectThreshold %f",
                      this->GetTclName(), threshold);
}

//----------------------------------------------------------------------------
// Eliminate this method ...........!!!!!!!!!!!!!!
void vtkPVCompositeRenderModuleUI::SetCollectThresholdInternal(float threshold)
{
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
  this->AddTraceEntry("$kw(%s) CompositeCompressionCallback %d", 
                      this->GetTclName(), val);

  this->CompositeCompressionFlag = val;
  if ( this->CompositeCompressionCheck->GetState() != val )
    {
    this->CompositeCompressionCheck->SetState(val);
    }

  // Let the render module do what it needs to.
  this->CompositeRenderModule->SetUseCompositeCompression(val);

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
void vtkPVCompositeRenderModuleUI::SquirtLevelScaleCallback()
{
  int val = (int)(this->SquirtLevelScale->GetValue());
  this->SetSquirtLevel(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SquirtCheckCallback()
{
  int val = this->SquirtCheck->GetState();
  if (val)
    {
    val = (int)(this->SquirtLevelScale->GetValue());
    }
  this->SetSquirtLevel(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetSquirtLevel(int level)
{
  if (this->SquirtLevel == level)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) SetSquirtLevel %d", 
                      this->GetTclName(), level);
  this->SquirtLevel = level;

  if (level == 0)
    {
    this->SquirtLevelScale->EnabledOff();
    this->SquirtLevelScale->SetWidth(2);    
    this->SquirtLevelLabel->EnabledOff();
    this->SquirtCheck->SetState(0);
    this->SquirtLevelLabel->SetLabel("24 Bits-disabled");
    vtkTimerLog::MarkEvent("--- Squirt disabled.");
    }
  else
    {
    this->SquirtLevelScale->EnabledOn();
    this->SquirtLevelScale->SetWidth(15);    
    this->SquirtLevelLabel->EnabledOn();
    this->SquirtLevelScale->SetValue(level);
    this->SquirtCheck->SetState(1);
    switch(level)
      {
      case 1:
        this->SquirtLevelLabel->SetLabel("24 Bits");
        break;
      case 2:
        this->SquirtLevelLabel->SetLabel("22 Bits");
        break;
      case 3:
        this->SquirtLevelLabel->SetLabel("19 Bits");
        break;
      case 4:
        this->SquirtLevelLabel->SetLabel("16 Bits");
        break;
      case 5:
        this->SquirtLevelLabel->SetLabel("13 Bits");
        break;
      case 6:
        this->SquirtLevelLabel->SetLabel("10 Bits");
        break;
      }

    vtkTimerLog::FormatAndMarkEvent("--- Squirt level %d.", level);
    }
 
  if (this->CompositeRenderModule)
    {
    this->CompositeRenderModule->SetSquirtLevel(level);
    }
}



//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::ReductionFactorScaleCallback()
{
  int val = (int)(this->ReductionFactorScale->GetValue());
  this->SetReductionFactor(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::ReductionCheckCallback()
{
  int val = this->ReductionCheck->GetState();
  if (val)
    {
    val = (int)(this->ReductionFactorScale->GetValue());
    }
  else
    { // value of 1 is disabled.
    val = 1;
    }
  this->SetReductionFactor(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetReductionFactor(int factor)
{
  if (this->ReductionFactor == factor)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) SetReductionFactor %d", 
                      this->GetTclName(), factor);
  this->ReductionFactor = factor;

  if (factor == 1)
    {
    this->ReductionFactorScale->EnabledOff();
    this->ReductionFactorScale->SetWidth(2);    
    this->ReductionFactorScale->EnabledOff();
    this->ReductionCheck->SetState(0);
    vtkTimerLog::MarkEvent("--- Reduction disabled.");
    }
  else
    {
    this->ReductionFactorScale->EnabledOn();
    this->ReductionFactorScale->SetWidth(15);    
    this->ReductionFactorLabel->EnabledOn();
    this->ReductionFactorScale->SetValue(factor);
    this->ReductionCheck->SetState(1);
    vtkTimerLog::FormatAndMarkEvent("--- Reduction factor %d.", factor);
    }
  char str[128];
  sprintf(str, "%d Pixels", factor);
  this->ReductionFactorLabel->SetLabel(str); 


  if (this->CompositeRenderModule)
    {
    this->CompositeRenderModule->SetReductionFactor(factor);
    }
}




//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;
  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;

  os << indent << "CompositeWithFloatFlag: " 
     << this->CompositeWithFloatFlag << endl;
  os << indent << "CompositeWithRGBAFlag: " 
     << this->CompositeWithRGBAFlag << endl;
  os << indent << "CompositeCompressionFlag: " 
     << this->CompositeCompressionFlag << endl;
}

