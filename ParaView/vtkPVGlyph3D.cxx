/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGlyph3D.cxx
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
#include "vtkPVGlyph3D.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkKWCompositeCollection.h"
#include "vtkPVWindow.h"
#include "vtkObjectFactory.h"
#include "vtkPVSourceInterface.h"

int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGlyph3D::vtkPVGlyph3D()
{
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  this->GlyphSourceTclName = NULL;
  this->GlyphScaleMode = NULL;
  this->GlyphVectorMode = NULL;
  
  this->GlyphSourceFrame = vtkKWWidget::New();
  this->GlyphSourceLabel = vtkKWLabel::New();
  this->GlyphSourceMenu = vtkPVInputMenu::New();
  this->ScaleModeFrame = vtkKWWidget::New();
  this->ScaleModeLabel = vtkKWLabel::New();
  this->ScaleModeMenu = vtkKWOptionMenu::New();
  this->VectorModeFrame = vtkKWWidget::New();
  this->VectorModeLabel = vtkKWLabel::New();
  this->VectorModeMenu = vtkKWOptionMenu::New();
  this->OrientCheck = vtkKWCheckButton::New();
  this->ScaleCheck = vtkKWCheckButton::New();
  this->ScaleEntry = vtkKWLabeledEntry::New();
}

//----------------------------------------------------------------------------
vtkPVGlyph3D::~vtkPVGlyph3D()
{
  if (this->GlyphSourceTclName)
    {
    delete [] this->GlyphSourceTclName;
    this->GlyphSourceTclName = NULL;
    }
  if (this->GlyphScaleMode)
    {
    delete [] this->GlyphScaleMode;
    this->GlyphScaleMode = NULL;
    }
  if (this->GlyphVectorMode)
    {
    delete [] this->GlyphVectorMode;
    this->GlyphVectorMode = NULL;
    }
  
  this->GlyphSourceLabel->Delete();
  this->GlyphSourceLabel = NULL;
  this->GlyphSourceMenu->Delete();
  this->GlyphSourceMenu = NULL;
  this->GlyphSourceFrame->Delete();
  this->GlyphSourceFrame = NULL;
  this->ScaleModeLabel->Delete();
  this->ScaleModeLabel = NULL;
  this->ScaleModeMenu->Delete();
  this->ScaleModeMenu = NULL;
  this->ScaleModeFrame->Delete();
  this->ScaleModeFrame = NULL;
  this->VectorModeLabel->Delete();
  this->VectorModeLabel = NULL;
  this->VectorModeMenu->Delete();
  this->VectorModeMenu = NULL;
  this->VectorModeFrame->Delete();
  this->VectorModeFrame = NULL;
  this->OrientCheck->Delete();
  this->OrientCheck = NULL;
  this->ScaleCheck->Delete();
  this->ScaleCheck = NULL;
  this->ScaleEntry->Delete();
  this->ScaleEntry = NULL;
}

