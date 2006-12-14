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
vtkCxxRevisionMacro(vtkKWMatrixWidget, "1.6");

//----------------------------------------------------------------------------
vtkKWMatrixWidget::vtkKWMatrixWidget()
{
  this->NumberOfColumns       = 1;
  this->NumberOfRows          = 1;
  this->EntrySet              = vtkKWEntrySet::New();
  this->ElementWidth          = 5;
  this->ReadOnly              = 0;
  this->RestrictElementValue  = vtkKWMatrixWidget::RestrictDouble;
  this->ElementChangedCommand = NULL;
  this->ElementChangedCommandTrigger      = (vtkKWMatrixWidget::TriggerOnFocusOut | 
                               vtkKWMatrixWidget::TriggerOnReturnKey);
}

//----------------------------------------------------------------------------
vtkKWMatrixWidget::~vtkKWMatrixWidget()
{
  if (this->EntrySet)
    {
    this->EntrySet->SetParent(NULL);
    this->EntrySet->Delete();
    }

  if (this->ElementChangedCommand)
    {
    delete [] this->ElementChangedCommand;
    this->ElementChangedCommand = NULL;
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
  if (this->NumberOfColumns == arg || arg < 0)
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
  if (this->NumberOfRows == arg || arg < 0)
    {
    return;
    }

  this->NumberOfRows = arg;
  this->Modified();
  
  this->UpdateWidget();
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetElementValue(int row, int col, const char *val)
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
const char* vtkKWMatrixWidget::GetElementValue(int row, int col)
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
void vtkKWMatrixWidget::SetElementValueAsInt(int row, int col, int val)
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
int vtkKWMatrixWidget::GetElementValueAsInt(int row, int col)
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
void vtkKWMatrixWidget::SetElementValueAsDouble(int row, int col, double val)
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
double vtkKWMatrixWidget::GetElementValueAsDouble(int row, int col)
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

  int entry_trigger = 0;
  if (this->ElementChangedCommandTrigger & 
      vtkKWMatrixWidget::TriggerOnFocusOut)
    {
    entry_trigger |= vtkKWEntry::TriggerOnFocusOut;
    }
  if (this->ElementChangedCommandTrigger & 
      vtkKWMatrixWidget::TriggerOnReturnKey)
    {
    entry_trigger |= vtkKWEntry::TriggerOnReturnKey;
    }
  if (this->ElementChangedCommandTrigger & 
      vtkKWMatrixWidget::TriggerOnAnyChange)
    {
    entry_trigger |= vtkKWEntry::TriggerOnAnyChange;
    }

  char command[256];

  int nb_requested_entries = this->NumberOfColumns * this->NumberOfRows;
  int nb_entries = this->EntrySet->GetNumberOfWidgets();
  while (nb_entries < nb_requested_entries)
    {
    int id = nb_entries++;
    vtkKWEntry *entry = this->EntrySet->AddWidget(id);
    if (entry)
      {
      entry->SetWidth(this->ElementWidth);
      entry->SetReadOnly(this->ReadOnly);
      entry->SetRestrictValue(this->RestrictElementValue);
      sprintf(command, "ElementChangedCallback %d", id);
      entry->SetCommand(this, command);
      entry->SetCommandTrigger(entry_trigger);
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
void vtkKWMatrixWidget::SetRestrictElementValue(int arg)
{
  if (this->RestrictElementValue == arg)
    {
    return;
    }

  this->RestrictElementValue = arg;
  this->Modified();

  if (this->EntrySet->IsCreated())
    {
    for (int i = 0; i < this->EntrySet->GetNumberOfWidgets(); i++)
      {
      vtkKWEntry *entry = this->EntrySet->GetWidget(i);
      if (entry)
        {
        entry->SetRestrictValue(this->RestrictElementValue);
        }
      }
    }
}

void vtkKWMatrixWidget::SetRestrictElementValueToInteger()
{ 
  this->SetRestrictElementValue(vtkKWMatrixWidget::RestrictInteger); 
}

void vtkKWMatrixWidget::SetRestrictElementValueToDouble()
{ 
  this->SetRestrictElementValue(vtkKWMatrixWidget::RestrictDouble); 
}

void vtkKWMatrixWidget::SetRestrictElementValueToNone()
{ 
  this->SetRestrictElementValue(vtkKWMatrixWidget::RestrictNone); 
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetElementWidth(int width)
{
  if (this->ElementWidth == width)
    {
    return;
    }

  this->ElementWidth = width;
  this->Modified();

  if (this->EntrySet->IsCreated())
    {
    for (int i = 0; i < this->EntrySet->GetNumberOfWidgets(); i++)
      {
      vtkKWEntry *entry = this->EntrySet->GetWidget(i);
      if (entry)
        {
        entry->SetWidth(this->ElementWidth);
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
void vtkKWMatrixWidget::SetElementChangedCommandTrigger(int arg)
{
  if (this->ElementChangedCommandTrigger == arg)
    {
    return;
    }

  this->ElementChangedCommandTrigger = arg;
  this->Modified();

  int entry_trigger = 0;
  if (this->ElementChangedCommandTrigger & 
      vtkKWMatrixWidget::TriggerOnFocusOut)
    {
    entry_trigger |= vtkKWEntry::TriggerOnFocusOut;
    }
  if (this->ElementChangedCommandTrigger & 
      vtkKWMatrixWidget::TriggerOnReturnKey)
    {
    entry_trigger |= vtkKWEntry::TriggerOnReturnKey;
    }
  if (this->ElementChangedCommandTrigger & 
      vtkKWMatrixWidget::TriggerOnAnyChange)
    {
    entry_trigger |= vtkKWEntry::TriggerOnAnyChange;
    }

  if (this->EntrySet->IsCreated())
    {
    for (int i = 0; i < this->EntrySet->GetNumberOfWidgets(); i++)
      {
      vtkKWEntry *entry = this->EntrySet->GetWidget(i);
      if (entry)
        {
        entry->SetCommandTrigger(entry_trigger);
        }
      }
    }
}

void vtkKWMatrixWidget::SetElementChangedCommandTriggerToReturnKeyAndFocusOut()
{ 
  this->SetElementChangedCommandTrigger(
    vtkKWMatrixWidget::TriggerOnFocusOut | 
    vtkKWMatrixWidget::TriggerOnReturnKey); 
}

void vtkKWMatrixWidget::SetElementChangedCommandTriggerToAnyChange()
{ 
  this->SetElementChangedCommandTrigger(vtkKWMatrixWidget::TriggerOnAnyChange); 
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::ElementChangedCallback(int id, const char *value)
{
  int rank = this->EntrySet->GetWidgetPosition(id);
  if (this->NumberOfColumns && this->NumberOfRows)
    {
    int row = rank / this->NumberOfColumns;
    int col = rank % this->NumberOfColumns;
    this->InvokeElementChangedCommand(row, col, value);
    }
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::SetElementChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->ElementChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWMatrixWidget::InvokeElementChangedCommand(
  int row, int col, const char *value)
{
  if (this->ElementChangedCommand && *this->ElementChangedCommand && 
      this->IsCreated())
    {
    const char *val = this->ConvertInternalStringToTclString(
      value, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    this->Script("%s %d %d \"%s\"", 
                 this->ElementChangedCommand, row, col, val ? val : "");
    }

  void *data[3];
  data[0] = &row;
  data[1] = &col;
  data[2] = &value;
  
  this->InvokeEvent(vtkKWMatrixWidget::ElementChangedEvent, data);
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
  os << indent << "RestrictElementValue: " << this->RestrictElementValue << endl;
}

