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
#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkPVXMLElement.h"

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
  this->SetCommand = NULL;

  this->MinHelp = 0;
  this->MaxHelp = 0;

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
  this->SetSetCommand(NULL);
  this->SetMinHelp(0);
  this->SetMaxHelp(0);
}

void vtkPVMinMax::SetMinimumLabel(const char* label)
{
  this->MinLabel->SetLabel(label);
}

void vtkPVMinMax::SetMaximumLabel(const char* label)
{
  this->MaxLabel->SetLabel(label);
}

void vtkPVMinMax::SetMinimumHelp(const char* help)
{
  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (help != this->MinHelp)
    {
    this->SetMinHelp(help);
    }
  this->MinLabel->SetBalloonHelpString(help);
  this->MinScale->SetBalloonHelpString(help);
  
}

void vtkPVMinMax::SetMaximumHelp(const char* help)
{
  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (help != this->MaxHelp)
    {
    this->SetMaxHelp(help);
    }
  this->MaxLabel->SetBalloonHelpString(help);
  this->MaxScale->SetBalloonHelpString(help);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::Create(vtkKWApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("PVScale already created");
    return;
    }

  // For getting the widget in a script.
  this->SetTraceName(this->MinLabel->GetLabel());
  
  this->SetApplication(pvApp);

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
  this->Script("pack %s -side left", this->MinLabel->GetWidgetName());

  this->MaxLabel->SetParent(this->MaxFrame);
  this->MaxLabel->Create(pvApp, "-width 18 -justify right");
  this->Script("pack %s -side left", this->MaxLabel->GetWidgetName());

  this->MinScale->SetParent(this->MinFrame);
  this->MinScale->Create(this->Application, "-showvalue 1 -digits 5");
  this->MinScale->SetCommand(this, "MinValueCallback");
  this->Script("pack %s -fill x -expand t", this->MinScale->GetWidgetName());
  
  this->MaxScale->SetParent(this->MaxFrame);
  this->MaxScale->Create(this->Application, "-showvalue 1 -digits 5");
  this->MaxScale->SetCommand(this, "MaxValueCallback");
  this->Script("pack %s -fill x -expand t", this->MaxScale->GetWidgetName());

  this->SetMinimumHelp(this->MinHelp);
  this->SetMaximumHelp(this->MaxHelp);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinValue(float val)
{
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

  if (this->ModifiedFlag)
    {  
    this->AddTraceEntry("$kw(%s) SetMinValue %f", this->GetTclName(), 
                         this->MinScale->GetValue());
    this->AddTraceEntry("$kw(%s) SetMaxValue %f", this->GetTclName(), 
                         this->MaxScale->GetValue());
    }

  pvApp->BroadcastScript("%s %s %f %f",
                         this->ObjectTclName, this->SetCommand,
                         this->GetMinValue(), this->GetMaxValue());

  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVMinMax::Reset()
{
  // Command to update the UI.
  this->Script("%s SetValue [%s %s]", this->MinScale->GetTclName(), 
               this->ObjectTclName, this->GetMinCommand); 
  this->Script("%s SetValue [%s %s]", this->MaxScale->GetTclName(), 
               this->ObjectTclName, this->GetMaxCommand); 

  this->ModifiedFlag = 0;
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

void vtkPVMinMax::SaveInTclScript(ofstream *file)
{
  char *result;
  
  *file << this->ObjectTclName << " " << this->SetCommand;
  this->Script("set tempValue [%s %s]", this->ObjectTclName, this->GetMinCommand);
  result = this->Application->GetMainInterp()->result;
  *file << " " << result;
  this->Script("set tempValue [%s %s]", this->ObjectTclName, this->GetMaxCommand);
  result = this->Application->GetMainInterp()->result;
  *file << " " << result << "\n";
}

vtkPVMinMax* vtkPVMinMax::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVMinMax::SafeDownCast(clone);
}

void vtkPVMinMax::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVMinMax* pvmm = vtkPVMinMax::SafeDownCast(clone);
  if (pvmm)
    {
    pvmm->SetMinimumLabel(this->MinLabel->GetLabel());
    pvmm->SetMaximumLabel(this->MaxLabel->GetLabel());
    pvmm->SetMinimumHelp(this->MinHelp);
    pvmm->SetMaximumHelp(this->MaxHelp);
    pvmm->SetResolution(this->MinScale->GetResolution());
    float min, max;
    this->MinScale->GetRange(min, max);
    pvmm->SetRange(min, max);
    pvmm->SetSetCommand(this->SetCommand);
    pvmm->SetGetMinCommand(this->GetMinCommand);
    pvmm->SetGetMaxCommand(this->GetMaxCommand);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVMinMax.");
    }
}

//----------------------------------------------------------------------------
int vtkPVMinMax::ReadXMLAttributes(vtkPVXMLElement* element,
                                   vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the MinimumLabel.
  const char* min_label = element->GetAttribute("min_label");
  if(!min_label)
    {
    vtkErrorMacro("No min_label attribute.");
    return 0;
    }
  this->SetMinimumLabel(min_label);
  
  // Setup the MaximumLabel.
  const char* max_label = element->GetAttribute("max_label");
  if(!max_label)
    {
    vtkErrorMacro("No max_label attribute.");
    return 0;
    }
  this->SetMaximumLabel(max_label);
  
  // Setup the MinimumHelp.
  const char* min_help = element->GetAttribute("min_help");
  if(!min_help)
    {
    vtkErrorMacro("No min_help attribute.");
    return 0;
    }
  this->SetMinimumHelp(min_help);
  
  // Setup the MaximumHelp.
  const char* max_help = element->GetAttribute("max_help");
  if(!max_help)
    {
    vtkErrorMacro("No max_help attribute.");
    return 0;
    }
  this->SetMaximumHelp(max_help);
  
  // Setup the GetMinCommand.
  const char* get_min_command = element->GetAttribute("get_min_command");
  if(!get_min_command)
    {
    vtkErrorMacro("No get_min_command attribute.");
    return 0;
    }
  this->SetGetMinCommand(get_min_command);
  
  // Setup the GetMaxCommand.
  const char* get_max_command = element->GetAttribute("get_max_command");
  if(!get_max_command)
    {
    vtkErrorMacro("No get_max_command attribute.");
    return 0;
    }
  this->SetGetMaxCommand(get_max_command);
  
  // Setup the SetCommand.
  const char* set_command = element->GetAttribute("set_command");
  if(!set_command)
    {
    vtkErrorMacro("No set_command attribute.");
    return 0;
    }
  this->SetSetCommand(set_command);
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVMinMax::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "GetMaxCommand: " 
     << (this->GetGetMaxCommand()?this->GetGetMaxCommand():"none") << endl;
  os << "GetMinCommand: " 
     << (this->GetGetMinCommand()?this->GetGetMinCommand():"none") << endl;
  os << "SetCommand: " << (this->SetCommand?this->SetCommand:"none") << endl;
}