//----------------------------------------------------------------------------
vtkPVGlyph3D* vtkPVGlyph3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVGlyph3D");
  if(ret)
    {
    return (vtkPVGlyph3D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVGlyph3D;
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
  
  this->GlyphSourceFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->GlyphSourceFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->GlyphSourceFrame->GetWidgetName());
  
  this->GlyphSourceLabel->SetParent(this->GlyphSourceFrame);
  this->GlyphSourceLabel->Create(pvApp, "");
  this->GlyphSourceLabel->SetLabel("Glyph Source:");
  this->GlyphSourceLabel->SetBalloonHelpString("Select the data set to use as a glyph");
  
  this->GlyphSourceMenu->SetParent(this->GlyphSourceFrame);
  this->GlyphSourceMenu->Create(pvApp, "");
  this->GlyphSourceMenu->SetInputType("vtkPolyData");
  this->GlyphSourceMenu->SetBalloonHelpString("Select the data set to use as a glyph");
  this->UpdateSourceMenu();
  if (this->GlyphSourceTclName)
    {
    this->GlyphSourceMenu->SetValue(this->GlyphSourceTclName);
    }
  this->AcceptCommands->AddString("%s ChangeSource",
                                  this->GetTclName());
  this->ResetCommands->AddString("%s SetValue %s",
                                 this->GlyphSourceMenu->GetTclName(),
                                 this->GetGlyphSourceTclName());
  
  this->Script("pack %s %s -side left",
               this->GlyphSourceLabel->GetWidgetName(),
                this->GlyphSourceMenu->GetWidgetName());
  
  this->ScaleModeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScaleModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->ScaleModeFrame->GetWidgetName());
  
  this->ScaleModeLabel->SetParent(this->ScaleModeFrame);
  this->ScaleModeLabel->Create(pvApp, "");
  this->ScaleModeLabel->SetLabel("Scale Mode:");
  this->ScaleModeLabel->SetBalloonHelpString("Select whether/how to scale the glyphs");
  
  this->ScaleModeMenu->SetParent(this->ScaleModeFrame);
  this->ScaleModeMenu->Create(pvApp, "");
  this->ScaleModeMenu->AddEntryWithCommand("Scalar", this,
                                           "ChangeAcceptButtonColor");
  this->ScaleModeMenu->AddEntryWithCommand("Vector", this,
                                           "ChangeAcceptButtonColor");
  this->ScaleModeMenu->AddEntryWithCommand("Vector Components", this,
                                           "ChangeAcceptButtonColor");
  this->ScaleModeMenu->AddEntryWithCommand("Data Scaling Off", this,
                                           "ChangeAcceptButtonColor");
  this->ScaleModeMenu->SetValue("Scalar");
  this->ScaleModeMenu->SetBalloonHelpString("Select whether/how to scale the glyphs");  
  this->SetGlyphScaleMode("Scalar");
  
  this->AcceptCommands->AddString("%s ChangeScaleMode",
                                  this->GetTclName());
  this->ResetCommands->AddString("%s SetValue %s",
                                 this->GetTclName(),
                                 this->GetGlyphScaleMode());
  
  this->Script("pack %s %s -side left",
               this->ScaleModeLabel->GetWidgetName(),
               this->ScaleModeMenu->GetWidgetName());
  
  this->VectorModeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->VectorModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->VectorModeFrame->GetWidgetName());
  
  this->VectorModeLabel->SetParent(this->VectorModeFrame);
  this->VectorModeLabel->Create(pvApp, "");
  this->VectorModeLabel->SetLabel("Vector Mode:");
  this->VectorModeLabel->SetBalloonHelpString("Select what to use as vectors for scaling/rotation");
  
  this->VectorModeMenu->SetParent(this->VectorModeFrame);
  this->VectorModeMenu->Create(pvApp, "");
  this->VectorModeMenu->AddEntryWithCommand("Normal", this,
                                            "ChangeAcceptButtonColor");
  this->VectorModeMenu->AddEntryWithCommand("Vector", this,
                                            "ChangeAcceptButtonColor");
  this->VectorModeMenu->AddEntryWithCommand("Vector Rotation Off", this,
                                            "ChangeAcceptButtonColor");
  this->VectorModeMenu->SetValue("Vector");
  this->VectorModeMenu->SetBalloonHelpString("Select what to use as vectors for scaling/rotation");
  this->SetGlyphVectorMode("Vector");
  
  this->AcceptCommands->AddString("%s ChangeVectorMode",
                                  this->GetTclName());
  this->ResetCommands->AddString("%s SetValue %s",
                                 this->GetTclName(),
                                 this->GetGlyphVectorMode());
  
  this->Script("pack %s %s -side left",
               this->VectorModeLabel->GetWidgetName(),
               this->VectorModeMenu->GetWidgetName());
  
  this->OrientCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->OrientCheck->Create(pvApp, "-text Orient");
  this->OrientCheck->SetState(1);
  this->OrientCheck->SetCommand(this, "ChangeAcceptButtonColor");
  this->OrientCheck->SetBalloonHelpString("Select whether to orient the glyphs");
  
  this->ResetCommands->AddString("%s SetState [%s GetOrient]",
                                 this->OrientCheck->GetTclName(),
                                 this->GetVTKSourceTclName()); 
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetOrient [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->OrientCheck->GetTclName());
  
  this->ScaleCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScaleCheck->Create(pvApp, "-text Scale");
  this->ScaleCheck->SetState(1);
  this->ScaleCheck->SetCommand(this, "ChangeAcceptButtonColor");
  this->ScaleCheck->SetBalloonHelpString("Select whether to scale the glyphs");
  
  this->ResetCommands->AddString("%s SetState [%s GetScaling]",
                                 this->ScaleCheck->GetTclName(),
                                 this->GetVTKSourceTclName()); 
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetScaling [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->ScaleCheck->GetTclName());

  this->ScaleEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScaleEntry->Create(pvApp);
  this->ScaleEntry->SetLabel("Scale Factor:");
  this->ScaleEntry->SetValue("1.0");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->ScaleEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->ScaleEntry->SetBalloonHelpString("Select the amount to scale the glyphs by");
  
  this->ResetCommands->AddString("%s SetValue [%s GetScaleFactor]",
                                 this->ScaleEntry->GetTclName(),
                                 this->GetVTKSourceTclName()); 
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetScaleFactor [%s GetValueAsFloat]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->ScaleEntry->GetTclName());

  this->Script("pack %s %s %s",
               this->OrientCheck->GetWidgetName(),
               this->ScaleCheck->GetWidgetName(),
               this->ScaleEntry->GetWidgetName());
}

