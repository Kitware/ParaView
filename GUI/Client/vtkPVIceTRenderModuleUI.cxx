/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTRenderModuleUI);
vtkCxxRevisionMacro(vtkPVIceTRenderModuleUI, "1.6");

int vtkPVIceTRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVIceTRenderModuleUI::vtkPVIceTRenderModuleUI()
{
  this->CommandFunction = vtkPVIceTRenderModuleUICommand;
  this->CompositeOptionEnabled = 1;

  this->CollectLabel = vtkKWLabel::New();
  this->CollectCheck = vtkKWCheckButton::New();
  this->CollectThresholdScale = vtkKWScale::New();
  this->CollectThresholdLabel = vtkKWLabel::New();
  this->CollectThreshold = 100.0;
}


//----------------------------------------------------------------------------
vtkPVIceTRenderModuleUI::~vtkPVIceTRenderModuleUI()
{
  // Save UI values in registry.
  vtkPVApplication *pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegistryValue(2, "RunTime", "CollectThreshold", "%d",
                            this->CollectThreshold);
    }

  this->CollectLabel->Delete();
  this->CollectLabel = NULL;
  this->CollectCheck->Delete();
  this->CollectCheck = NULL;
  this->CollectThresholdScale->Delete();
  this->CollectThresholdScale = NULL;
  this->CollectThresholdLabel->Delete();
  this->CollectThresholdLabel = NULL;
}

//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }

  this->Superclass::Create(app, NULL);

  vtkPVApplication *pvapp = vtkPVApplication::SafeDownCast(app);
  // Skip over LOD res and threshold, composite threshold, and subsample rate.
  int row = 10;

  this->CollectLabel->SetParent(this->LODScalesFrame);
  this->CollectLabel->Create(app, "-anchor w");
  this->CollectLabel->SetText("Client Collect:");

  this->CollectCheck->SetParent(this->LODScalesFrame);
  this->CollectCheck->Create(app, "");
  this->CollectCheck->SetState(1);
  this->CollectCheck->SetCommand(this, "CollectCheckCallback");

  this->CollectThresholdScale->SetParent(this->LODScalesFrame);
  this->CollectThresholdScale->Create(app, "-orient horizontal");
  this->CollectThresholdScale->SetRange(0.0, 1000.0);
  this->CollectThresholdScale->SetResolution(10.0);
  this->CollectThresholdScale->SetValue(this->CollectThreshold);
  this->CollectThresholdScale->SetEndCommand(this,
                                             "CollectThresholdScaleCallback");
  this->CollectThresholdScale->SetCommand(this,
                                          "CollectThresholdLabelCallback");
  this->CollectThresholdScale->SetBalloonHelpString(
    "This slider determines when any geometry is collected on the client."
    "If geometry is not collected on the client, the outline is drawn on"
    "the client (but the tile display still shows the geometry)."
    "Left: Never collect any geometry on the client."
    "Right: Collect larger geometry on client.");

  this->CollectThresholdLabel->SetParent(this->LODScalesFrame);
  this->CollectThresholdLabel->Create(app, "-anchor w");
  if (pvapp->GetRegistryValue(2, "RunTime", "CollectThreshold", 0))
    {
    this->CollectThreshold
      = pvapp->GetFloatRegistryValue(2, "RunTime", "CollectThreshold");
    }

  // Force the set.
  float tmp = this->CollectThreshold;
  this->CollectThreshold = -1.0;
  this->SetCollectThreshold(tmp);

  this->Script("grid %s -row %d -column 2 -sticky nws", 
               this->CollectThresholdLabel->GetWidgetName(), row++);
  this->Script("grid %s -row %d -column 0 -sticky nws", 
               this->CollectLabel->GetWidgetName(), row);
  this->Script("grid %s -row %d -column 1 -sticky nes", 
               this->CollectCheck->GetWidgetName(), row);
  this->Script("grid %s -row %d -column 2 -sticky news", 
               this->CollectThresholdScale->GetWidgetName(), row++);

  this->Script("grid columnconfigure %s 2 -weight 1",
               this->CollectThresholdScale->GetParent()->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::CollectCheckCallback()
{
  int val = this->CollectCheck->GetState();

  if (val)
    {
    float threshold = this->CollectThresholdScale->GetValue();
    this->SetCollectThreshold(threshold);
    }
  else
    {
    this->SetCollectThreshold(VTK_LARGE_FLOAT);
    }
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::CollectThresholdScaleCallback()
{
  float threshold = this->CollectThresholdScale->GetValue();
  this->SetCollectThreshold(threshold);
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::CollectThresholdLabelCallback()
{
  float threshold = this->CollectThresholdScale->GetValue();

  if (threshold == VTK_LARGE_FLOAT)
    {
    this->CollectThresholdLabel->SetText("Always Collect");
    }
  else
    {
    char str[256];
    sprintf(str, "Collect below %.0f MBytes", threshold);
    this->CollectThresholdLabel->SetText(str);
    }
}

//-----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::SetCollectThreshold(float threshold)
{
  if (this->CollectThreshold == threshold) return;

  this->CollectThresholdLabelCallback();

  if (threshold == VTK_LARGE_FLOAT)
    {
    this->CollectCheck->SetState(0);
    this->CollectThresholdScale->EnabledOff();
    this->CollectThresholdLabel->EnabledOff();
    }
  else
    {
    this->CollectCheck->SetState(1);
    this->CollectThresholdScale->EnabledOn();
    this->CollectThresholdLabel->EnabledOn();
    this->CollectThresholdScale->SetValue(threshold);
    }

  this->CollectThreshold = threshold;

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
              this->RenderModuleProxy->GetProperty("CollectGeometryThreshold"));
  dvp->SetElement(0, threshold);
  this->RenderModuleProxy->UpdateVTKObjects();

  vtkTimerLog::FormatAndMarkEvent("--- Change LOD Threshold %f.", threshold);
  
  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->GetTraceHelper()->AddEntry("catch {$kw(%s) SetCollectThreshold %f}",
                                   this->GetTclName(), threshold);
}



//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "CollectThreshold: " << this->CollectThreshold << endl;
}

