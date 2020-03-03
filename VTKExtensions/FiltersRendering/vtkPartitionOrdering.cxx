/*=========================================================================

  Program:   ParaView
  Module:    vtkPartitionOrdering.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionOrdering.h"

#include "vtkBlockSortHelper.h"
#include "vtkDataSet.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkPartitionOrdering);
vtkCxxSetObjectMacro(vtkPartitionOrdering, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPartitionOrdering::vtkPartitionOrdering()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPartitionOrdering::~vtkPartitionOrdering()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkPartitionOrdering::ViewOrderAllProcessesInDirection(
  const double dop[3], vtkIntArray* orderedList)
{
  assert("pre: orderedList_exists" && orderedList != 0);
  vtkBlockSortHelper::BackToFront<BoxT> sortBoxes(
    vtkVector3d(0.0), vtkVector3d(dop), /*is_parallel = */ true);
  vtkBlockSortHelper::Sort(this->Boxes.begin(), this->Boxes.end(), sortBoxes);

  orderedList->SetNumberOfValues(static_cast<int>(this->Boxes.size()));
  int counter = 0;
  for (auto iter = this->Boxes.rbegin(); iter != this->Boxes.rend(); ++iter)
  {
    orderedList->SetValue(counter++, iter->Rank);
  }
  return counter;
}

//------------------------------------------------------------------------------
int vtkPartitionOrdering::ViewOrderAllProcessesFromPosition(
  const double pos[3], vtkIntArray* orderedList)
{
  assert("pre: orderedList_exists" && orderedList != 0);

  vtkBlockSortHelper::BackToFront<BoxT> sortBoxes(
    vtkVector3d(pos), vtkVector3d(0.0), /*is_parallel = */ false);
  vtkBlockSortHelper::Sort(this->Boxes.begin(), this->Boxes.end(), sortBoxes);

  orderedList->SetNumberOfValues(static_cast<int>(this->Boxes.size()));
  int counter = 0;
  for (auto iter = this->Boxes.rbegin(); iter != this->Boxes.rend(); ++iter)
  {
    orderedList->SetValue(counter++, iter->Rank);
  }
  return counter;
}

//------------------------------------------------------------------------------
bool vtkPartitionOrdering::Construct(vtkDataSet* grid)
{
  this->Modified();
  this->GlobalBounds.Reset();
  this->Boxes.clear();
  if (!grid)
  {
    vtkErrorMacro("No grid provided to Construct()");
    return false;
  }

  double localBounds[6];
  grid->GetBounds(localBounds);
  return this->Construct(localBounds);
}

//------------------------------------------------------------------------------
bool vtkPartitionOrdering::Construct(const double localBounds[6])
{
  this->GlobalBounds.Reset();
  this->Boxes.clear();

  vtkMultiProcessController* controller = this->GetController();
  if (controller == nullptr || controller->GetNumberOfProcesses() == 1)
  {
    BoxT item;
    item.BBox.SetBounds(localBounds);
    item.Rank = 0;
    this->Boxes.push_back(item);
    return true;
  }

  const int num_ranks = controller->GetNumberOfProcesses();

  std::vector<vtkVector<double, 6> > all_bounds;
  all_bounds.resize(num_ranks);
  controller->AllGather(localBounds, reinterpret_cast<double*>(all_bounds[0].GetData()), 6);

  for (const auto& bds : all_bounds)
  {
    BoxT item;
    item.BBox.SetBounds(bds.GetData());
    item.Rank = static_cast<int>(this->Boxes.size());
    this->GlobalBounds.AddBox(item.BBox);
    this->Boxes.push_back(item);
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkPartitionOrdering::GetNumberOfRegions()
{
  return static_cast<int>(this->Boxes.size());
}

//------------------------------------------------------------------------------
void vtkPartitionOrdering::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (nullptr)" << endl;
  }
}
