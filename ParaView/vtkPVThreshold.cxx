/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVThreshold.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVThreshold.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"

int vtkPVThresholdCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVThreshold::vtkPVThreshold()
{
  this->CommandFunction = vtkPVThresholdCommand;
  
  this->Threshold = NULL;
  
  this->AttributeModeFrame = vtkKWWidget::New();
  this->AttributeModeLabel = vtkKWLabel::New();
  this->AttributeModeMenu = vtkKWOptionMenu::New();
  this->UpperValueScale = vtkKWScale::New();
  this->LowerValueScale = vtkKWScale::New();
  this->AllScalarsCheck = vtkKWCheckButton::New();
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
  return new vtkPVThreshold();
}

//----------------------------------------------------------------------------
void vtkPVThreshold::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  float range[2];
  
  this->vtkPVSource::CreateProperties();
  
  this->Threshold = (vtkThreshold*)this->GetVTKSource();
  if (!this->Threshold)
    {
    return;
    }
  
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
  
  if (this->Threshold->GetInput()->GetPointData()->GetScalars())
    {
    this->Threshold->GetInput()->GetPointData()->GetScalars()->GetRange(range);
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
  this->CancelCommands->AddString("%s SetValue [%s %s]",
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
  this->CancelCommands->AddString("%s SetValue [%s %s]",
                                  this->LowerValueScale->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetLowerThreshold");

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue]\"",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "ThresholdBetween",
                                  this->LowerValueScale->GetTclName(),
                                  this->UpperValueScale->GetTclName());

  this->AllScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->AllScalarsCheck->Create(pvApp, "-text AllScalars");
  this->AllScalarsCheck->SetState(1);
  this->AllScalarsCheck->SetBalloonHelpString("If AllScalars is checked, then a cell is only included if all its points are within the threshold. This is only relevant for point data.");
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s %s]",
                                  this->AllScalarsCheck->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetAllScalars");

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "SetAllScalars",
                                  this->AllScalarsCheck->GetTclName());

  this->Script("pack %s %s %s",
               this->UpperValueScale->GetWidgetName(),
               this->LowerValueScale->GetWidgetName(),
               this->AllScalarsCheck->GetWidgetName());
}

void vtkPVThreshold::UpperValueCallback()
{
  float lowerValue = this->LowerValueScale->GetValue();
  if (this->UpperValueScale->GetValue() < lowerValue)
    {
    this->UpperValueScale->SetValue(lowerValue);
    }
}

void vtkPVThreshold::LowerValueCallback()
{
  float upperValue = this->UpperValueScale->GetValue();
  if (this->LowerValueScale->GetValue() > upperValue)
    {
    this->LowerValueScale->SetValue(upperValue);
    }
}

void vtkPVThreshold::ChangeAttributeMode(const char* newMode)
{
  float range[2];
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (strcmp(newMode, "point") == 0)
    {
    if (this->Threshold->GetInput()->GetPointData()->GetScalars())
      {
      this->Threshold->GetInput()->GetPointData()->GetScalars()->GetRange(range);
      }
    else
      {
      range[0] = 0;
      range[1] = 1;
      }
    this->Threshold->SetAttributeModeToUsePointData();
    pvApp->BroadcastScript("%s SetAttributeModeToUsePointData",
                           this->GetVTKSourceTclName());
    }
  else if (strcmp(newMode, "cell") == 0)
    {
    if (this->Threshold->GetInput()->GetCellData()->GetScalars())
      {
      this->Threshold->GetInput()->GetCellData()->GetScalars()->GetRange(range);
      }
    else
      {
      range[0] = 0;
      range[1] = 1;
      }
    this->Threshold->SetAttributeModeToUseCellData();
    pvApp->BroadcastScript("%s SetAttributeModeToUseCellData",
                           this->GetVTKSourceTclName());
    }

  this->UpperValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->UpperValueScale->SetRange(range[0], range[1]);
  this->UpperValueScale->SetValue(range[1]);

  this->LowerValueScale->SetResolution((range[1] - range[0]) / 100.0);
  this->LowerValueScale->SetRange(range[0], range[1]);
  this->LowerValueScale->SetValue(range[0]);
}

void vtkPVThreshold::Save(ofstream *file)
{
  char sourceTclName[256];
  char* tempName;
  vtkThreshold *source = (vtkThreshold*)this->GetVTKSource();
  char *charFound;
  int pos;
  
  if (this->DefaultScalarsName)
    {
    *file << "vtkSimpleFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (strncmp(this->GetNthPVInput(0)->GetVTKDataTclName(),
                "EnSight", 7) == 0)
      {
      char *charFound;
      int pos;
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      sprintf(sourceTclName, "EnSightReader");
      tempName = strtok(dataName, "O");
      strcat(sourceTclName, tempName+7);
      *file << sourceTclName << " GetOutput ";
      charFound = strrchr(dataName, 't');
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      }
    else if (strncmp(this->GetNthPVInput(0)->GetVTKDataTclName(),
                     "DataSet", 7) == 0)
      {
      sprintf(sourceTclName, "DataSetReader");
      tempName = strtok(this->GetNthPVInput(0)->GetVTKDataTclName(), "O");
      strcat(sourceTclName, tempName+7);
      *file << sourceTclName << " GetOutput]\n\t";
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    *file << this->ChangeScalarsFilterTclName << " SetFieldName "
          << this->DefaultScalarsName << "\n\n";
    }

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";
  
  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->DefaultScalarsName)
    {
    if (strncmp(this->GetNthPVInput(0)->GetVTKDataTclName(),
                "EnSight", 7) == 0)
      {
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      sprintf(sourceTclName, "EnSightReader");
      tempName = strtok(dataName, "O");
      strcat(sourceTclName, tempName+7);
      *file << sourceTclName << " GetOutput ";
      charFound = strrchr(dataName, 't');
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      }
    else if (strncmp(this->GetNthPVInput(0)->GetVTKDataTclName(),
                     "DataSet", 7) == 0)
      {
      sprintf(sourceTclName, "DataSetReader");
      tempName = strtok(this->GetNthPVInput(0)->GetVTKDataTclName(), "O");
      strcat(sourceTclName, tempName+7);
      *file << sourceTclName << " GetOutput]\n\t";
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
  
  this->GetPVOutput(0)->Save(file, this->VTKSourceTclName);
}