void vtkPVGlyph3D::ChangeScaleMode()
{
  char *newMode = NULL;
  vtkPVApplication *pvApp = this->GetPVApplication();

  newMode = this->ScaleModeMenu->GetValue();
  
  if (strcmp(newMode, "Scalar") == 0)
    {
    pvApp->BroadcastScript("%s SetScaleModeToScaleByScalar",
                           this->GetVTKSourceTclName());
    }
  else if (strcmp(newMode, "Vector") == 0)
    {
    pvApp->BroadcastScript("%s SetScaleModeToScaleByVector",
                           this->GetVTKSourceTclName());
    }
  else if (strcmp(newMode, "Vector Components") == 0)
    {
    pvApp->BroadcastScript("%s SetScaleModeToScaleByVectorComponents",
                           this->GetVTKSourceTclName());
    }
  else if (strcmp(newMode, "Data Scaling Off") == 0)
    {
    pvApp->BroadcastScript("%s SetScaleModeToDataScalingOff",
                           this->GetVTKSourceTclName());
    }
  if (newMode)
    {
    this->SetGlyphScaleMode(newMode);
    }
}

void vtkPVGlyph3D::ChangeVectorMode()
{
  char *newMode = NULL;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  newMode = this->VectorModeMenu->GetValue();
  
  if (strcmp(newMode, "Normal") == 0)
    {
    pvApp->BroadcastScript("%s SetVectorModeToUseNormal",
                           this->GetVTKSourceTclName());
    }
  else if (strcmp(newMode, "Vector") == 0)
    {
    pvApp->BroadcastScript("%s SetVectorModeToUseVector",
                           this->GetVTKSourceTclName());
    }
  else if (strcmp(newMode, "Vector Rotation Off") == 0)
    {
    pvApp->BroadcastScript("%s SetVectorModeToVectorRotationOff",
                           this->GetVTKSourceTclName());
    }
  if (newMode)
    {
    this->SetGlyphVectorMode(newMode);
    }
}

void vtkPVGlyph3D::UpdateSourceMenu()
{
  int i;
  vtkKWCompositeCollection *sources = this->GetWindow()->GetSources();
  vtkPVSource *currentSource;
  char *tclName = NULL;
  
  this->GlyphSourceMenu->ClearEntries();
  for (i = 0; i < sources->GetNumberOfItems(); i++)
    {
    currentSource = (vtkPVSource*)sources->GetItemAsObject(i);
    if (currentSource->GetNthPVOutput(0))
      {
      if (currentSource->GetNthPVOutput(0)->GetVTKData()->IsA("vtkPolyData"))
        {
        tclName = currentSource->GetNthPVOutput(0)->GetVTKDataTclName();
        if (!this->GlyphSourceTclName)
          {
          this->SetGlyphSourceTclName(tclName);
          }
        this->GlyphSourceMenu->AddEntryWithCommand(tclName, this,
                                                   "ChangeAcceptButtonColor");
        }
      }
    }
}

void vtkPVGlyph3D::ChangeSource()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *tclName;
  
  tclName = this->GlyphSourceMenu->GetValue();
  this->SetGlyphSourceTclName(tclName);
  
  if (strcmp(this->GlyphSourceTclName, "") != 0)
    {
    pvApp->BroadcastScript("%s SetSource %s",
                           this->GetVTKSourceTclName(), tclName);
    }
}

