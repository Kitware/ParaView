/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarListWidgetProperty.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVScalarListWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"

vtkStandardNewMacro(vtkPVScalarListWidgetProperty);
vtkCxxRevisionMacro(vtkPVScalarListWidgetProperty, "1.5");

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

void vtkPVScalarListWidgetProperty::SetVTKCommands(int numCmds, char **cmd,
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
