/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyph3D.cxx
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

  // Glyph adds too its input, so sould not replace it.
  this->ReplaceInput = 0;
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
                                 this->ScaleModeMenu->GetTclName(),
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
                                 this->VectorModeMenu->GetTclName(),
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
  vtkKWCompositeCollection *sources = this->GetWindow()->GetGlyphSources();
  vtkPVSource *source;
  char *tclName = NULL;
  
  this->GlyphSourceMenu->ClearEntries();
  for (i = 0; i < sources->GetNumberOfItems(); i++)
    {
    source = (vtkPVSource*)sources->GetItemAsObject(i);
    if (source->GetNthPVOutput(0))
      {
      if (source->GetNthPVOutput(0)->GetVTKData()->IsA("vtkPolyData"))
        {
        tclName = source->GetNthPVOutput(0)->GetVTKDataTclName();
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
  char sourceName[30];
  char *tempName;
  char *charFound;
  int pos;
  vtkPVSourceInterface *pvsInterface = this->GetInterface();
  
  if (this->ChangeScalarsFilterTclName)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
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

  if (strcmp(this->GlyphSourceTclName, "pvGlyphArrowOutput") == 0)
    {
    sprintf(sourceName, "pvGlyphArrow%d", this->InstanceCount);
    *file << "vtkArrowSource " << sourceName << "\n\n";
    }
  else if (strcmp(this->GlyphSourceTclName, "pvGlyphConeOutput") == 0)
    {
    sprintf(sourceName, "pvGlyphCone%d", this->InstanceCount);
    *file << "vtkConeSource " << sourceName << "\n\n";
    }
  else if (strcmp(this->GlyphSourceTclName, "pvGlyphSphereOutput") == 0)
    {
    sprintf(sourceName, "pvGlyphSphere%d", this->InstanceCount);
    *file << "vtkSphereSource " << sourceName << "\n\n";
    }
  
  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";
  
  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->ChangeScalarsFilterTclName)
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
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
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

  *file << sourceName <<" GetOutput]\n\t";

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
