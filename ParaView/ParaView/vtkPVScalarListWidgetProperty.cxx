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

vtkStandardNewMacro(vtkPVScalarListWidgetProperty);
vtkCxxRevisionMacro(vtkPVScalarListWidgetProperty, "1.9");

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

void vtkPVScalarListWidgetProperty::AddScalar(float scalar)
{
  float *scalars = new float[this->NumberOfScalars];
  int i;
  for (i = 0; i < this->NumberOfScalars; i++)
    {
    scalars[i] = this->Scalars[i];
    }

  delete [] this->Scalars;
  this->Scalars = NULL;
  
  this->Scalars = new float[this->NumberOfScalars+1];
  for (i = 0; i < this->NumberOfScalars; i++)
    {
    this->Scalars[i] = scalars[i];
    }
  delete [] scalars;
  this->Scalars[i] = scalar;
  this->NumberOfScalars++;
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
  pm->SendStreamToServer();
}

void vtkPVScalarListWidgetProperty::SetAnimationTime(float time)
{
  this->SetScalars(1, &time);
  this->Widget->ModifiedCallback();
  this->Widget->Reset();
}

void vtkPVScalarListWidgetProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfScalars: " << this->NumberOfScalars << endl;
}
