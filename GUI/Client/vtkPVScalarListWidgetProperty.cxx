/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScalarListWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVScalarListWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"
#include "vtkClientServerStream.h"

#include "vtkPVObjectWidget.h"
#include "vtkPVSource.h"

vtkStandardNewMacro(vtkPVScalarListWidgetProperty);
vtkCxxRevisionMacro(vtkPVScalarListWidgetProperty, "1.12.2.1");

vtkPVScalarListWidgetProperty::vtkPVScalarListWidgetProperty()
{
  this->VTKCommands = NULL;
  this->NumberOfScalarsPerCommand = NULL;
  this->NumberOfCommands = 0;
  this->Scalars = NULL;
  this->NumberOfScalars = 0;
}

vtkPVScalarListWidgetProperty::~vtkPVScalarListWidgetProperty()
{
  int i;

  this->VTKSourceID.ID = 0;
  for (i = 0; i < this->NumberOfCommands; i++)
    {
    delete [] this->VTKCommands[i];
    }

  delete [] this->VTKCommands;
  this->VTKCommands = NULL;

  delete [] this->NumberOfScalarsPerCommand;
  this->NumberOfScalarsPerCommand = NULL;

  delete [] this->Scalars;
  this->Scalars = NULL;
}

void vtkPVScalarListWidgetProperty::SetVTKCommands(int numCmds, const char * const*cmd,
                                             int *numScalars)
{
  int i;
  
  if (numCmds > this->NumberOfCommands)
    {
    if (this->VTKCommands)
      {
      for (i = 0; i < this->NumberOfCommands; i++)
        {
        delete [] this->VTKCommands[i];
        }
      delete [] this->VTKCommands;
      this->VTKCommands = NULL;
      }
    if (this->NumberOfScalarsPerCommand)
      {
      delete [] this->NumberOfScalarsPerCommand;
      this->NumberOfScalarsPerCommand = NULL;
      }
    this->VTKCommands = new char*[numCmds];
    this->NumberOfScalarsPerCommand = new int[numCmds];
    }
  else
    {
    for (i = 0; i < this->NumberOfCommands; i++)
      {
      delete [] this->VTKCommands[i];
      }
    }
  
  this->NumberOfCommands = numCmds;
  
  for (i = 0; i < numCmds; i++)
    {
    this->VTKCommands[i] = new char[strlen(cmd[i])+1];
    strcpy(this->VTKCommands[i], cmd[i]);
    this->NumberOfScalarsPerCommand[i] = numScalars[i];
    }
}

void vtkPVScalarListWidgetProperty::SetScalars(int num, float *scalars)
{
  if (num > this->NumberOfScalars)
    {
    delete [] this->Scalars;
    this->Scalars = new float[num];
    }
  
  this->NumberOfScalars = num;
  memcpy(this->Scalars, scalars, num*sizeof(float));
}

float vtkPVScalarListWidgetProperty::GetScalar(int idx)
{
  if (idx >= this->NumberOfScalars)
    {
    return 0;
    }
  return this->Scalars[idx];
}

void vtkPVScalarListWidgetProperty::AcceptInternal()
{
  int i, j, count = 0;
  vtkPVProcessModule* pm = this->Widget->GetPVApplication()->GetProcessModule();
  
  for (i = 0; i < this->NumberOfCommands; i++)
    {
    pm->GetStream() 
      << vtkClientServerStream::Invoke << this->VTKSourceID << this->VTKCommands[i];
    for (j = 0; j < this->NumberOfScalarsPerCommand[i]; j++)
      {
      pm->GetStream() << this->Scalars[count];
      count++;
      }
    pm->GetStream() << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}

void vtkPVScalarListWidgetProperty::SetAnimationTime(float time)
{
  this->SetScalars(1, &time);
  this->Widget->ModifiedCallback();
  this->Widget->Reset();
}

void vtkPVScalarListWidgetProperty::SetAnimationTimeInBatch(
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
            << "] SetElement 0 $value"<< endl;
      }
    *file << "$pvTemp" << this->Widget->GetPVSource()->GetVTKSourceID(0)
          << " UpdateVTKObjects" << endl;
    }
}

void vtkPVScalarListWidgetProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfScalars: " << this->NumberOfScalars << endl;
}
