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
  return new vtkPVGlyph3D();
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
  
  this->GlyphSourceMenu->SetParent(this->GlyphSourceFrame);
  this->GlyphSourceMenu->Create(pvApp, "");
  this->GlyphSourceMenu->SetInputType("vtkPolyData");
  this->UpdateSourceMenu();
  if (this->GlyphSourceTclName)
    {
    this->GlyphSourceMenu->SetValue(this->GlyphSourceTclName);
    }
  this->AcceptCommands->AddString("%s ChangeSource",
                                  this->GetTclName());
  this->CancelCommands->AddString("%s SetValue %s",
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
  
  this->ScaleModeMenu->SetParent(this->ScaleModeFrame);
  this->ScaleModeMenu->Create(pvApp, "");
  this->ScaleModeMenu->AddEntry("Scalar");
  this->ScaleModeMenu->AddEntry("Vector");
  this->ScaleModeMenu->AddEntry("Vector Components");
  this->ScaleModeMenu->AddEntry("Data Scaling Off");
  this->ScaleModeMenu->SetValue("Scalar");
  this->SetGlyphScaleMode("Scalar");
  
  this->AcceptCommands->AddString("%s ChangeScaleMode",
                                  this->GetTclName());
  this->CancelCommands->AddString("%s SetValue %s",
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
  
  this->VectorModeMenu->SetParent(this->VectorModeFrame);
  this->VectorModeMenu->Create(pvApp, "");
  this->VectorModeMenu->AddEntry("Normal");
  this->VectorModeMenu->AddEntry("Vector");
  this->VectorModeMenu->AddEntry("Vector Rotation Off");
  this->VectorModeMenu->SetValue("Vector");
  this->SetGlyphVectorMode("Vector");
  
  this->AcceptCommands->AddString("%s ChangeVectorMode",
                                  this->GetTclName());
  this->CancelCommands->AddString("%s SetValue %s",
                                  this->GetTclName(),
                                  this->GetGlyphVectorMode());
  
  this->Script("pack %s %s -side left",
               this->VectorModeLabel->GetWidgetName(),
               this->VectorModeMenu->GetWidgetName());
  
  this->OrientCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->OrientCheck->Create(pvApp, "-text Orient");
  this->OrientCheck->SetState(1);
  
  this->CancelCommands->AddString("%s SetState [%s GetOrient]",
                                  this->OrientCheck->GetTclName(),
                                  this->GetVTKSourceTclName()); 
  this->AcceptCommands->AddString("%s AcceptHelper2 %s SetOrient [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  this->OrientCheck->GetTclName());
  
  this->ScaleCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScaleCheck->Create(pvApp, "-text Scale");
  this->ScaleCheck->SetState(1);
  
  this->CancelCommands->AddString("%s SetState [%s GetScaling]",
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
  
  this->CancelCommands->AddString("%s SetValue [%s GetScaleFactor]",
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
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToScaleByScalar",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "Vector") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToScaleByVector",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "Vector Components") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToScaleByVectorComponents",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "Data Scaling Off") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetScaleModeToDataScalingOff",
                           this->GetTclName());
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
    pvApp->BroadcastScript("[%s GetVTKSource] SetVectorModeToUseNormal",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "Vector") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetVectorModeToUseVector",
                           this->GetTclName());
    }
  else if (strcmp(newMode, "Vector Rotation Off") == 0)
    {
    pvApp->BroadcastScript("[%s GetVTKSource] SetVectorModeToVectorRotationOff",
                           this->GetTclName());
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
    if (currentSource->GetNthPVOutput(0)->GetVTKData()->IsA("vtkPolyData"))
      {
      tclName = currentSource->GetNthPVOutput(0)->GetVTKDataTclName();
      if (!this->GlyphSourceTclName)
        {
        this->SetGlyphSourceTclName(tclName);
        }
      this->GlyphSourceMenu->AddEntry(tclName);
      }
    }
}

void vtkPVGlyph3D::ChangeSource()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *tclName;
  
  tclName = this->GlyphSourceMenu->GetValue();
  this->SetGlyphSourceTclName(tclName);
  
  pvApp->BroadcastScript("[%s GetVTKSource] SetSource %s",
                         this->GetTclName(), tclName);
}
