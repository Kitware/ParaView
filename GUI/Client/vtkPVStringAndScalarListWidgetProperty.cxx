/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringAndScalarListWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStringAndScalarListWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVObjectWidget.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWidget.h"
#include "vtkStringList.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkPVStringAndScalarListWidgetProperty);
vtkCxxRevisionMacro(vtkPVStringAndScalarListWidgetProperty, "1.10");

vtkPVStringAndScalarListWidgetProperty::vtkPVStringAndScalarListWidgetProperty()
{
  this->Strings = vtkStringList::New();
  this->NumberOfStringsPerCommand = NULL;
}

vtkPVStringAndScalarListWidgetProperty::~vtkPVStringAndScalarListWidgetProperty()
{
  this->Strings->Delete();
  if (this->NumberOfStringsPerCommand)
    {
    delete [] this->NumberOfStringsPerCommand;
    this->NumberOfStringsPerCommand = NULL;
    }
}

void vtkPVStringAndScalarListWidgetProperty::AcceptInternal()
{
  int i, j, scalarCount = 0, stringCount = 0;
  vtkClientServerStream& stream = 
    this->Widget->GetPVApplication()->GetProcessModule()->GetStream();
  
  for (i = 0; i < this->NumberOfCommands; i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->VTKSourceID << this->VTKCommands[i];
    for (j = 0; j < this->NumberOfStringsPerCommand[i]; j++)
      {
      stream << this->Strings->GetString(stringCount);
      stringCount++;
      }
    for (j = 0; j < this->NumberOfScalarsPerCommand[i]; j++)
      {
      stream << this->Scalars[scalarCount];
      scalarCount++;
      }
    stream << vtkClientServerStream::End;
    }
  this->Widget->GetPVApplication()->GetProcessModule()->SendStream(vtkProcessModule::DATA_SERVER);
}

void vtkPVStringAndScalarListWidgetProperty::SetVTKCommands(
  int numCmds, const char * const*cmds, int *numStrings, int *numScalars)
{
  int i;
  int oldNumCmds = this->NumberOfCommands;
  
  this->Superclass::SetVTKCommands(numCmds, cmds, numScalars);
  
  if (numCmds > oldNumCmds)
    {
    if (this->NumberOfStringsPerCommand)
      {
      delete [] this->NumberOfStringsPerCommand;
      }
    this->NumberOfStringsPerCommand = new int[numCmds];
    }
  
  for (i = 0; i < numCmds; i++)
    {
    this->NumberOfStringsPerCommand[i] = numStrings[i];
    }
}

void vtkPVStringAndScalarListWidgetProperty::SetStrings(int num,
                                                        const char * const*strings)
{
  this->Strings->RemoveAllItems();
  int i;
  
  for (i = 0; i < num; i++)
    {
    this->AddString(strings[i]);
    }
}

void vtkPVStringAndScalarListWidgetProperty::AddString(const char *string)
{
  this->Strings->AddString(string);
}

void vtkPVStringAndScalarListWidgetProperty::SetString(int idx, const char *string)
{
  this->Strings->SetString(idx, string);
}

const char* vtkPVStringAndScalarListWidgetProperty::GetString(int idx)
{
  return this->Strings->GetString(idx);
}

int vtkPVStringAndScalarListWidgetProperty::GetNumberOfStrings()
{
  return this->Strings->GetNumberOfStrings();
}

void vtkPVStringAndScalarListWidgetProperty::SetAnimationTimeInBatch(
  ofstream *file, float val)
{
  if (this->Widget->GetPVSource())
    {
    vtkPVObjectWidget* ov = vtkPVObjectWidget::SafeDownCast(this->Widget);
    if (ov)
      {
      *file << "if { [[$pvTemp" <<  ov->GetPVSource()->GetVTKSourceID(0) 
            << " GetProperty " << ov->GetVariableName() 
            << "] GetClassName] == \"vtkSMIntVectorProperty\"} {" << endl;
      *file << "  set value [expr round(" << val << ")]" << endl;
      *file << "} else {" << endl;
      *file << "  set value " << val << endl;
      *file << "}" << endl;
      *file << "[$pvTemp" << ov->GetPVSource()->GetVTKSourceID(0)
            << " GetProperty " << ov->GetVariableName()
            << "] SetElement 1 $value"<< endl;
      *file << "$pvTemp" << ov->GetPVSource()->GetVTKSourceID(0)
            << " UpdateVTKObjects" << endl;
      }
    }
}

void vtkPVStringAndScalarListWidgetProperty::PrintSelf(ostream &os,
                                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
