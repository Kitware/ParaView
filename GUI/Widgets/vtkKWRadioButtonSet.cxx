/*=========================================================================

  Module:    vtkKWRadioButtonSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWRadioButtonSet.h"

#include "vtkKWRadioButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWRadioButtonSet);
vtkCxxRevisionMacro(vtkKWRadioButtonSet, "1.23");

//----------------------------------------------------------------------------
vtkKWRadioButton* vtkKWRadioButtonSet::GetWidget(int id)
{
  return static_cast<vtkKWRadioButton*>(this->GetWidgetInternal(id));
}

//----------------------------------------------------------------------------
vtkKWRadioButton* vtkKWRadioButtonSet::AddWidget(int id)
{
  return static_cast<vtkKWRadioButton*>(this->InsertWidgetInternal(
                                          id, this->GetNumberOfWidgets()));
}

//----------------------------------------------------------------------------
vtkKWRadioButton* vtkKWRadioButtonSet::InsertWidget(int id, int pos)
{
  return static_cast<vtkKWRadioButton*>(this->InsertWidgetInternal(
                                          id, pos));
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWRadioButtonSet::InsertWidgetInternal(
  int id, int pos)
{
  vtkKWRadioButton *widget = static_cast<vtkKWRadioButton*>(
    this->Superclass::InsertWidgetInternal(id, pos));
  if (widget)
    {
    widget->SetValueAsInt(id);
    }
  return widget;
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWRadioButtonSet::AllocateAndCreateWidget()
{
  vtkKWRadioButton *widget = vtkKWRadioButton::New();
  widget->SetParent(this);
  widget->Create();

  // For convenience, all radiobuttons share the same var name

  if (this->GetNumberOfWidgets())
    {
    vtkKWRadioButton *first = this->GetWidget(this->GetIdOfNthWidget(0));
    if (first)
      {
      widget->SetVariableName(first->GetVariableName());
      }
    }

  return static_cast<vtkKWWidget*>(widget);
}

//----------------------------------------------------------------------------
void vtkKWRadioButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
