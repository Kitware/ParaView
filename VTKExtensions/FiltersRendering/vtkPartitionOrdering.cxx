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
#include "vtkDataSet.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <map>

vtkStandardNewMacro(vtkPartitionOrdering);
vtkCxxSetObjectMacro(vtkPartitionOrdering, Controller, vtkMultiProcessController);

namespace
{
double Distance2ToBoundingBox(const double* point, const double* bbox)
{
  double retVal = 0;
  for (int i = 0; i < 3; i++)
  {
    if (point[i] < bbox[i * 2])
    {
      retVal += (point[i] - bbox[i * 2]) * (point[i] - bbox[i * 2]);
    }
    else if (point[i] > bbox[i * 2 + 1])
    {
      retVal += (point[i] - bbox[i * 2 + 1]) * (point[i] - bbox[i * 2 + 1]);
    }
  }
  return retVal;
}

void DistanceVectorToBoundingBox(const double* point, const double* bbox, double* result)
{
  for (int i = 0; i < 3; i++)
  {
    if (point[i] < bbox[i * 2])
    {
      result[i] = bbox[i * 2] - point[i];
    }
    else if (point[i] > bbox[i * 2 + 1])
    {
      result[i] = point[i] - bbox[i * 2 + 1];
    }
    else
    {
      result[i] = 0;
    }
  }
}
}

vtkPartitionOrdering::vtkPartitionOrdering()
{
  this->Controller = NULL;
}
vtkPartitionOrdering::~vtkPartitionOrdering()
{
  this->SetController(NULL);
}

int vtkPartitionOrdering::ViewOrderAllProcessesInDirection(
  const double dop[3], vtkIntArray* orderedList)
{
  assert("pre: orderedList_exists" && orderedList != 0);

  // find the first corner of the bounding box that we hit going in the direction
  // of dop. We only need a single point if the first point isn't unique since we take
  // a projection later on in this direction.
  double pos[3] = { this->GlobalBounds[0], this->GlobalBounds[2], this->GlobalBounds[4] };
  for (int i = 0; i < 3; i++)
  {
    if (dop[i] < 0)
    {
      pos[i] = this->GlobalBounds[i * 2 + 1];
    }
  }

  // map from projected distance from position to closest point in a process's
  // bounding box to the process's id. a vector is used in case
  // multiple bounding boxes have the same distance to the point.
  std::map<double, std::vector<size_t> > processDistances;
  double distanceVector[3];
  for (size_t i = 0; i < this->ProcessBounds.size() / 6; i++)
  {
    DistanceVectorToBoundingBox(pos, &this->ProcessBounds[i * 6], distanceVector);
    double dist = std::abs(dop[0] * distanceVector[0]) + std::abs(dop[1] * distanceVector[1]) +
      std::abs(dop[2] * distanceVector[2]);
    processDistances[dist].push_back(i);
  }
  int numberOfValues = static_cast<int>(this->ProcessBounds.size() / 6);
  orderedList->SetNumberOfValues(numberOfValues);
  int counter = 0;
  for (std::map<double, std::vector<size_t> >::iterator it = processDistances.begin();
       it != processDistances.end(); it++)
  {
    for (std::vector<size_t>::iterator vit = it->second.begin(); vit != it->second.end(); vit++)
    {
      orderedList->SetValue(counter, *vit);
      counter++;
    }
  }

  return static_cast<int>(this->ProcessBounds.size() / 6);
}

int vtkPartitionOrdering::ViewOrderAllProcessesFromPosition(
  const double pos[3], vtkIntArray* orderedList)
{
  assert("pre: orderedList_exists" && orderedList != 0);

  // map from distance from position to closest point in a process's
  // bounding box to the process's id. a vector is used in case
  // multiple bounding boxes have the same distance to the point.
  std::map<double, std::vector<size_t> > processDistances;
  for (size_t i = 0; i < this->ProcessBounds.size() / 6; i++)
  {
    double dist2 = Distance2ToBoundingBox(pos, &this->ProcessBounds[i * 6]);
    processDistances[dist2].push_back(i);
  }
  int numberOfValues = static_cast<int>(this->ProcessBounds.size() / 6);
  orderedList->SetNumberOfValues(numberOfValues);
  int counter = 0;
  for (std::map<double, std::vector<size_t> >::iterator it = processDistances.begin();
       it != processDistances.end(); it++)
  {
    for (std::vector<size_t>::iterator vit = it->second.begin(); vit != it->second.end(); vit++)
    {
      orderedList->SetValue(counter, *vit);
      counter++;
    }
  }

  return numberOfValues;
}

bool vtkPartitionOrdering::Construct(vtkDataSet* grid)
{
  this->Modified();
  if (!grid)
  {
    vtkErrorMacro("No grid provided to Construct()");
    this->ProcessBounds.clear();
    return false;
  }

  vtkMultiProcessController* controller = this->GetController();
  if (!controller)
  {
    controller = vtkMultiProcessController::GetGlobalController();
  }
  double localBounds[6];
  grid->GetBounds(localBounds);
  return this->Construct(localBounds);
}

bool vtkPartitionOrdering::Construct(const double localBounds[6])
{
  vtkMultiProcessController* controller = this->GetController();
  if (!controller)
  {
    controller = vtkMultiProcessController::GetGlobalController();
  }

  this->ProcessBounds.resize(6 * controller->GetNumberOfProcesses());
  controller->AllGather(localBounds, &this->ProcessBounds[0], 6);

  for (int i = 0; i < 6; i++)
  {
    this->GlobalBounds[i] = this->ProcessBounds[i];
  }
  for (size_t i = 1; i < this->ProcessBounds.size() / 6; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (this->GlobalBounds[j * 2] > this->ProcessBounds[i * 6 + j * 2])
      {
        this->GlobalBounds[j * 2] = this->ProcessBounds[i * 6 + j * 2];
      }
      if (this->GlobalBounds[j * 2 + 1] < this->ProcessBounds[i * 6 + j * 2 + 1])
      {
        this->GlobalBounds[j * 2 + 1] = this->ProcessBounds[i * 6 + j * 2 + 1];
      }
    }
  }

  return true;
}

int vtkPartitionOrdering::GetNumberOfRegions()
{
  return static_cast<int>(this->ProcessBounds.size() / 6);
}

void vtkPartitionOrdering::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (NULL)" << endl;
  }
}
