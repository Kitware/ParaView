/*=========================================================================

  Program:   
  Module:    vtkPVThreshold.cxx
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
#include "vtkPVThreshold.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkObjectFactory.h"
#include "vtkPVSourceInterface.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkPVData.h"
#include "vtkPVArraySelection.h"

int vtkPVThresholdCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVThreshold::vtkPVThreshold()
{
  this->CommandFunction = vtkPVThresholdCommand;
  
  this->AttributeModeFrame = vtkKWWidget::New();
  this->AttributeModeLabel = vtkKWLabel::New();
  this->AttributeModeMenu = vtkKWOptionMenu::New();
  this->UpperValueScale = vtkKWScale::New();
  this->LowerValueScale = vtkKWScale::New();
  this->AllScalarsCheck = vtkPVLabeledToggle::New();
}

//----------------------------------------------------------------------------
vtkPVThreshold::~vtkPVThreshold()
{
  this->AttributeModeLabel->Delete();
  this->AttributeModeLabel = NULL;
  this->AttributeModeMenu->Delete();
  this->AttributeModeMenu = NULL;
  this->AttributeModeFrame->Delete();
  this->AttributeModeFrame = NULL;
  this->UpperValueScale->Delete();
  this->UpperValueScale = NULL;
  this->LowerValueScale->Delete();
  this->LowerValueScale = NULL;
  this->AllScalarsCheck->Delete();
  this->AllScalarsCheck = NULL;
}

//----------------------------------------------------------------------------
vtkPVThreshold* vtkPVThreshold::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVThreshold");
  if(ret)
    {
    return (vtkPVThreshold*)ret;
    }
  return new vtkPVThreshold();
}

//----------------------------------------------------------------------------
void vtkPVThreshold::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  float range[2];
  
  this->vtkPVSource::CreateProperties();
  
  this->AttributeModeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->AttributeModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->AttributeModeFrame->GetWidgetName());
  
  this->AttributeModeLabel->SetParent(this->AttributeModeFrame);
  this->AttributeModeLabel->Create(pvApp, "");
  this->AttributeModeLabel->SetLabel("Attribute Mode:");
  this->AttributeModeLabel->SetBalloonHelpString("Select whether to operate on point or cell data");
  this->AttributeModeMenu->SetParent(this->AttributeModeFrame);
  this->AttributeModeMenu->Create(pvApp, "");
  this->AttributeModeMenu->AddEntryWithCommand("Point Data", this,
                                               "ChangeAttributeMode point");
  this->AttributeModeMenu->AddEntryWithCommand("Cell Data", this,
                                               "ChangeAttributeMode cell");
  this->AttributeModeMenu->SetCurrentEntry("Point Data");
  this->AttributeModeMenu->SetBalloonHelpString("Select whether to operate on point or cell data");
  this->Script("pack %s %s -side left",
               this->AttributeModeLabel->GetWidgetName(),
               this->AttributeModeMenu->GetWidgetName());
  
  if (this->GetNthPVInput(0)->GetVTKData()->GetPointData()->GetScalars())
    {
    this->GetNthPVInput(0)->GetVTKData()->GetPointData()->GetScalars()->GetRange(range);
    }
  else
    {
    range[0] = 0;
    range[1] = 1;
    }
  
  this->UpperValueScale->SetParent(this->GetParameterFrame()->GetFrame());
  this->UpperValueScale->Create(pvApp, "-showvalue 1");
  this->UpperValueScale->DisplayLabel("Upper Threshold");
  this->UpperValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->UpperValueScale->SetRange(range[0], range[1]);
  this->UpperValueScale->SetValue(range[1]);
  this->UpperValueScale->SetCommand(this, "UpperValueCallback");
  this->UpperValueScale->SetBalloonHelpString("Choose the upper value of the threshold");
  
  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
                                 this->UpperValueScale->GetTclName(),
                                 this->GetVTKSourceTclName(),
                                 "GetUpperThreshold");

  this->LowerValueScale->SetParent(this->GetParameterFrame()->GetFrame());
  this->LowerValueScale->Create(pvApp, "-showvalue 1");
  this->LowerValueScale->DisplayLabel("Lower Threshold");
  this->LowerValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->LowerValueScale->SetRange(range[0], range[1]);
  this->LowerValueScale->SetValue(range[0]);
  this->LowerValueScale->SetCommand(this, "LowerValueCallback");
  this->LowerValueScale->SetBalloonHelpString("Choose the lower value of the threshold");
  
  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
                                 this->LowerValueScale->GetTclName(),
                                 this->GetVTKSourceTclName(),
                                 "GetLowerThreshold");

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [list [%s GetValue] [%s GetValue]]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "ThresholdBetween",
                                  this->LowerValueScale->GetTclName(),
                                  this->UpperValueScale->GetTclName());

  this->AllScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->AllScalarsCheck->SetPVSource(this);
  this->AllScalarsCheck->Create(pvApp, "AllScalars", "SetAllScalars",
                                "GetAllScalars", "If AllScalars is checked, then a cell is only included if all its points are within the threshold. This is only relevant for point data.",
                                this->GetVTKSourceTclName());
  this->Widgets->AddItem(this->AllScalarsCheck);
  this->AllScalarsCheck->SetState(1);

  this->Script("pack %s %s %s",
               this->UpperValueScale->GetWidgetName(),
               this->LowerValueScale->GetWidgetName(),
               this->AllScalarsCheck->GetWidgetName());
}

void vtkPVThreshold::UpperValueCallback()
{
  float lowerValue = this->LowerValueScale->GetValue();
  
  this->ChangeAcceptButtonColor();
  
  if (this->UpperValueScale->GetValue() < lowerValue)
    {
    this->UpperValueScale->SetValue(lowerValue);
    }
}

void vtkPVThreshold::LowerValueCallback()
{
  float upperValue = this->UpperValueScale->GetValue();

  this->ChangeAcceptButtonColor();
  
  if (this->LowerValueScale->GetValue() > upperValue)
    {
    this->LowerValueScale->SetValue(upperValue);
    }
}

void vtkPVThreshold::ChangeAttributeMode(const char* newMode)
{
  float range[2];
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkDataSet *thresholdInput = this->GetNthPVInput(0)->GetVTKData();
  char *arrayName = this->ScalarOperationMenu->GetValue();
  
  this->ChangeAcceptButtonColor();
  
  if (strcmp(newMode, "point") == 0)
    {
    if (thresholdInput->GetPointData()->GetArray(arrayName))
      {
      thresholdInput->GetPointData()->GetArray(arrayName)->GetRange(range);
      }
    else
      {
      range[0] = 0;
      range[1] = 1;
      }
    ((vtkThreshold*)this->GetVTKSource())->SetAttributeModeToUsePointData();
    pvApp->BroadcastScript("%s SetAttributeModeToUsePointData",
                           this->GetVTKSourceTclName());
    this->ScalarOperationMenu->UsePointDataOn();
    }
  else if (strcmp(newMode, "cell") == 0)
    {
    if (thresholdInput->GetCellData()->GetArray(arrayName))
      {
      thresholdInput->GetCellData()->GetArray(arrayName)->GetRange(range);
      }
    else
      {
      range[0] = 0;
      range[1] = 1;
      }
    ((vtkThreshold*)this->GetVTKSource())->SetAttributeModeToUseCellData();
    pvApp->BroadcastScript("%s SetAttributeModeToUseCellData",
                           this->GetVTKSourceTclName());
    this->ScalarOperationMenu->UsePointDataOff();
    }

  this->UpperValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->UpperValueScale->SetRange(range[0], range[1]);
  this->UpperValueScale->SetValue(range[1]);

  this->LowerValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->LowerValueScale->SetRange(range[0], range[1]);
  this->LowerValueScale->SetValue(range[0]);
  
  this->ScalarOperationMenu->FillMenu();
}

void vtkPVThreshold::UpdateScalars()
{
  float range[2];
  char *arrayName;
  int attributeMode;
  vtkDataArray *dataArray;
  
  this->vtkPVSource::UpdateScalars();
  arrayName = this->ScalarOperationMenu->GetValue();
  attributeMode = ((vtkThreshold*)this->GetVTKSource())->GetAttributeMode();
  
  if (attributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
    {
    dataArray = this->GetNthPVInput(0)->GetVTKData()->GetPointData()->GetArray(arrayName);
    if (dataArray)
      {
      dataArray->GetRange(range);
      }
    }
  else if (attributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA)
    {
    dataArray = this->GetNthPVInput(0)->GetVTKData()->GetCellData()->GetArray(arrayName);
    if (dataArray)
      {
      dataArray->GetRange(range);
      }
    }
  
  this->UpperValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->UpperValueScale->SetRange(range[0], range[1]);
  this->UpperValueScale->SetValue(range[1]);

  this->LowerValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->LowerValueScale->SetRange(range[0], range[1]);
  this->LowerValueScale->SetValue(range[0]);  
}

void vtkPVThreshold::SaveInTclScript(ofstream *file)
{
  char* tempName;
  char *charFound;
  int pos;
  vtkPVSourceInterface *pvsInterface =
    this->GetNthPVInput(0)->GetPVSource()->GetInterface();
  
  if (this->DefaultScalarsName)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    *file << this->ChangeScalarsFilterTclName
          << " SetInputFieldToPointDataField\n";
    *file << this->ChangeScalarsFilterTclName
          << " SetOutputAttributeDataToPointData\n";
    *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 0 "
          << this->DefaultScalarsName << " 0\n\n";
    }

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";
  
  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->DefaultScalarsName)
    {
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    }
  else
    {
    *file << this->ChangeScalarsFilterTclName << " GetOutput]\n\t";
    }
  
  *file << this->VTKSourceTclName << " SetAttributeModeToUse";
  if (strcmp(this->AttributeModeMenu->GetValue(), "Point Data") == 0)
    {
    *file << "PointData\n\t";
    }
  else
    {
    *file << "CellData\n\t";
    }
  
  *file << this->VTKSourceTclName << " ThresholdBetween "
        << this->LowerValueScale->GetValue() << " "
        << this->UpperValueScale->GetValue() << "\n\t"
        << this->VTKSourceTclName << " SetAllScalars "
        << this->AllScalarsCheck->GetState() << "\n\n";
  
  this->GetPVOutput(0)->SaveInTclScript(file, this->VTKSourceTclName);
}
