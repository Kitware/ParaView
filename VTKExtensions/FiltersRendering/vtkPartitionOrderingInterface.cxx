/*=========================================================================

  Program:   ParaView
  Module:    vtkPartitionOrderingInterface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionOrderingInterface.h"

#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPartitionOrdering.h"

#include <map>

vtkStandardNewMacro(vtkPartitionOrderingInterface);

vtkPartitionOrderingInterface::vtkPartitionOrderingInterface()
{
}

vtkPartitionOrderingInterface::~vtkPartitionOrderingInterface()
{
}

int vtkPartitionOrderingInterface::GetNumberOfRegions()
{
  if (!this->Implementation)
  {
    vtkErrorMacro("Must set Implementation");
    return 0;
  }

  if (vtkPKdTree* tree = vtkPKdTree::SafeDownCast(this->Implementation))
  {
    return tree->GetNumberOfRegions();
  }
  else if (vtkPartitionOrdering* ordering =
             vtkPartitionOrdering::SafeDownCast(this->Implementation))
  {
    return ordering->GetNumberOfRegions();
  }
  return 0;
}
int vtkPartitionOrderingInterface::ViewOrderAllProcessesInDirection(
  const double dop[3], vtkIntArray* orderedList)
{
  if (!this->Implementation)
  {
    vtkErrorMacro("Must set Implementation");
    return 0;
  }

  if (vtkPKdTree* tree = vtkPKdTree::SafeDownCast(this->Implementation))
  {
    return tree->ViewOrderAllProcessesInDirection(dop, orderedList);
  }
  else if (vtkPartitionOrdering* ordering =
             vtkPartitionOrdering::SafeDownCast(this->Implementation))
  {
    return ordering->ViewOrderAllProcessesInDirection(dop, orderedList);
  }
  return 0;
}

int vtkPartitionOrderingInterface::ViewOrderAllProcessesFromPosition(
  const double pos[3], vtkIntArray* orderedList)
{
  if (!this->Implementation)
  {
    vtkErrorMacro("Must set Implementation");
    return 0;
  }

  if (vtkPKdTree* tree = vtkPKdTree::SafeDownCast(this->Implementation))
  {
    return tree->ViewOrderAllProcessesFromPosition(pos, orderedList);
  }
  else if (vtkPartitionOrdering* ordering =
             vtkPartitionOrdering::SafeDownCast(this->Implementation))
  {
    return ordering->ViewOrderAllProcessesFromPosition(pos, orderedList);
  }
  return 0;
}

void vtkPartitionOrderingInterface::SetImplementation(vtkObject* implementation)
{
  if (implementation == NULL)
  {
    this->Implementation = NULL;
    return;
  }
  if (implementation != this->Implementation)
  {
    if (implementation->IsA("vtkPKdTree") || implementation->IsA("vtkPartitionOrdering"))
    {
      this->Implementation = implementation;
      return;
    }
    vtkErrorMacro("Implementation must be a vtkPKdTree or a vtkPartitionOrdering but is a "
      << implementation->GetClassName());
  }
}

vtkMTimeType vtkPartitionOrderingInterface::GetMTime()
{
  if (!this->Implementation)
  {
    vtkErrorMacro("Must set Implementation");
    return 0;
  }

  if (vtkPKdTree* tree = vtkPKdTree::SafeDownCast(this->Implementation))
  {
    return tree->GetMTime();
  }
  else if (vtkPartitionOrdering* ordering =
             vtkPartitionOrdering::SafeDownCast(this->Implementation))
  {
    return ordering->GetMTime();
  }
  return 0;
}

void vtkPartitionOrderingInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Implementation: " << this->Implementation << endl;
}