void vtkPVGlyph3D::SaveInTclScript(ofstream *file)
{
  char sourceTclName[256];
  char* tempName;
  vtkGlyph3D *source = (vtkGlyph3D*)this->GetVTKSource();
  char *charFound;
  int pos;
  
  if (this->ChangeScalarsFilterTclName)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      }
    else if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
                    GetSourceClassName(), "vtkDataSetReader") == 0)
      {
      tempName = strtok(this->GetNthPVInput(0)->GetVTKDataTclName(), "O");
      *file << tempName << " GetOutput]\n\t";
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
    if (this->DefaultScalarsName)
      {
      *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 0 "
            << this->DefaultScalarsName << " 0\n";
      }
    else if (this->DefaultVectorsName)
      {
      *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 0 "
            << this->DefaultScalarsName << " 0\n";
      *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 1 "
            << this->DefaultScalarsName << " 1\n";
      *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 2 "
            << this->DefaultScalarsName << " 2\n";
      }
    *file << "\n";
    }

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";
  
  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->ChangeScalarsFilterTclName)
    {
    if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      }
    else if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
                    GetSourceClassName(), "vtkDataSetReader") == 0)
      {
      tempName = strtok(this->GetNthPVInput(0)->GetVTKDataTclName(), "O");
      *file << tempName << " GetOutput]\n\t";
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
  
  *file << this->VTKSourceTclName << " SetSource [";
  if (strncmp(this->GetNthPVInput(0)->GetVTKDataTclName(),
              "EnSight", 7) == 0)
    {
    sprintf(sourceTclName, "EnSightReader");
    tempName = strtok(this->GlyphSourceTclName, "O");
    strcat(sourceTclName, this->GlyphSourceTclName+7);
    *file << sourceTclName << " GetOutput ";
    charFound = strrchr(this->GlyphSourceTclName, 't');
    pos = charFound - this->GlyphSourceTclName + 1;
    *file << this->GlyphSourceTclName+pos << "]\n\t";
    }
  else if (strncmp(this->GetNthPVInput(0)->GetVTKDataTclName(),
                   "DataSet", 7) == 0)
    {
    sprintf(sourceTclName, "DataSetReader");
    tempName = strtok(this->GlyphSourceTclName, "O");
    strcat(sourceTclName, tempName+7);
    *file << sourceTclName << " GetOutput]\n\t";
    }
  else
    {
    char *whichSource;
    
    charFound = strrchr(this->GlyphSourceTclName, 't');
    pos = charFound - this->GlyphSourceTclName + 1;
    whichSource = this->GlyphSourceTclName + pos;
    tempName = strtok(this->GlyphSourceTclName, "O");
    *file << tempName << whichSource <<" GetOutput]\n\t";
    }

  *file << this->VTKSourceTclName << " SetScaleModeTo";
  if (strcmp(this->ScaleModeMenu->GetValue(), "Scalar") == 0)
    {
    *file << "ScaleByScalar\n\t";
    }
  else if (strcmp(this->ScaleModeMenu->GetValue(), "Vector") == 0)
    {
    *file << "ScaleByVector\n\t";
    }
  else if (strcmp(this->ScaleModeMenu->GetValue(), "Vector Components") == 0)
    {
    *file << "ScaleByVectorComponents\n\t";
    }
  else
    {
    *file << "DataScalingOff\n\t";
    }
  
  *file << this->VTKSourceTclName << " SetVectorModeTo";
  if (strcmp(this->VectorModeMenu->GetValue(), "Normal") == 0)
    {
    *file << "UseNormal\n\t";
    }
  else if (strcmp(this->VectorModeMenu->GetValue(), "Vector") == 0)
    {
    *file << "UseVector\n\t";
    }
  else
    {
    *file << "VectorRotationOff\n\t";
    }
  
  *file << this->VTKSourceTclName << " SetOrient "
        << this->OrientCheck->GetState() << "\n\t";
  *file << this->VTKSourceTclName << " SetScaling "
        << this->ScaleCheck->GetState() << "\n\t";
  *file << this->VTKSourceTclName << " SetScaleFactor "
        << this->ScaleEntry->GetValueAsFloat() << "\n\n";
  
  this->GetPVOutput(0)->SaveInTclScript(file, this->VTKSourceTclName);
}
