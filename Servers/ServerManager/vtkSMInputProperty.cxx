/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMInputProperty);
vtkCxxRevisionMacro(vtkSMInputProperty, "1.1");

struct vtkSMInputPropertyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMSourceProxy> > Inputs;
};

//---------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
{
  this->ImmediateUpdate = 1;
  this->UpdateSelf = 1;
  this->MultipleInput = 0;
  this->CleanCommand = 0;

  this->IPInternals = new vtkSMInputPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty()
{
  delete this->IPInternals;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddInput(vtkSMSourceProxy* input, int modify)
{
  this->IPInternals->Inputs.push_back(input);
  if (modify)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddInput(vtkSMSourceProxy* input)
{
  this->AddInput(input, 1);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllInputs()
{
  this->IPInternals->Inputs.clear();
  this->Modified();
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMInputProperty::GetInput(unsigned int idx)
{
  return this->IPInternals->Inputs[idx];
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetNumberOfInputs()
{
  return this->IPInternals->Inputs.size();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AppendCommandToStream(
    vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command)
    {
    return;
    }

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke 
         << objectId << "CleanInputs" << this->CleanCommand
         << vtkClientServerStream::End;
    }
  unsigned int numInputs = this->GetNumberOfInputs();
  for (unsigned int i=0; i<numInputs; i++)
    {
    *str << vtkClientServerStream::Invoke 
         << objectId 
         << "AddInput" 
         << this->IPInternals->Inputs[i] 
         << this->Command;
    if (this->MultipleInput)
      {
      *str << 1;
      }
    else
      {
      *str << 0;
      }
    *str << vtkClientServerStream::End;
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SaveState(
  const char* name,  ofstream* file, vtkIndent indent)
{
  vtkSMProxyManager* pm = this->GetProxyManager();
  if (!pm)
    {
    return;
    }
  
  unsigned int numInputs = this->GetNumberOfInputs();
  for (unsigned int idx=0; idx<numInputs; idx++)
    {
    for (unsigned int i=0; i<this->GetNumberOfDomains(); i++)
      {
      vtkSMProxyGroupDomain* dom = vtkSMProxyGroupDomain::SafeDownCast(
        this->GetDomain(i));
      if (dom)
        {
        unsigned int numGroups = dom->GetNumberOfGroups();
        for (unsigned int j=0; j<numGroups; j++)
          {
          const char* proxyname = pm->IsProxyInGroup(
            this->GetInput(idx), dom->GetGroup(j));
          if (proxyname)
            {
            *file << indent 
                  << name 
                  << " : " <<  proxyname
                  << " : " << dom->GetGroup(j)
                  << endl;
            }
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::ReadXMLAttributes(vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(element);

  int multiple_input;
  int retVal = element->GetScalarAttribute("multiple_input", &multiple_input);
  if(retVal) 
    { 
    this->SetMultipleInput(multiple_input); 
    }

  const char* clean_command = element->GetAttribute("clean_command");
  if(clean_command) 
    { 
    this->SetCleanCommand(clean_command); 
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent 
     << "CleanCommand: "
     << (this->CleanCommand ? this->CleanCommand : "(none)") 
     << endl;
  os << indent << "MultipleInput: " << this->MultipleInput << endl;
}
