/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBar.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
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

#include "vtkPVScalarBar.h"
#include "vtkKWView.h"
#include "vtkKWWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkPVSource.h"

int vtkPVScalarBarCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVScalarBar::vtkPVScalarBar()
{
  this->CommandFunction = vtkPVScalarBarCommand;
  this->ScalarBar = vtkScalarBarActor::New();
  this->ScalarBar->SetWidth(0.06);
  
  this->DataNotebookButton = vtkKWPushButton::New();
  this->OrientationMenu = vtkPVMenuButton::New();
  this->VisibilityButton = vtkKWCheckButton::New();
  this->TitleEntry = vtkKWLabeledEntry::New();
  this->WidthScale = vtkKWScale::New();
  this->HeightScale = vtkKWScale::New();
  this->XPositionScale = vtkKWScale::New();
  this->YPositionScale = vtkKWScale::New();
  this->Properties = vtkKWWidget::New();
  this->PVData = NULL;
  this->Visibility = 1;
}

//----------------------------------------------------------------------------
vtkPVScalarBar::~vtkPVScalarBar()
{
  this->ScalarBar->Delete();
  this->ScalarBar = NULL;
  this->DataNotebookButton->Delete();
  this->DataNotebookButton = NULL;
  this->OrientationMenu->Delete();
  this->OrientationMenu = NULL;
  this->VisibilityButton->Delete();
  this->VisibilityButton = NULL;
  this->TitleEntry->Delete();
  this->TitleEntry = NULL;
  this->ScalarBar->Delete();
  this->ScalarBar = NULL;
  this->WidthScale->Delete();
  this->WidthScale = NULL;
  this->HeightScale->Delete();
  this->HeightScale = NULL;
  this->XPositionScale->Delete();
  this->XPositionScale = NULL;
  this->YPositionScale->Delete();
  this->YPositionScale = NULL;
  this->Properties->Delete();
  this->Properties = NULL;
}

