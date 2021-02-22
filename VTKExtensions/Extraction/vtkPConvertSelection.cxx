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

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtractSelection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPConvertSelection);
vtkCxxSetObjectMacro(vtkPConvertSelection, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPConvertSelection::vtkPConvertSelection()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  vtkNew<vtkPVExtractSelection> se;
  this->SetSelectionExtractor(se.GetPointer());
}

//----------------------------------------------------------------------------
vtkPConvertSelection::~vtkPConvertSelection()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
// returns if the input has PROCESS_ID == processId
static void vtkTrimTree(vtkSelection* input, int processId)
{
  if (input)
  {
    unsigned int numNodes = input->GetNumberOfNodes();
    for (unsigned int cc = 0; cc < numNodes; cc++)
    {
      vtkSelectionNode* node = input->GetNode(cc);
      int propId = (node->GetProperties()->Has(vtkSelectionNode::PROCESS_ID()))
        ? node->GetProperties()->Get(vtkSelectionNode::PROCESS_ID())
        : -1;
      if (propId != -1 && processId != -1 && propId != processId)
      {
        input->RemoveNode(node);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Adds the process id to all leaf nodes.
static void vtkAddProcessID(vtkSelection* input, int myId)
{
  if (input)
  {
    unsigned int numNodes = input->GetNumberOfNodes();
    for (unsigned int cc = 0; cc < numNodes; cc++)
    {
      input->GetNode(cc)->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), myId);
    }
  }
}

//----------------------------------------------------------------------------
int vtkPConvertSelection::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() == 1)
  {
    // nothing much to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkSelection* input = vtkSelection::GetData(inInfo);

  // vtkDataObject* data = vtkDataObject::GetData(inputVector[1], 0);

  vtkSelection* output = vtkSelection::GetData(outputVector, 0);

  // Now we need to remote components from the input that don't belong to this
  // process.
  int myId = this->Controller->GetLocalProcessId();

  vtkSmartPointer<vtkSelection> newInput = vtkSmartPointer<vtkSelection>::New();
  newInput->ShallowCopy(input);

  ::vtkTrimTree(newInput, myId);

  // This is needed since vtkConvertSelection simply shallow copies input to
  // output and raises errors when "data" is empty.
  input->Register(this);
  inInfo->Set(vtkDataObject::DATA_OBJECT(), newInput);
  int ret = this->Superclass::RequestData(request, inputVector, outputVector);
  inInfo->Set(vtkDataObject::DATA_OBJECT(), input);
  input->UnRegister(this);
  if (!ret)
  {
    return 0;
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
