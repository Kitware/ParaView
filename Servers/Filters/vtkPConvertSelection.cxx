/*=========================================================================

  Program:   ParaView
  Module:    vtkPConvertSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPConvertSelection.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPConvertSelection);
vtkCxxRevisionMacro(vtkPConvertSelection, "1.1");
vtkCxxSetObjectMacro(vtkPConvertSelection, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPConvertSelection::vtkPConvertSelection()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPConvertSelection::~vtkPConvertSelection()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
// returns if the input has PROCESS_ID == processId
static bool vtkTrimTree(vtkSelection* input, int processId)
{
  if (input)
    {
    if (input->GetContentType() == vtkSelection::SELECTIONS)
      {
      // trim children.
      int numChildren = static_cast<int>(input->GetNumberOfChildren());
      for (int cc=numChildren-1; cc >= 0; --cc)
        {
        bool match = ::vtkTrimTree(input->GetChild(cc), processId);
        if (!match)
          {
          input->RemoveChild(cc);
          }
        }
      }

    int propId = (input->GetProperties()->Has(vtkSelection::PROCESS_ID()))?
      input->GetProperties()->Get(vtkSelection::PROCESS_ID()): -1;
      
    if (propId == -1 || processId == -1 || propId == processId) 
      {
      return true;
      }
    }

  return false;
}

//----------------------------------------------------------------------------
// Adds the process id to all leaf nodes.
static void vtkAddProcessID(vtkSelection* input, int myId)
{
  if (input)
    {
    if (input->GetContentType() == vtkSelection::SELECTIONS)
      {
      // trim children.
      unsigned int numChildren = input->GetNumberOfChildren();
      for (unsigned int cc=0; cc < numChildren; cc++)
        {
        ::vtkAddProcessID(input->GetChild(cc), myId);
        }
      }
    else
      {
      input->GetProperties()->Set(vtkSelection::PROCESS_ID(), myId);
      }
    }
}

//----------------------------------------------------------------------------
int vtkPConvertSelection::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses()==1)
    {
    // nothing much to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkSelection* input = vtkSelection::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* dataInfo = inputVector[1]->GetInformationObject(0);
  vtkDataObject* data = dataInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkSelection* output = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Now we need to remote components from the input that don't belong to this
  // process.
  int myId = this->Controller->GetLocalProcessId();

  vtkSmartPointer<vtkSelection> newInput = 
    vtkSmartPointer<vtkSelection>::New();
  newInput->ShallowCopy(input);

  if (!::vtkTrimTree(newInput, myId))
    {
    // empty output?
    return 1;
    }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(data);
  if (ds && ds->GetNumberOfPoints() > 0)
    {
    // This is needed since vtkConvertSelection simply shallow copies input to
    // output and raises errors when "data" is empty.
    if (!this->Convert(newInput, data, output))
      {
      return 0;
      }
    }

  // Now add process id to the generated output.
  ::vtkAddProcessID(output, myId); 
  return 1;
}


//----------------------------------------------------------------------------
void vtkPConvertSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}