//----------------------------------------------------------------------------
vtkPVScalarBar *vtkPVScalarBar::New()
{
  return new vtkPVScalarBar();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::Clone(vtkPVApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);
  
  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());
  
  // The application is needed by the clones to send scalar ranges back.
  pvApp->BroadcastScript("%s SetApplication %s", this->GetTclName(),
			 pvApp->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::CreateProperties()
{
  const char *scalarBarPage;
  float *pos;

  // invoke superclass always
  this->vtkKWComposite::CreateProperties();
  
  scalarBarPage = this->GetClassName();
  this->Notebook->AddPage(scalarBarPage);
  this->Properties->SetParent(this->Notebook->GetFrame(scalarBarPage));
  this->Properties->Create(this->Application, "frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  this->DataNotebookButton->SetParent(this->Properties);
  this->DataNotebookButton->Create(this->Application, "");
  this->DataNotebookButton->SetLabel("Return to Data Notebook");
  this->DataNotebookButton->SetCommand(this, "ShowDataNotebook");
  
  this->OrientationMenu->SetParent(this->Properties);
  this->OrientationMenu->Create(this->Application, "");
  this->OrientationMenu->SetButtonText("Orientation");
  this->OrientationMenu->AddCommand("Horizontal", this,
                                    "SetOrientationToHorizontal");
  this->OrientationMenu->AddCommand("Vertical", this,
                                    "SetOrientationToVertical");
  
  this->VisibilityButton->SetParent(this->Properties);
  this->VisibilityButton->Create(this->Application, "-text Visibility");
  this->VisibilityButton->SetCommand(this, "SetScalarBarVisibility");
  this->VisibilityButton->SetState(this->ScalarBar->GetVisibility());
  
  this->TitleEntry->SetParent(this->Properties);
  this->TitleEntry->Create(this->Application);
  this->TitleEntry->SetLabel("Scalar Bar Title:");
  this->Script("bind %s <KeyPress-Return> {%s SetScalarBarTitle}",
	       this->TitleEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->WidthScale->SetParent(this->Properties);
  this->WidthScale->Create(this->Application, "-showvalue 1");
  this->WidthScale->DisplayLabel("Width");
  this->WidthScale->SetRange(0.0, 1.0);
  this->WidthScale->SetResolution(0.01);
  this->WidthScale->SetValue(this->ScalarBar->GetWidth());
  this->WidthScale->SetCommand(this, "SetScalarBarWidth");
  
  this->HeightScale->SetParent(this->Properties);
  this->HeightScale->Create(this->Application, "-showvalue 1");
  this->HeightScale->DisplayLabel("Height");
  this->HeightScale->SetRange(0.0, 1.0);
  this->HeightScale->SetResolution(0.01);
  this->HeightScale->SetValue(this->ScalarBar->GetHeight());
  this->HeightScale->SetCommand(this, "SetScalarBarHeight");

  pos = this->ScalarBar->GetPosition();

  this->XPositionScale->SetParent(this->Properties);
  this->XPositionScale->Create(this->Application, "-showvalue 1");
  this->XPositionScale->DisplayLabel("Lower Left Corner X");
  this->XPositionScale->SetRange(0.0, 1.0);
  this->XPositionScale->SetResolution(0.01);
  this->XPositionScale->SetValue(pos[0]);
  this->XPositionScale->SetCommand(this, "SetScalarBarXPosition");

  this->YPositionScale->SetParent(this->Properties);
  this->YPositionScale->Create(this->Application, "-showvalue 1");
  this->YPositionScale->DisplayLabel("Lower Left Corner Y");
  this->YPositionScale->SetRange(0.0, 1.0);
  this->YPositionScale->SetResolution(0.01);
  this->YPositionScale->SetValue(pos[1]);
  this->YPositionScale->SetCommand(this, "SetScalarBarYPosition");

  this->Script("pack %s %s %s %s",
               this->DataNotebookButton->GetWidgetName(),
               this->OrientationMenu->GetWidgetName(),
               this->VisibilityButton->GetWidgetName(),
               this->TitleEntry->GetWidgetName());
  this->Script("pack %s %s %s %s -fill x",
               this->WidthScale->GetWidgetName(),
               this->HeightScale->GetWidgetName(),
	       this->XPositionScale->GetWidgetName(),
	       this->YPositionScale->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::ShowProperties()
{
  vtkKWWindow *pw = this->View->GetParentWindow();
  pw->ShowProperties();

  // unpack any current children
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->View->GetPropertiesParent()->GetWidgetName());
  
  if (!this->PropertiesCreated)
    {
    this->InitializeProperties();
    }
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->View->PackProperties();
}
  
//----------------------------------------------------------------------------
vtkProp *vtkPVScalarBar::GetProp()
{
  return this->ScalarBar;
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetScalarBarXPosition()
{
  float *pos;

  pos = this->ScalarBar->GetPosition();

  this->ScalarBar->GetPositionCoordinate()->
    SetCoordinateSystemToNormalizedViewport();
  this->ScalarBar->GetPositionCoordinate()->
    SetValue(this->XPositionScale->GetValue(), pos[1]);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetScalarBarYPosition()
{
  float *pos;

  pos = this->ScalarBar->GetPosition();

  this->ScalarBar->GetPositionCoordinate()->
    SetCoordinateSystemToNormalizedViewport();
  this->ScalarBar->GetPositionCoordinate()->
    SetValue(pos[0], this->YPositionScale->GetValue());
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetScalarBarWidth()
{
  this->ScalarBar->SetWidth(this->WidthScale->GetValue());
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetScalarBarHeight()
{
  this->ScalarBar->SetHeight(this->HeightScale->GetValue());
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetScalarBarTitle()
{
  this->ScalarBar->SetTitle(this->TitleEntry->GetValue());
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetScalarBarVisibility()
{
  if (this->VisibilityButton->GetState())
    {
    this->VisibilityOn();
    }
  else
    {
    this->VisibilityOff();
    }
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetVisibility(int v)
{
  this->ScalarBar->SetVisibility(v);
}

//----------------------------------------------------------------------------
int vtkPVScalarBar::GetVisibility()
{
  return this->ScalarBar->GetVisibility();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetOrientationToHorizontal()
{
  //  float tmp;
  //  float *pos;
  
  //  if (this->ScalarBar->GetOrientation() == VTK_ORIENT_VERTICAL)
  //    {
  //    pos = this->ScalarBar->GetPosition();
  //    this->ScalarBar->GetPositionCoordinate()->
  //      SetCoordinateSystemToNormalizedViewport();
  //    this->ScalarBar->GetPositionCoordinate()->
  //      SetValue((1.0 - pos[0])/2.0, pos[1]);
  //    tmp = this->ScalarBar->GetHeight();
  //    this->ScalarBar->SetHeight(this->ScalarBar->GetWidth());
  //    this->HeightScale->SetValue(this->ScalarBar->GetHeight());
  //    this->ScalarBar->SetWidth(tmp);
  //    this->WidthScale->SetValue(this->ScalarBar->GetWidth());
    this->ScalarBar->SetOrientationToHorizontal();
    this->GetView()->Render();
    //    }
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetOrientationToVertical()
{
  //  float tmp;
  //  float *pos;
  
  //  if (this->ScalarBar->GetOrientation() == VTK_ORIENT_HORIZONTAL)
  //    {
  //    pos = this->ScalarBar->GetPosition();
  //    this->ScalarBar->GetPositionCoordinate()->
  //      SetCoordinateSystemToNormalizedViewport();
  //    this->ScalarBar->GetPositionCoordinate()->
  //      SetValue(1.0 - 2*pos[0], pos[1]);
  //    tmp = this->ScalarBar->GetHeight();
  //    this->ScalarBar->SetHeight(this->ScalarBar->GetWidth());
  //    this->HeightScale->SetValue(this->ScalarBar->GetHeight());
  //    this->ScalarBar->SetWidth(tmp);
  //    this->WidthScale->SetValue(this->ScalarBar->GetWidth());
    this->ScalarBar->SetOrientationToVertical();
    this->GetView()->Render();
    //    }
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::ShowDataNotebook()
{
  this->GetPVData()->GetPVSource()->ShowProperties();
}

//----------------------------------------------------------------------------
void vtkPVScalarBar::SetPVData(vtkPVData *data)
{
  if (this->PVData == data)
    {
    return;
    }
  this->Modified();
  
  if (this->PVData)
    {
    // extra careful for circular references
    vtkPVData *tmp = this->PVData;
    this->PVData = NULL;
    // Manage double pointer.
    tmp->SetScalarBar(NULL);
    tmp->UnRegister(this);
    }
  
  if (data)
    {
    this->PVData = data;
    data->Register(this);
    // Manage double pointer.
    data->SetScalarBar(this);
    this->ScalarBar->SetLookupTable(data->GetActorComposite()->
                                    GetMapper()->GetLookupTable());
    }
}
