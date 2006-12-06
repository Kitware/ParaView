/*=========================================================================

  Module:    vtkKWMatrixWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMatrixWidget.h"

#include "vtkKWEntrySet.h"
#include "vtkKWEntry.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMatrixWidget );
vtkCxxRevisionMacro(vtkKWMatrixWidget, "1.1");

//----------------------------------------------------------------------------
vtkKWMatrixWidget::vtkKWMatrixWidget()
{
  this->NumberOfColumns = 4;
  this->NumberOfRows    = 4;
  this->EntrySet        = vtkKWEntrySet::New();
  this->Width           = 6;
  this->ReadOnly        = 0;
  this->RestrictValue   = vtkKWMatrixWidget::RestrictDouble;
}

//----------------------------------------------------------------------------
vtkKWMatrixWidget::~vtkKWMatrixWidget()
{
  if (this->EntrySet)
    {
    this->EntrySet->SetParent(NULL);
    this->EntrySet->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->EntrySet->SetParent(this);
  this->EntrySet->Create();
  this->EntrySet->PackHorizontallyOn();
  this->EntrySet->SetWidgetsPadX(1);
  this->EntrySet->SetWidgetsPadY(1);
  this->EntrySet->ExpandWidgetsOff();

  this->Script("pack %s -fill both",
               this->EntrySet->GetWidgetName());

  this->UpdateWidget();
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetNumberOfColumns(int arg)
{
  if (this->NumberOfColumns == arg || arg <= 0)
    {
    return;
    }

  this->NumberOfColumns = arg;
  this->Modified();
  
  this->UpdateWidget();
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetNumberOfRows(int arg)
{
  if (this->NumberOfRows == arg || arg <= 0)
    {
    return;
    }

  this->NumberOfRows = arg;
  this->Modified();
  
  this->UpdateWidget();
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetValue(int row, int col, const char *val)
{
  if (this->EntrySet && this->EntrySet->IsCreated() &&
      row >= 0 && row < this->NumberOfRows && 
      col >= 0 && col < this->NumberOfColumns)
    {
    int id = col + row * this->NumberOfColumns;
    this->EntrySet->GetWidget(id)->SetValue(val);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWMatrixWidget::GetValue(int row, int col)
{
  if (this->EntrySet && this->EntrySet->IsCreated() &&
      row >= 0 && row < this->NumberOfRows && 
      col >= 0 && col < this->NumberOfColumns)
    {
    int id = col + row * this->NumberOfColumns;
    return this->EntrySet->GetWidget(id)->GetValue();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetValueAsInt(int row, int col, int val)
{
  if (this->EntrySet && this->EntrySet->IsCreated() &&
      row >= 0 && row < this->NumberOfRows && 
      col >= 0 && col < this->NumberOfColumns)
    {
    int id = col + row * this->NumberOfColumns;
    this->EntrySet->GetWidget(id)->SetValueAsInt(val);
    }
}

//----------------------------------------------------------------------------
int vtkKWMatrixWidget::GetValueAsInt(int row, int col)
{
  if (this->EntrySet && this->EntrySet->IsCreated() &&
      row >= 0 && row < this->NumberOfRows && 
      col >= 0 && col < this->NumberOfColumns)
    {
    int id = col + row * this->NumberOfColumns;
    return this->EntrySet->GetWidget(id)->GetValueAsInt();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetValueAsDouble(int row, int col, double val)
{
  if (this->EntrySet && this->EntrySet->IsCreated() &&
      row >= 0 && row < this->NumberOfRows && 
      col >= 0 && col < this->NumberOfColumns)
    {
    int id = col + row * this->NumberOfColumns;
    this->EntrySet->GetWidget(id)->SetValueAsDouble(val);
    }
}

//----------------------------------------------------------------------------
double vtkKWMatrixWidget::GetValueAsDouble(int row, int col)
{
  if (this->EntrySet && this->EntrySet->IsCreated() &&
      row >= 0 && row < this->NumberOfRows && 
      col >= 0 && col < this->NumberOfColumns)
    {
    int id = col + row * this->NumberOfColumns;
    return this->EntrySet->GetWidget(id)->GetValueAsDouble();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::UpdateWidget()
{
  if (!this->EntrySet->IsCreated())
    {
    return;
    }

  int i;

  this->EntrySet->SetMaximumNumberOfWidgetsInPackingDirection(
    this->NumberOfColumns);

  int nb_requested_entries = this->NumberOfColumns * this->NumberOfRows;
  int nb_entries = this->EntrySet->GetNumberOfWidgets();
  while (nb_entries < nb_requested_entries)
    {
    vtkKWEntry *entry = this->EntrySet->AddWidget(nb_entries++);
    if (entry)
      {
      entry->SetWidth(this->Width);
      entry->SetReadOnly(this->ReadOnly);
      entry->SetRestrictValue(this->RestrictValue);
      }
    }

  for (i = 0; i < nb_requested_entries; i++)
    {
    this->EntrySet->SetWidgetVisibility(i, 1);
    }
  for (i = nb_requested_entries; i < nb_entries; i++)
    {
    this->EntrySet->SetWidgetVisibility(i, 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetRestrictValue(int arg)
{
  if (this->RestrictValue == arg)
    {
    return;
    }

  this->RestrictValue = arg;
  this->Modified();

  if (this->EntrySet->IsCreated())
    {
    for (int i = 0; i < this->EntrySet->GetNumberOfWidgets(); i++)
      {
      vtkKWEntry *entry = this->EntrySet->GetWidget(i);
      if (entry)
        {
        entry->SetRestrictValue(this->RestrictValue);
        }
      }
    }
}

void vtkKWMatrixWidget::SetRestrictValueToInteger()
{ 
  this->SetRestrictValue(vtkKWMatrixWidget::RestrictInteger); 
}

void vtkKWMatrixWidget::SetRestrictValueToDouble()
{ 
  this->SetRestrictValue(vtkKWMatrixWidget::RestrictDouble); 
}

void vtkKWMatrixWidget::SetRestrictValueToNone()
{ 
  this->SetRestrictValue(vtkKWMatrixWidget::RestrictNone); 
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Width = width;
  this->Modified();

  if (this->EntrySet->IsCreated())
    {
    for (int i = 0; i < this->EntrySet->GetNumberOfWidgets(); i++)
      {
      vtkKWEntry *entry = this->EntrySet->GetWidget(i);
      if (entry)
        {
        entry->SetWidth(this->Width);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetReadOnly(int arg)
{
  if (this->ReadOnly == arg)
    {
    return;
    }

  this->ReadOnly = arg;
  this->Modified();

  if (this->EntrySet->IsCreated())
    {
    for (int i = 0; i < this->EntrySet->GetNumberOfWidgets(); i++)
      {
      vtkKWEntry *entry = this->EntrySet->GetWidget(i);
      if (entry)
        {
        entry->SetReadOnly(this->ReadOnly);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->EntrySet)
    {
    this->EntrySet->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Width: " << this->GetWidth() << endl;
  os << indent << "Readonly: " << (this->ReadOnly ? "On" : "Off") << endl;
  os << indent << "RestrictValue: " << this->RestrictValue << endl;
}

