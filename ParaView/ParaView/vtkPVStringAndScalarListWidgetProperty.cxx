/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVStringAndScalarListWidgetProperty.cxx
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
#include "vtkPVStringAndScalarListWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"
#include "vtkStringList.h"

vtkStandardNewMacro(vtkPVStringAndScalarListWidgetProperty);
vtkCxxRevisionMacro(vtkPVStringAndScalarListWidgetProperty, "1.1.2.3");

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
           << this->VTKSourceID << " " << this->VTKCommands[i];
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
  this->Widget->GetPVApplication()->GetProcessModule()->SendStreamToServer();
}

void vtkPVStringAndScalarListWidgetProperty::SetVTKCommands(
  int numCmds, char **cmds, int *numStrings, int *numScalars)
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
                                                        char **strings)
{
  this->Strings->RemoveAllItems();
  int i;
  
  for (i = 0; i < num; i++)
    {
    this->AddString(strings[i]);
    }
}

void vtkPVStringAndScalarListWidgetProperty::AddString(char *string)
{
  this->Strings->AddString(string);
}

void vtkPVStringAndScalarListWidgetProperty::SetString(int idx, char *string)
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

void vtkPVStringAndScalarListWidgetProperty::PrintSelf(ostream &os,
                                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
