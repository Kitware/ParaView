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

#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkExtentTranslator.h"
#include "vtkKdTreeGenerator.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPKdTree.h"
#include "vtkPoints.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

#include <set>
#include <vector>

class vtkKdTreeManager::vtkDataObjectSet : public std::set<vtkSmartPointer<vtkDataObject> >
{
};

vtkStandardNewMacro(vtkKdTreeManager);
//----------------------------------------------------------------------------
vtkKdTreeManager::vtkKdTreeManager()
{
  vtkMultiProcessController* globalController = vtkMultiProcessController::GetGlobalController();
  if (!globalController)
  {
    vtkWarningMacro("No global controller");
  }
  this->DataObjects = new vtkDataObjectSet();
  this->KdTree = 0;
  this->NumberOfPieces = globalController ? globalController->GetNumberOfProcesses() : 1;
  this->KdTreeInitialized = false;

  vtkPKdTree* tree = vtkPKdTree::New();
  tree->SetController(globalController);
  tree->SetMinCells(0);
  tree->SetNumberOfRegionsOrMore(this->NumberOfPieces);
  this->SetKdTree(tree);
  tree->FastDelete();

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 1;
}

//----------------------------------------------------------------------------
vtkKdTreeManager::~vtkKdTreeManager()
{
  this->SetKdTree(0);

  delete this->DataObjects;
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::AddDataObject(vtkDataObject* dataObject)
{
  this->DataObjects->insert(dataObject);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::RemoveAllDataObjects()
{
  this->DataObjects->clear();
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
void vtkKdTreeManager::SetStructuredDataInformation(vtkExtentTranslator* translator,
  const int whole_extent[6], const double origin[3], const double spacing[3])
{
  this->ExtentTranslator = translator;
  this->SetWholeExtent(const_cast<int*>(whole_extent));
  this->SetOrigin(const_cast<double*>(origin));
  this->SetSpacing(const_cast<double*>(spacing));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::GenerateKdTree()
{
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
    vtkNew<vtkSphereSource> sphere;
    sphere->Update();
    this->KdTree->AddDataSet(sphere->GetOutput());
    this->KdTree->BuildLocator();
    this->KdTree->RemoveAllDataSets();
    this->KdTreeInitialized = true;
  }

  for (vtkDataObjectSet::iterator iter = this->DataObjects->begin();
       iter != this->DataObjects->end(); ++iter)
  {
    this->AddDataObjectToKdTree(iter->GetPointer());
  }

  if (this->ExtentTranslator)
  {
    // vtkPKdTree needs a dataset to compute volume bounds correctly. Create an
    // outline source with the right bounds so that vtkPKdTree is happy.
    vtkNew<vtkOutlineSource> outline;
    vtkBoundingBox bbox;
    bbox.AddPoint(this->Origin);
    bbox.AddPoint(
      this->Origin[0] + (this->WholeExtent[1] - this->WholeExtent[0] + 1) * this->Spacing[0],
      this->Origin[1] + (this->WholeExtent[3] - this->WholeExtent[2] + 1) * this->Spacing[1],
      this->Origin[2] + (this->WholeExtent[5] - this->WholeExtent[4] + 1) * this->Spacing[2]);
    double bounds[6];
    bbox.GetBounds(bounds);
    outline->SetBounds(bounds);
    outline->Update();
    this->AddDataSetToKdTree(outline->GetOutput());

    // Ask the vtkKdTreeGenerator to generate the cuts for the kd tree.
    vtkKdTreeGenerator* generator = vtkKdTreeGenerator::New();
    generator->SetKdTree(this->KdTree);
    generator->SetNumberOfPieces(this->NumberOfPieces);
    generator->BuildTree(this->ExtentTranslator, this->WholeExtent, this->Origin, this->Spacing);
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
  // this->KdTree->PrintTree();
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
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
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
void vtkKdTreeManager::AddDataSetToKdTree(vtkDataSet* data)
{
  // vtkPKdTree is picky about its inputs.  It expects the same amount of
  // inputs on each process and for each input to have at least one cell in
  // it.  It's that second limitation that is really annoying.

  vtkMultiProcessController* controller = this->KdTree->GetController();
  vtkIdType numLocalCells = data->GetNumberOfCells();

  // First, check to see if all process have cells in the data (common
  // condition).
  vtkIdType minLocalCells;
  controller->AllReduce(&numLocalCells, &minLocalCells, 1, vtkCommunicator::MIN_OP);
  if (minLocalCells > 0)
  {
    // Everyone has data.  You can safely just add the data.
    this->KdTree->AddDataSet(data);
    return;
  }

  // Next, check to see if no process has any data.
  vtkIdType maxLocalCells;
  controller->AllReduce(&numLocalCells, &maxLocalCells, 1, vtkCommunicator::MAX_OP);
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
    if (rank == srcDataProc)
      numSrcCells = numLocalCells;
    controller->Broadcast(&numSrcCells, 1, srcDataProc);
    if (numSrcCells > 0)
      break;
    srcDataProc++;
  }

  // Broadcast the coordinates of the first point in the source process.
  double pointCoords[3];
  if (rank == srcDataProc)
    data->GetPoint(0, pointCoords);
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
    vtkNew<vtkPoints> dummyPoints;
    dummyPoints->SetDataTypeToDouble();
    dummyPoints->InsertNextPoint(pointCoords);
    vtkNew<vtkUnstructuredGrid> dummyData;
    dummyData->SetPoints(dummyPoints.GetPointer());
    vtkIdType ptId = 0;
    dummyData->InsertNextCell(VTK_VERTEX, 1, &ptId);
    this->KdTree->AddDataSet(dummyData.GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkKdTreeManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KdTree: " << this->KdTree << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
}
