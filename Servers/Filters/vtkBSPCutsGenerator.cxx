/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBSPCutsGenerator.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBSPCuts.cxx"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTreeManager.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkMultiProcessController.h"

vtkStandardNewMacro(vtkBSPCutsGenerator);
//----------------------------------------------------------------------------
vtkBSPCutsGenerator::vtkBSPCutsGenerator()
{
  this->Enabled = true;
  this->PKdTree = 0;
}

//----------------------------------------------------------------------------
vtkBSPCutsGenerator::~vtkBSPCutsGenerator()
{
  this->SetPKdTree(0);
}

//----------------------------------------------------------------------------
int vtkBSPCutsGenerator::RequestDataObject(
  vtkInformation*, vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  vtkBSPCuts* output = vtkBSPCuts::GetData(outputVector, 0);
  if (!output)
    {
    output = vtkBSPCuts::New();
    output->SetPipelineInformation(outputVector->GetInformationObject(0));
    outputVector->GetInformationObject(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(),
      output->GetExtentType());
    output->FastDelete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkBSPCutsGenerator::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }

  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkBSPCutsGenerator::RequestData(vtkInformation *,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  if (this->Enabled && controller && controller->GetNumberOfProcesses() > 1)
    {
    vtkKdTreeManager* mgr = vtkKdTreeManager::New();
    vtkBSPCuts* output = vtkBSPCuts::GetData(outputVector, 0);
    for (int cc=0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
      {
      vtkDataObject* input = vtkDataObject::GetData(inputVector[0], cc);
      //vtkDataObject* clone = input->NewInstance();
      //clone->ShallowCopy(input);
      if (input->GetExtentType() == VTK_3D_EXTENT)
        {
        mgr->SetStructuredProducer(input->GetProducerPort()->GetProducer());
        }
      else
        {
        mgr->AddProducer(input->GetProducerPort()->GetProducer());
        }
      //clone->FastDelete();
      }

    mgr->Update();
    output->ShallowCopy(mgr->GetKdTree()->GetCuts());

    this->SetPKdTree(mgr->GetKdTree());

    mgr->RemoveAllProducers();
    mgr->SetStructuredProducer(NULL);
    mgr->Delete();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkBSPCutsGenerator::SetPKdTree(vtkPKdTree* tree)
{
  if (this->PKdTree)
    {
    this->PKdTree->UnRegister(this);
    }
  this->PKdTree = tree;
  if (tree)
    {
    tree->Register(this);
    }
}

//----------------------------------------------------------------------------
void vtkBSPCutsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

