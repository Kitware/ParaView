/*=========================================================================

  Module:    vtkKWSpinButtons.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSpinButtons.h"
#include "vtkKWPushButton.h"
#include "vtkKWIcon.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWSpinButtons );
vtkCxxRevisionMacro(vtkKWSpinButtons, "1.5");

//----------------------------------------------------------------------------
vtkKWSpinButtons::vtkKWSpinButtons()
{
  this->PreviousButton = vtkKWPushButton::New();
  this->NextButton = vtkKWPushButton::New();

  this->ArrowOrientation = vtkKWSpinButtons::ArrowOrientationVertical;
  this->LayoutOrientation = vtkKWSpinButtons::LayoutOrientationVertical;

  this->ButtonsPadX = 0;
  this->ButtonsPadY = 0;
}

//----------------------------------------------------------------------------
vtkKWSpinButtons::~vtkKWSpinButtons()
{
  if (this->PreviousButton)
    {
    this->PreviousButton->Delete();
    this->PreviousButton = NULL;
    }

  if (this->NextButton)
    {
    this->NextButton->Delete();
    this->NextButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->PreviousButton->SetParent(this);
  this->PreviousButton->Create();
  this->PreviousButton->SetPadX(0);
  this->PreviousButton->SetPadY(this->PreviousButton->GetPadX());

  this->NextButton->SetParent(this);
  this->NextButton->Create();
  this->NextButton->SetPadX(this->PreviousButton->GetPadX());
  this->NextButton->SetPadY(this->PreviousButton->GetPadY());
  
  this->UpdateArrowOrientation();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetArrowOrientation(int val)
{
  if (val < vtkKWSpinButtons::ArrowOrientationHorizontal)
    {
    val = vtkKWSpinButtons::ArrowOrientationHorizontal;
    }
  if (val > vtkKWSpinButtons::ArrowOrientationVertical)
    {
    val = vtkKWSpinButtons::ArrowOrientationVertical;
    }

  if (this->ArrowOrientation == val)
    {
    return;
    }

  this->ArrowOrientation = val;
  this->Modified();

  this->UpdateArrowOrientation();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::UpdateArrowOrientation()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->ArrowOrientation == vtkKWSpinButtons::ArrowOrientationVertical)  
    {
    if (this->PreviousButton && this->PreviousButton->IsCreated())
      {
      this->PreviousButton->SetImageToPredefinedIcon(vtkKWIcon::IconSpinUp);
      }
    if (this->NextButton && this->NextButton->IsCreated())
      {
      this->NextButton->SetImageToPredefinedIcon(vtkKWIcon::IconSpinDown);
      }
    }
  else
    {
    if (this->PreviousButton && this->PreviousButton->IsCreated())
      {
      this->PreviousButton->SetImageToPredefinedIcon(vtkKWIcon::IconSpinLeft);
      }
    if (this->NextButton && this->NextButton->IsCreated())
      {
      this->NextButton->SetImageToPredefinedIcon(vtkKWIcon::IconSpinRight);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetLayoutOrientation(int val)
{
  if (val < vtkKWSpinButtons::LayoutOrientationHorizontal)
    {
    val = vtkKWSpinButtons::LayoutOrientationHorizontal;
    }
  if (val > vtkKWSpinButtons::LayoutOrientationVertical)
    {
    val = vtkKWSpinButtons::LayoutOrientationVertical;
    }

  if (this->LayoutOrientation == val)
    {
    return;
    }

  this->LayoutOrientation = val;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetButtonsPadX(int arg)
{
  if (arg == this->ButtonsPadX)
    {
    return;
    }

  this->ButtonsPadX = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetButtonsPadY(int arg)
{
  if (arg == this->ButtonsPadY)
    {
    return;
    }

  this->ButtonsPadY = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *next, *prev;
  if (this->LayoutOrientation == vtkKWSpinButtons::LayoutOrientationVertical)  
    {
    prev = "top";
    next = "bottom";
    }
  else
    {
    prev = "left";
    next = "right";
    }
  if (this->PreviousButton && this->PreviousButton->IsCreated())
    {
    this->Script(
      "pack %s -side %s -expand y -fill both -padx %d -pady %d",
      this->PreviousButton->GetWidgetName(), prev,
      this->ButtonsPadX, this->ButtonsPadY);
    }
  if (this->NextButton && this->NextButton->IsCreated())
    {
    this->Script(
      "pack %s -side %s -expand y -fill both -padx %d -pady %d",
      this->NextButton->GetWidgetName(), next,
      this->ButtonsPadX, this->ButtonsPadY);
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetButtonsWidth(int w)
{
  if (this->PreviousButton)
    {
    this->PreviousButton->SetWidth(w);
    }
  if (this->NextButton)
    {
    this->NextButton->SetWidth(w);
    }
}

//----------------------------------------------------------------------------
int vtkKWSpinButtons::GetButtonsWidth()
{
  int d_w = this->PreviousButton ? this->PreviousButton->GetWidth() : 0;
  int i_w = this->NextButton ? this->NextButton->GetWidth() : 0;
  return d_w > i_w ? d_w : i_w;
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetButtonsHeight(int h)
{
  if (this->PreviousButton)
    {
    this->PreviousButton->SetHeight(h);
    }
  if (this->NextButton)
    {
    this->NextButton->SetHeight(h);
    }
}

//----------------------------------------------------------------------------
int vtkKWSpinButtons::GetButtonsHeight()
{
  int d_h = this->PreviousButton ? this->PreviousButton->GetHeight() : 0;
  int i_h = this->NextButton ? this->NextButton->GetHeight() : 0;
  return d_h > i_h ? d_h : i_h;
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetPreviousCommand(
  vtkObject *object, const char *method)
{
  if (this->PreviousButton)
    {
    this->PreviousButton->SetCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetNextCommand(
  vtkObject *object, const char *method)
{
  if (this->NextButton)
    {
    this->NextButton->SetCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->PreviousButton);
  this->PropagateEnableState(this->NextButton);
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PreviousButton: " << this->PreviousButton << endl;
  os << indent << "NextButton: " << this->NextButton << endl;
  if (this->ArrowOrientation == vtkKWSpinButtons::ArrowOrientationHorizontal)
    {
    os << indent << "ArrowOrientation: Horizontal\n";
    }
  else
    {
    os << indent << "ArrowOrientation: Vertical\n";
    }
  if (this->LayoutOrientation == vtkKWSpinButtons::LayoutOrientationHorizontal)
    {
    os << indent << "LayoutOrientation: Horizontal\n";
    }
  else
    {
    os << indent << "LayoutOrientation: Vertical\n";
    }
  os << indent << "ButtonsPadX: " << this->ButtonsPadX << endl;
  os << indent << "ButtonsPadY: " << this->ButtonsPadY << endl;
}

