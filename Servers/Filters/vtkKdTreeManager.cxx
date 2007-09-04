/*=========================================================================

  Program:   ParaView
  Module:    vtkKdTreeManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKdTreeManager.h"

#include "vtkAlgorithm.h"
#include "vtkDataSet.h"
#include "vtkKdTreeGenerator.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>
#include <vtkstd/vector>

class vtkKdTreeManager::vtkAlgorithmSet : 
  public vtkstd::set<vtkSmartPointer<vtkAlgorithm> > {};

vtkStandardNewMacro(vtkKdTreeManager);
vtkCxxRevisionMacro(vtkKdTreeManager, "1.4");
vtkCxxSetObjectMacro(vtkKdTreeManager, StructuredProducer, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkKdTreeManager, KdTree, vtkPKdTree);
//----------------------------------------------------------------------------
vtkKdTreeManager::vtkKdTreeManager()
{
  this->Producers = new vtkAlgorithmSet();
  this->StructuredProducer = 0;
  this->KdTree = 0;
  this->NumberOfPieces = 1;
}

//----------------------------------------------------------------------------
vtkKdTreeManager::~vtkKdTreeManager()
{
  this->SetKdTree(0);
  this->SetStructuredProducer(0);

  delete this->Producers;
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::AddProducer(vtkAlgorithm* producer)
{
  this->Producers->insert(producer);
  if (this->KdTree)
    {
    this->KdTree->RemoveAllDataSets();
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::RemoveProducer(vtkAlgorithm* producer)
{
  vtkAlgorithmSet::iterator iter = this->Producers->find(producer);
  if (iter != this->Producers->end())
    {
    if (this->KdTree)
      {
      this->KdTree->RemoveAllDataSets();
      }
    this->Producers->erase(iter);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::RemoveAllProducers()
{
  if (this->KdTree)
    {
    this->KdTree->RemoveAllDataSets();
    }
  this->Producers->clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::Update()
{
  vtkAlgorithmSet::iterator iter;
  vtkstd::vector<vtkDataSet*> outputs;
  vtkstd::vector<vtkDataSet*>::iterator dsIter;
  
  bool update_required =  (this->GetMTime() > this->UpdateTime);

  // Update all inputs.
  for (iter = this->Producers->begin(); iter != this->Producers->end(); ++iter)
    {
    vtkDataSet*output = vtkDataSet::SafeDownCast(
      iter->GetPointer()->GetOutputDataObject(0));
    if (output)
      {
      outputs.push_back(output);
      update_required |= (output->GetMTime() > this->UpdateTime);
      }
    }

  if (this->StructuredProducer)
    {
    vtkDataSet* output = vtkDataSet::SafeDownCast(
      this->StructuredProducer->GetOutputDataObject(0));
    if (output)
      {
      outputs.push_back(output);
      update_required |= (output->GetMTime() > this->UpdateTime);
      }
    }

  if (!update_required)
    {
    return;
    }

  this->KdTree->RemoveAllDataSets();
  for (dsIter = outputs.begin(); dsIter != outputs.end(); ++dsIter)
    {
    if ((*dsIter)->GetNumberOfPoints() > 0)
      {
      this->KdTree->AddDataSet(*dsIter);
      }
    }

  if (this->StructuredProducer)
    {
    // Ask the vtkKdTreeGenerator to generate the cuts for the kd tree.
    vtkKdTreeGenerator* generator = vtkKdTreeGenerator::New();
    generator->SetKdTree(this->KdTree);
    generator->SetNumberOfPieces(this->NumberOfPieces);
    generator->BuildTree(this->StructuredProducer->GetOutputDataObject(0));
    generator->Delete();
    }
  else
    {
    // Ensure that the kdtree is not using predefined cuts.
    this->KdTree->SetCuts(0);
    }

  this->KdTree->BuildLocator();
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KdTree: " << this->KdTree << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
}


