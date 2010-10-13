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
#include "vtkCellType.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkKdTreeGenerator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/set>
#include <vtkstd/vector>

class vtkKdTreeManager::vtkAlgorithmSet : 
  public vtkstd::set<vtkSmartPointer<vtkAlgorithm> > {};

vtkStandardNewMacro(vtkKdTreeManager);
vtkCxxSetObjectMacro(vtkKdTreeManager, StructuredProducer, vtkAlgorithm);
//----------------------------------------------------------------------------
vtkKdTreeManager::vtkKdTreeManager()
{
  vtkMultiProcessController* globalController =
    vtkMultiProcessController::GetGlobalController();

  this->Producers = new vtkAlgorithmSet();
  this->StructuredProducer = 0;
  this->KdTree = 0;
  this->NumberOfPieces = globalController->GetNumberOfProcesses();
  this->KdTreeInitialized = false;

  vtkPKdTree* tree = vtkPKdTree::New();
  tree->SetController(globalController);
  tree->SetMinCells(0);
  tree->SetNumberOfRegionsOrMore(this->NumberOfPieces);
  this->SetKdTree(tree);
  tree->FastDelete();
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
void vtkKdTreeManager::SetKdTree(vtkPKdTree* tree)
{
  if (this->KdTree != tree)
    {
    vtkSetObjectBodyMacro(KdTree, vtkPKdTree, tree);
    this->KdTreeInitialized = false;
    }
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::Update()
{
  vtkAlgorithmSet::iterator iter;
  vtkstd::vector<vtkDataObject*> outputs;
  vtkstd::vector<vtkDataObject*>::iterator dsIter;
  
  bool update_required =  (this->GetMTime() > this->UpdateTime);

  // Update all inputs.
  for (iter = this->Producers->begin(); iter != this->Producers->end(); ++iter)
    {
    vtkDataObject *output = iter->GetPointer()->GetOutputDataObject(0);
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
  if (!this->KdTreeInitialized)
    {
    // HACK: This hack fixes the following issue:
    // * create wavelet (num procs >= 4)
    // * volume render -- broken!!!
    // * change some wavelet parameter (force the KdTree to rebuild) and all's
    //   fine!  
    // Seems like something doesn't get initialized correctly, I have no idea
    // what. This seems to overcome the issue.
    vtkSphereSource* sphere = vtkSphereSource::New();
    sphere->Update();
    this->KdTree->AddDataSet(sphere->GetOutput());
    sphere->Delete();
    this->KdTree->BuildLocator();
    this->KdTree->RemoveAllDataSets();
    this->KdTreeInitialized = true;
    }
  for (dsIter = outputs.begin(); dsIter != outputs.end(); ++dsIter)
    {
    this->AddDataObjectToKdTree(*dsIter);
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
    // this is needed to clear the region assignments provided by the structured
    // dataset.
    this->KdTree->AssignRegionsContiguous();
    }

  this->KdTree->BuildLocator();
  //this->KdTree->PrintTree();
  this->UpdateTime.Modified();
  this->KdTree->PrintTree();
}

//-----------------------------------------------------------------------------
void vtkKdTreeManager::AddDataObjectToKdTree(vtkDataObject* data)
{

  vtkCompositeDataSet* mbs = vtkCompositeDataSet::SafeDownCast(data);
  if (!mbs)
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(data);
    this->AddDataSetToKdTree(ds);
    return;
    }

  // for vtkPKdTree to work correctly, we need ensure that the number of
  // inputs on all processes match up. To ensure that for composite datasets,
  // we should add each leaf (NULL or not). However vtkPVGeometryFilter ensures
  // that the non-null leafs match up on all processes. Hence we can avoid the
  // extra headache.

  vtkCompositeDataIterator* iter = mbs->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      this->AddDataSetToKdTree(ds);
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkKdTreeManager::AddDataSetToKdTree(vtkDataSet *data)
{
  // vtkPKdTree is picky about its inputs.  It expects the same amount of
  // inputs on each process and for each input to have at least one cell in
  // it.  It's that second limitation that is really annoying.

  vtkMultiProcessController *controller = this->KdTree->GetController();
  vtkIdType numLocalCells = data->GetNumberOfCells();

  // First, check to see if all process have cells in the data (common
  // condition).
  vtkIdType minLocalCells;
  controller->AllReduce(&numLocalCells, &minLocalCells, 1,
                        vtkCommunicator::MIN_OP);
  if (minLocalCells > 0)
    {
    // Everyone has data.  You can safely just add the data.
    this->KdTree->AddDataSet(data);
    return;
    }

  // Next, check to see if no process has any data.
  vtkIdType maxLocalCells;
  controller->AllReduce(&numLocalCells, &maxLocalCells, 1,
                        vtkCommunicator::MAX_OP);
  if (maxLocalCells <= 0)
    {
    // No one has data.  Just skip this data set.
    return;
    }

  // If some processes have cells and others don't, find a process with cells
  // and have it broadcast a point coordinate to those that do not.  The
  // processes without cells will create a fake data set with a single cell with
  // that single point.  That will skew the balance of the tree slightly, but
  // not enough for us to care.  The method I use to do this is not the most
  // efficient, but the amount of data we are dealing with is so small that the
  // time should be dwarfed by the time to do the actual decomposition later
  // (which I am assuming is what will happen next).
  int rank = controller->GetLocalProcessId();

  // Find a process with some data in it.
  int srcDataProc = 0;
  while (1)
    {
    vtkIdType numSrcCells;
    if (rank == srcDataProc) numSrcCells = numLocalCells;
    controller->Broadcast(&numSrcCells, 1, srcDataProc);
    if (numSrcCells > 0) break;
    srcDataProc++;
    }

  // Broadcast the coordinates of the first point in the source process.
  double pointCoords[3];
  if (rank == srcDataProc) data->GetPoint(0, pointCoords);
  controller->Broadcast(pointCoords, 3, srcDataProc);

  // If I already have data, just give it to the KdTree.  Otherwise, create
  // a "fake" data set with a placeholder cell.
  if (numLocalCells > 0)
    {
    this->KdTree->AddDataSet(data);
    }
  else
    {
    // I don't think vtkPKdTree pays any attention to the type of the data
    // sets.  Thus, it should be safe to create an unstructured grid even
    // if the original data set is not.
    VTK_CREATE(vtkPoints, dummyPoints);
    dummyPoints->SetDataTypeToDouble();
    dummyPoints->InsertNextPoint(pointCoords);
    VTK_CREATE(vtkUnstructuredGrid, dummyData);
    dummyData->SetPoints(dummyPoints);
    vtkIdType ptId = 0;
    dummyData->InsertNextCell(VTK_VERTEX, 1, &ptId);
    this->KdTree->AddDataSet(dummyData);
    }
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KdTree: " << this->KdTree << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
}


