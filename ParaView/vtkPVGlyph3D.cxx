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

int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGlyph3D::vtkPVGlyph3D()
{
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  this->Glyph3D = NULL;
  
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
  return new vtkPVGlyph3D();
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
  
  this->Glyph3D = (vtkGlyph3D*)this->GetVTKSource();
  if (!this->Glyph3D)
    {
    return;
    }

  this->GlyphSourceFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->GlyphSourceFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->GlyphSourceFrame->GetWidgetName());
  
  this->GlyphSourceLabel->SetParent(this->GlyphSourceFrame);
  this->GlyphSourceLabel->Create(pvApp, "");
  this->GlyphSourceLabel->SetLabel("Glyph Source:");
  
  this->GlyphSourceMenu->SetParent(this->GlyphSourceFrame);
  this->GlyphSourceMenu->Create(pvApp, "");
  this->GlyphSourceMenu->SetInputType("vtkPolyData");
  this->UpdateSourceMenu();

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
  
  this->ScaleModeMenu->SetParent(this->ScaleModeFrame);
  this->ScaleModeMenu->Create(pvApp, "");
  this->ScaleModeMenu->AddEntryWithCommand("Scalar", this,
                                           "ChangeScaleMode scalar");
  this->ScaleModeMenu->AddEntryWithCommand("Vector", this,
                                           "ChangeScaleMode vector");
  this->ScaleModeMenu->AddEntryWithCommand("Vector Components", this,
                                           "ChangeScaleMode components");
  this->ScaleModeMenu->AddEntryWithCommand("Data Scaling Off", this,
                                           "ChangeScaleMode off");
  this->ScaleModeMenu->SetCurrentEntry("Scalar");
  
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
  
  this->VectorModeMenu->SetParent(this->VectorModeFrame);
  this->VectorModeMenu->Create(pvApp, "");
  this->VectorModeMenu->AddEntryWithCommand("Normal", this,
                                            "ChangeVectorMode normal");
  this->VectorModeMenu->AddEntryWithCommand("Vector", this,
                                            "ChangeVectorMode vector");
  this->VectorModeMenu->AddEntryWithCommand("Vector Rotation Off", this,
                                            "ChangeVectorMode rotationOff");
  this->VectorModeMenu->SetCurrentEntry("Vector");
  
  this->Script("pack %s %s -side left",
               this->VectorModeLabel->GetWidgetName(),
               this->VectorModeMenu->GetWidgetName());
  
  this->OrientCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->OrientCheck->Create(pvApp, "-text Orient");
  this->OrientCheck->SetState(1);
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s GetOrient]",
                                  this->OrientCheck->GetTclName(),
                                  this->GetVTKSourceTclName()); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetOrient [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->OrientCheck->GetTclName());
  
  this->ScaleCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScaleCheck->Create(pvApp, "-text Scale");
  this->ScaleCheck->SetState(1);
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s GetScaling]",
                                  this->ScaleCheck->GetTclName(),
                                  this->GetVTKSourceTclName()); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetScaling [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->ScaleCheck->GetTclName());

  this->ScaleEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScaleEntry->Create(pvApp);
  this->ScaleEntry->SetLabel("Scale Factor:");
  this->ScaleEntry->SetValue("1.0");
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s GetScaleFactor]",
                                  this->ScaleEntry->GetTclName(),
                                  this->GetVTKSourceTclName()); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetScaleFactor [%s GetValueAsFloat]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->ScaleEntry->GetTclName());

  this->Script("pack %s %s %s",
               this->OrientCheck->GetWidgetName(),
               this->ScaleCheck->GetWidgetName(),
               this->ScaleEntry->GetWidgetName());
}

void vtkPVGlyph3D::ChangeScaleMode(const char *newMode)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (strcmp(newMode, "scalar") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToScaleByScalar",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "vector") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToScaleByVector",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "components") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToScaleByVectorComponents",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "off") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToDataScalingOff",
                           this->GetTclName());
    }
}

void vtkPVGlyph3D::ChangeVectorMode(const char *newMode)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (strcmp(newMode, "normal") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetVectorModeToUseNormal",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "vector") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetVectorModeToUseVector",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "rotationOff") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetVectorModeToVectorRotationOff",
                           this->GetTclName());
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
    if (currentSource->GetNthPVOutput(0)->GetVTKData()->IsA("vtkPolyData"))
      {
      tclName = currentSource->GetNthPVOutput(0)->GetVTKDataTclName();
      this->GlyphSourceMenu->AddEntryWithCommand(tclName, this,
                                                 "ChangeSource");
      }
    }
  if (tclName)
    {
    this->GlyphSourceMenu->SetValue(tclName);
    }
}

void vtkPVGlyph3D::ChangeSource()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *tclName;
  
  tclName = this->GlyphSourceMenu->GetValue();
  
  pvApp->BroadcastScript("[%s GetVTKSource] SetSource %s",
                         this->GetTclName(), tclName);
}
