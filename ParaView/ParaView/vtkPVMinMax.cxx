/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMinMax.cxx
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
#include "vtkPVMinMax.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVMinMax* vtkPVMinMax::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVMinMax");
  if (ret)
    {
    return (vtkPVMinMax*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVMinMax;
}

//----------------------------------------------------------------------------
vtkPVMinMax::vtkPVMinMax()
{
  this->MinFrame = vtkKWWidget::New();
  this->MinFrame->SetParent(this);
  this->MaxFrame = vtkKWWidget::New();
  this->MaxFrame->SetParent(this);
  this->MinLabel = vtkKWLabel::New();
  this->MaxLabel = vtkKWLabel::New();
  this->MinScale = vtkKWScale::New();
  this->MaxScale = vtkKWScale::New();

  this->GetMinCommand = NULL;
  this->GetMaxCommand = NULL;
  this->PVSource = NULL;
}

//----------------------------------------------------------------------------
vtkPVMinMax::~vtkPVMinMax()
{
  this->MinScale->Delete();
  this->MinScale = NULL;
  this->MaxScale->Delete();
  this->MaxScale = NULL;
  this->MinLabel->Delete();
  this->MinLabel = NULL;
  this->MaxLabel->Delete();
  this->MaxLabel = NULL;
  this->MinFrame->Delete();
  this->MinFrame = NULL;
  this->MaxFrame->Delete();
  this->MaxFrame = NULL;
  this->SetGetMinCommand(NULL);
  this->SetGetMaxCommand(NULL);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::Create(vtkKWApplication *pvApp, char *minLabel,
                         char* maxLabel, float min, float max,
                         float resolution, char *setCmd, char *getMinCmd,
                         char *getMaxCmd, char *minHelp, char *maxHelp)
{
  if (this->Application)
    {
    vtkErrorMacro("PVScale already created");
    return;
    }
  if ( ! this->PVSource)
    {
    vtkErrorMacro("PVSource must be set before calling create");
    return;
    }

  // For getting the widget in a script.
  this->SetName(minLabel);
  
  this->SetApplication(pvApp);

  this->SetSetCommand(setCmd);
  this->SetGetMinCommand(getMinCmd);
  this->SetGetMaxCommand(getMaxCmd);
  
  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  this->MinFrame->Create(pvApp, "frame", "");
  this->MaxFrame->Create(pvApp, "frame", "");
  this->Script("pack %s %s -fill x -expand t",
               this->MinFrame->GetWidgetName(),
               this->MaxFrame->GetWidgetName());
  
  // Now a label
  this->MinLabel->SetParent(this->MinFrame);
  this->MinLabel->Create(pvApp, "-width 18 -justify right");
  this->MinLabel->SetLabel(minLabel);
  if (minHelp)
    {
    this->MinLabel->SetBalloonHelpString(minHelp);
    }
  this->Script("pack %s -side left", this->MinLabel->GetWidgetName());

  this->MaxLabel->SetParent(this->MaxFrame);
  this->MaxLabel->Create(pvApp, "-width 18 -justify right");
  this->MaxLabel->SetLabel(maxLabel);
  if (maxHelp)
    {
    this->MaxLabel->SetBalloonHelpString(maxHelp);
    }
  this->Script("pack %s -side left", this->MaxLabel->GetWidgetName());

  this->MinScale->SetParent(this->MinFrame);
  this->MinScale->Create(this->Application, "-showvalue 1 -digits 5");
  this->MinScale->SetCommand(this, "MinValueCallback");
  this->MinScale->SetRange(min, max);
  this->MinScale->SetResolution(resolution);
  this->SetMinValue(min);
  if (minHelp)
    {
    this->MinScale->SetBalloonHelpString(minHelp);
    }
  this->Script("pack %s -fill x -expand t", this->MinScale->GetWidgetName());
  
  this->MaxScale->SetParent(this->MaxFrame);
  this->MaxScale->Create(this->Application, "-showvalue 1 -digits 5");
  this->MaxScale->SetCommand(this, "MaxValueCallback");
  this->MaxScale->SetRange(min, max);
  this->MaxScale->SetResolution(resolution);
  this->SetMaxValue(max);
  if (maxHelp)
    {
    this->MaxScale->SetBalloonHelpString(maxHelp);
    }
  this->Script("pack %s -fill x -expand t", this->MaxScale->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
                                 this->MinScale->GetTclName(), 
                                 this->PVSource->GetVTKSourceTclName(), 
                                 getMinCmd); 
  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
                                 this->MaxScale->GetTclName(), 
                                 this->PVSource->GetVTKSourceTclName(), 
                                 getMaxCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s %s [%s GetMinValue] [%s GetMaxValue]",
                                  this->PVSource->GetVTKSourceTclName(),
                                  setCmd, this->GetTclName(),
                                  this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinValue(float val)
{
  float oldVal;
  
  oldVal = this->MinScale->GetValue();
  if (val == oldVal)
    {
    return;
    }

  if (val > this->MaxScale->GetValue())
    {
    this->MinScale->SetValue(this->MaxScale->GetValue());
    }
  else
    {
    this->MinScale->SetValue(val); 
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaxValue(float val)
{
  float oldVal;
  
  oldVal = this->MaxScale->GetValue();
  if (val == oldVal)
    {
    return;
    }
  if (val < this->MinScale->GetValue())
    {
    this->MaxScale->SetValue(this->MinScale->GetValue());
    }
  else
    {
    this->MaxScale->SetValue(val); 
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag && this->PVSource)
    {  
    if ( ! this->TraceInitialized)
      {
      pvApp->AddTraceEntry("set pv(%s) [$pv(%s) GetPVWidget {%s}]",
                           this->GetTclName(), this->PVSource->GetTclName(),
                           this->Name);
      this->TraceInitialized = 1;
      }

    pvApp->AddTraceEntry("$pv(%s) SetMinValue %f", this->GetTclName(), 
                         this->MinScale->GetValue());
    pvApp->AddTraceEntry("$pv(%s) SetMaxValue %f", this->GetTclName(), 
                         this->MaxScale->GetValue());
    }

  this->vtkPVWidget::Accept();
}

void vtkPVMinMax::SetResolution(float res)
{
  this->MinScale->SetResolution(res);
  this->MaxScale->SetResolution(res);
}

void vtkPVMinMax::SetRange(float min, float max)
{
  this->MinScale->SetRange(min, max);
  this->MaxScale->SetRange(min, max);
}

void vtkPVMinMax::MinValueCallback()
{
  if (this->MinScale->GetValue() > this->MaxScale->GetValue())
    {
    this->MinScale->SetValue(this->MaxScale->GetValue());
    }
  
  this->ModifiedCallback();
}

void vtkPVMinMax::MaxValueCallback()
{
  if (this->MaxScale->GetValue() < this->MinScale->GetValue())
    {
    this->MaxScale->SetValue(this->MinScale->GetValue());
    }
  
  this->ModifiedCallback();
}

void vtkPVMinMax::SaveInTclScript(ofstream *file, const char *sourceName)
{
  char *result;
  
  *file << sourceName << " " << this->SetCommand;
  this->Script("set tempValue [%s %s]", sourceName, this->GetMinCommand);
  result = this->Application->GetMainInterp()->result;
  *file << " " << result;
  this->Script("set tempValue [%s %s]", sourceName, this->GetMaxCommand);
  result = this->Application->GetMainInterp()->result;
  *file << " " << result << "\n";
}
