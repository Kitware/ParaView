/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkKWFrame.h"
#include "vtkTimerLog.h"
#include "vtkPVRenderView.h"
#include "vtkPVServerInformation.h"
#include "vtkPVOptions.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositeRenderModuleUI);
vtkCxxRevisionMacro(vtkPVCompositeRenderModuleUI, "1.22.2.1");

int vtkPVCompositeRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVCompositeRenderModuleUI::vtkPVCompositeRenderModuleUI()
{
  this->CommandFunction = vtkPVCompositeRenderModuleUICommand;
  
  this->ParallelRenderParametersFrame = vtkKWFrameLabeled::New();

  this->CompositeWithFloatCheck = vtkKWCheckButton::New();
  this->CompositeWithRGBACheck = vtkKWCheckButton::New();
  this->CompositeCompressionCheck = vtkKWCheckButton::New();

  this->CompositeLabel = vtkKWLabel::New();
  this->CompositeCheck = vtkKWCheckButton::New();
  this->CompositeThresholdScale = vtkKWScale::New();
  this->CompositeThresholdLabel = vtkKWLabel::New();
  this->CompositeThreshold = 10.0;

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

  this->CompositeOptionEnabled = 1;
}

//----------------------------------------------------------------------------
vtkPVCompositeRenderModuleUI::~vtkPVCompositeRenderModuleUI()
{
  // Save UI values in registry.
  vtkPVApplication* pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegistryValue(2, "RunTime", "RenderInterruptsEnabled", "%d",
                             this->RenderInterruptsEnabled);
    pvapp->SetRegistryValue(2, "RunTime", "UseFloatInComposite", "%d",
                             this->CompositeWithFloatFlag);
    pvapp->SetRegistryValue(2, "RunTime", "UseRGBAInComposite", "%d",
                             this->CompositeWithRGBAFlag);
    pvapp->SetRegistryValue(2, "RunTime", "UseCompressionInComposite", "%d",
                             this->CompositeCompressionFlag);
    pvapp->SetRegistryValue(2, "RunTime", "CompositeThreshold", "%f",
                             this->CompositeThreshold);
    pvapp->SetRegistryValue(2, "RunTime", "ReductionFactor", "%d",
                             this->ReductionFactor);
    pvapp->SetRegistryValue(2, "RunTime", "SquirtLevel", "%d",
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

  this->CompositeLabel->Delete();
  this->CompositeLabel = NULL;
  this->CompositeCheck->Delete();
  this->CompositeCheck = NULL;
  this->CompositeThresholdScale->Delete();
  this->CompositeThresholdScale = NULL;
  this->CompositeThresholdLabel->Delete();
  this->CompositeThresholdLabel = NULL;

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

}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }
  this->Superclass::Create(app, NULL);

  vtkPVApplication *pvapp = vtkPVApplication::SafeDownCast(app);
  // Skip over LOD res and threshold.
  int row = 6;
  
  // LOD parameters: collection threshold
  // Conditional interface should really be part of a module. !!!!
  if ((pvapp->GetOptions()->GetClientMode() || pvapp->GetProcessModule()->GetNumberOfPartitions() > 1) &&
      !pvapp->GetOptions()->GetUseRenderingGroup())
    {
    // Determines when geometry is collected to process 0 for rendering.
    this->CompositeLabel->SetParent(this->LODScalesFrame);
    this->CompositeLabel->Create(app, "-anchor w");
    this->CompositeLabel->SetText("Composite:");

    this->CompositeCheck->SetParent(this->LODScalesFrame);
    this->CompositeCheck->Create(app, "");
    this->CompositeCheck->SetState(1);
    this->CompositeCheck->SetCommand(this, "CompositeCheckCallback");

    this->CompositeThresholdScale->SetParent(this->LODScalesFrame);
    this->CompositeThresholdScale->Create(app,
                                        "-orient horizontal");
    this->CompositeThresholdScale->SetRange(0.0, 100.0);
    this->CompositeThresholdScale->SetResolution(0.1);
    this->CompositeThresholdScale->SetValue(this->CompositeThreshold);
    this->CompositeThresholdScale->SetEndCommand(this, 
                                                 "CompositeThresholdScaleCallback");
    this->CompositeThresholdScale->SetCommand(this, 
                                              "CompositeThresholdLabelCallback");
    this->CompositeThresholdScale->SetBalloonHelpString(
      "This slider determines when distributed rendering is used."
      "When compositing is off geometry is collected to process 0 for "
      "local rendering. Threshold critera is based on size of model in mega "
      "bytes.  "
      "Left: Always use compositing. Right: Move even large models "
      "to process 0.");    

    this->CompositeThresholdLabel->SetParent(this->LODScalesFrame);
    this->CompositeThresholdLabel->Create(app, "-anchor w");
    if (pvapp &&
        pvapp->GetRegistryValue(2, "RunTime", "CompositeThreshold", 0))
      {
      this->CompositeThreshold = 
        pvapp->GetFloatRegistryValue(2, "RunTime", "CompositeThreshold");
      }

    // Force the set.
    float tmp = this->CompositeThreshold;
    this->CompositeThreshold = -1.0;
    this->SetCompositeThreshold(tmp);

    pvapp->Script("grid %s -row %d -column 2 -sticky nws", 
                  this->CompositeThresholdLabel->GetWidgetName(), row++);
    pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                  this->CompositeLabel->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 1 -sticky nes", 
                  this->CompositeCheck->GetWidgetName(), row);
    pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                  this->CompositeThresholdScale->GetWidgetName(), row++);

    pvapp->Script("grid columnconfigure %s 2 -weight 1",
                  this->CompositeThresholdScale->GetParent()->GetWidgetName());
    

    // Determines which reduction/subsampling factor to use.
    this->ReductionLabel->SetParent(this->LODScalesFrame);
    this->ReductionLabel->Create(app, "-anchor w");
    this->ReductionLabel->SetText("Subsample Rate:");

    this->ReductionCheck->SetParent(this->LODScalesFrame);
    this->ReductionCheck->Create(app, "");
    this->ReductionCheck->SetState(1);
    this->ReductionCheck->SetCommand(this, "ReductionCheckCallback");

    this->ReductionFactorScale->SetParent(this->LODScalesFrame);
    this->ReductionFactorScale->Create(app,
                                        "-orient horizontal");
    this->ReductionFactorScale->SetRange(2, 20);
    this->ReductionFactorScale->SetResolution(1);
    this->ReductionFactorScale->SetValue(this->ReductionFactor);
    this->ReductionFactorScale->SetCommand(this, "ReductionFactorScaleCallback");
    this->ReductionFactorScale->SetBalloonHelpString(
             "Subsampling is a compositing LOD technique. "
             "Subsampling will use larger pixels during interaction.");

    this->ReductionFactorLabel->SetParent(this->LODScalesFrame);
    this->ReductionFactorLabel->SetText("2 Pixels");
    this->ReductionFactorLabel->Create(app, "-anchor w");
    if (pvapp &&
        pvapp->GetRegistryValue(2, "RunTime", "ReductionFactor", 0))
      {
      this->SetReductionFactor(
        pvapp->GetIntRegistryValue(2, "RunTime", "ReductionFactor"));
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
    this->SquirtLabel->Create(app, "-anchor w");
    this->SquirtLabel->SetText("Squirt Compression:");

    this->SquirtCheck->SetParent(this->LODScalesFrame);
    this->SquirtCheck->Create(app, "");
    this->SquirtCheck->SetState(1);
    this->SquirtCheck->SetCommand(this, "SquirtCheckCallback");

    this->SquirtLevelScale->SetParent(this->LODScalesFrame);
    this->SquirtLevelScale->Create(app,
                                        "-orient horizontal");
    this->SquirtLevelScale->SetRange(1, 6);
    this->SquirtLevelScale->SetResolution(1);
    this->SquirtLevelScale->SetValue(this->SquirtLevel);
    this->SquirtLevelScale->SetEndCommand(this, 
                                          "SquirtLevelScaleCallback");

    this->SquirtLevelLabel->SetParent(this->LODScalesFrame);
    this->SquirtLevelLabel->Create(app, "-anchor w");
    if (pvapp &&
        pvapp->GetRegistryValue(2, "RunTime", "SquirtLevel", 0))
      {
      this->SquirtLevel = 
        pvapp->GetIntRegistryValue(2, "RunTime", "SquirtLevel");
      }

    if (pvapp->GetOptions()->GetClientMode() &&
      !pvapp->GetOptions()->GetTileDimensions()[0])
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
      int sqlevel = this->SquirtLevel;
      this->SquirtLevel = -1;
      this->SetSquirtLevel(sqlevel);
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
    this->ParallelRenderParametersFrame->Create(app,0);
    this->ParallelRenderParametersFrame->SetLabelText(
      "Parallel Rendering Parameters");

    this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
                 this->ParallelRenderParametersFrame->GetWidgetName());
  
    this->CompositeWithFloatCheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeWithFloatCheck->Create(app, 
                                          "-text \"Composite with floats\"");

    this->CompositeWithRGBACheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeWithRGBACheck->Create(app, 
                                         "-text \"Composite RGBA\"");

    this->CompositeCompressionCheck->SetParent(
      this->ParallelRenderParametersFrame->GetFrame());
    this->CompositeCompressionCheck->Create(app, 
                                            "-text \"Composite compression\"");
  
    this->CompositeWithFloatCheck->SetCommand(this, 
                                              "CompositeWithFloatCallback");
    if (pvapp && 
        pvapp->GetRegistryValue(2, "RunTime", "UseFloatInComposite", 0))
      {
      this->CompositeWithFloatFlag =
        pvapp->GetIntRegistryValue(2, "RunTime", "UseFloatInComposite");
      }
    this->CompositeWithFloatCheck->SetState(this->CompositeWithFloatFlag);
    this->CompositeWithFloatCallback();
    this->CompositeWithFloatCheck->SetBalloonHelpString(
      "Toggle the use of char/float values when compositing. "
      "If rendering defects occur, try turning this on.");
  
    this->CompositeWithRGBACheck->SetCommand(this, "CompositeWithRGBACallback");
    if (pvapp && pvapp->GetRegistryValue(2, "RunTime", 
                                          "UseRGBAInComposite", 0))
      {
      this->CompositeWithRGBAFlag =
        pvapp->GetIntRegistryValue(2, "RunTime", "UseRGBAInComposite");
      }
    this->CompositeWithRGBACheck->SetState(this->CompositeWithRGBAFlag);
    this->CompositeWithRGBACallback();
    this->CompositeWithRGBACheck->SetBalloonHelpString(
      "Toggle the use of RGB/RGBA values when compositing. "
      "This is here to bypass some bugs in some graphics card drivers.");

    this->CompositeCompressionCheck->SetCommand(this, 
                                               "CompositeCompressionCallback");
    if (pvapp && 
        pvapp->GetRegistryValue(2, "RunTime", "UseCompressionInComposite", 0))
      {
      this->CompositeCompressionFlag = 
        pvapp->GetIntRegistryValue(2, "RunTime", "UseCompressionInComposite");
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
void vtkPVCompositeRenderModuleUI::Initialize()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  if (pvApp == 0)
    {
    vtkErrorMacro("No application.");
    return;
    }
  // Consider the command line option that turns compositing off.
  // This is to avoid compositing when it is not available
  // on the server.
  if (!pvApp->GetProcessModule()->GetServerInformation()->GetRemoteRendering())
    {
    this->CompositeOptionEnabled = 0;
    }
  if ( ! this->CompositeOptionEnabled)
    {
    this->CompositeCheck->SetState(0);
    this->SetCompositeThreshold(VTK_LARGE_FLOAT);
    this->CompositeCheck->EnabledOff();
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeThresholdScaleCallback()
{
  float threshold = this->CompositeThresholdScale->GetValue();

  this->SetCompositeThreshold(threshold);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeThresholdLabelCallback()
{
  float threshold = this->CompositeThresholdScale->GetValue();

  if (threshold == VTK_LARGE_FLOAT)
    {
    this->CompositeThresholdLabel->SetText("Compositing Disabled");
    }
  else
    {
    char str[256];
    sprintf(str, "Composite above %.1f MBytes", threshold);
    this->CompositeThresholdLabel->SetText(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::CompositeCheckCallback()
{
  int val = this->CompositeCheck->GetState();

  if (val == 0)
    {
    this->SetCompositeThreshold(VTK_LARGE_FLOAT);
    }
  else
    {
    float threshold = this->CompositeThresholdScale->GetValue();
    this->SetCompositeThreshold(threshold);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetCompositeThreshold(float threshold)
{
  if (this->CompositeThreshold == threshold)
    {
    return;
    }

  // Hack to get rid of a feature (tiled display compositing).
  if ( ! this->CompositeOptionEnabled)
    {
    threshold = VTK_LARGE_FLOAT;
    }

  this->CompositeThresholdLabelCallback();

  if (threshold == VTK_LARGE_FLOAT)
    {
    this->CompositeCheck->SetState(0);
    this->CompositeThresholdScale->EnabledOff();
    this->CompositeThresholdLabel->EnabledOff();
    }
  else
    {
    this->CompositeCheck->SetState(1);
    this->CompositeThresholdScale->EnabledOn();
    this->CompositeThresholdLabel->EnabledOn();
    this->CompositeThresholdScale->SetValue(threshold);
    }

  this->CompositeThreshold = threshold;

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("CompositeThreshold"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property CompositeThreshold.");
    return;
    }
  dvp->SetElement(0, threshold);
  this->RenderModuleProxy->UpdateVTKObjects();

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetCompositeThreshold %f}",
                      this->GetTclName(), threshold);
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
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) CompositeWithFloatCallback %d}", 
                      this->GetTclName(), val);
  this->CompositeWithFloatFlag = val;
  if ( this->CompositeWithFloatCheck->GetState() != val )
    {
    this->CompositeWithFloatCheck->SetState(val);
    }
/* TODO: 
  if (this->CompositeRenderModule->GetCompositeID())
    {
    this->CompositeRenderModule->SetUseCompositeWithFloat(val);
    // Limit of composite manager.
    if (val != 0) // float
      {
      this->CompositeWithRGBACheck->SetState(1);
      }
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    }
*/
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
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) CompositeWithRGBACallback %d}", 
                      this->GetTclName(), val);
  this->CompositeWithRGBAFlag = val;
  if ( this->CompositeWithRGBACheck->GetState() != val )
    {
    this->CompositeWithRGBACheck->SetState(val);
    }
/* TODO:
  if (this->CompositeRenderModule->GetCompositeID())
    {
    this->CompositeRenderModule->SetUseCompositeWithRGBA(val);
    // Limit of composite manager.
    if (val != 1) // RGB
      {
      this->CompositeWithFloatCheck->SetState(0);
      }
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    }
*/
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
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) CompositeCompressionCallback %d}", 
                      this->GetTclName(), val);

  this->CompositeCompressionFlag = val;
  if ( this->CompositeCompressionCheck->GetState() != val )
    {
    this->CompositeCompressionCheck->SetState(val);
    }

  // Let the render module do what it needs to.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("UseCompositeCompression"));
  if (!ivp)
    {
    vtkErrorMacro("Cannot find property UseCompositeCompression on "
      "RenderModuleProxy.");
    return;
    }
  ivp->SetElement(0, val);
  this->RenderModuleProxy->UpdateVTKObjects();

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

  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetSquirtLevel %d}", 
                      this->GetTclName(), level);
  this->SquirtLevel = level;

  if (level == 0)
    {
    this->SquirtLevelScale->EnabledOff();
    this->SquirtLevelLabel->EnabledOff();
    this->SquirtCheck->SetState(0);
    this->SquirtLevelLabel->SetText("24 Bits-disabled");
    vtkTimerLog::MarkEvent("--- Squirt disabled.");
    }
  else
    {
    this->SquirtLevelScale->EnabledOn();
    this->SquirtLevelLabel->EnabledOn();
    this->SquirtLevelScale->SetValue(level);
    this->SquirtCheck->SetState(1);
    switch(level)
      {
      case 1:
        this->SquirtLevelLabel->SetText("24 Bits");
        break;
      case 2:
        this->SquirtLevelLabel->SetText("22 Bits");
        break;
      case 3:
        this->SquirtLevelLabel->SetText("19 Bits");
        break;
      case 4:
        this->SquirtLevelLabel->SetText("16 Bits");
        break;
      case 5:
        this->SquirtLevelLabel->SetText("13 Bits");
        break;
      case 6:
        this->SquirtLevelLabel->SetText("10 Bits");
        break;
      }

    vtkTimerLog::FormatAndMarkEvent("--- Squirt level %d.", level);
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("SquirtLevel"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property SquirtLevel on RenderModuleProxy.");
    return;
    }
  ivp->SetElement(0, level);
  this->RenderModuleProxy->UpdateVTKObjects();
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

  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->AddTraceEntry("catch {$kw(%s) SetReductionFactor %d}", 
                      this->GetTclName(), factor);
  this->ReductionFactor = factor;

  if (factor == 1)
    {
    this->ReductionFactorScale->EnabledOff();
    this->ReductionFactorLabel->EnabledOff();
    this->ReductionCheck->SetState(0);
    this->ReductionFactorLabel->SetText("Subsampling Disabled"); 
    vtkTimerLog::MarkEvent("--- Reduction disabled.");
    }
  else
    {
    this->ReductionFactorScale->EnabledOn();
    this->ReductionFactorLabel->EnabledOn();
    this->ReductionFactorScale->SetValue(factor);
    this->ReductionCheck->SetState(1);
    char str[128];
    sprintf(str, "%d Pixels", factor);
    this->ReductionFactorLabel->SetText(str); 
     vtkTimerLog::FormatAndMarkEvent("--- Reduction factor %d.", factor);
   }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("ReductionFactor"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find ReductionFactor on RenderModuleProxy.");
    return;
    }
  ivp->SetElement(0, factor);
  this->RenderModuleProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SetCompositeOptionEnabled(int val)
{
  this->CompositeOptionEnabled = val;
  this->CompositeCheck->SetEnabled(val);
  this->CompositeCheckCallback();
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::SaveState(ofstream *file)
{
  this->Superclass::SaveState(file);
  
  // We use catches because the paraview loading the state file might not
  // have this module.
  *file << "catch {$kw(" << this->GetTclName()
        << ") CompositeWithFloatCallback "
        << this->CompositeWithFloatCheck->GetState() << "}" << endl;
  *file << "catch {$kw(" << this->GetTclName()
        << ") CompositeWithRGBACallback "
        << this->CompositeWithRGBACheck->GetState() << "}" << endl;
  *file << "catch {$kw(" << this->GetTclName()
        << ") CompositeCompressionCallback "
        << this->CompositeCompressionCheck->GetState() << "}" << endl;
  
  *file << "catch {$kw(" << this->GetTclName() << ") SetCompositeThreshold "
        << this->CompositeThreshold << "}" << endl;
  *file << "catch {$kw(" << this->GetTclName() << ") SetReductionFactor "
        << this->ReductionFactor << "}" << endl;
  *file << "catch {$kw(" << this->GetTclName() << ") SetSquirtLevel "
        << this->SquirtLevel << "}" << endl;
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CompositeThreshold: " << this->CompositeThreshold << endl;
  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;

  os << indent << "CompositeWithFloatFlag: " 
     << this->CompositeWithFloatFlag << endl;
  os << indent << "CompositeWithRGBAFlag: " 
     << this->CompositeWithRGBAFlag << endl;
  os << indent << "CompositeCompressionFlag: " 
     << this->CompositeCompressionFlag << endl;
}
