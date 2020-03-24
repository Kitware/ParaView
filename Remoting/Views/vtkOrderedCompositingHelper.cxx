/*=========================================================================

  Program:   ParaView
  Module:    vtkOrderedCompositingHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOrderedCompositingHelper.h"

#include "vtkBlockSortHelper.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"

namespace
{
struct BoxT
{
  vtkOrderedCompositingHelper* self = nullptr;
  int rank = -1;
  void GetBounds(double bds[6]) const { this->self->GetBoundingBoxes()[this->rank].GetBounds(bds); }
};
}

vtkStandardNewMacro(vtkOrderedCompositingHelper);
//----------------------------------------------------------------------------
vtkOrderedCompositingHelper::vtkOrderedCompositingHelper()
{
}

//----------------------------------------------------------------------------
vtkOrderedCompositingHelper::~vtkOrderedCompositingHelper()
{
}

//----------------------------------------------------------------------------
void vtkOrderedCompositingHelper::SetBoundingBoxes(const std::vector<vtkBoundingBox>& boxes)
{
  if (this->Boxes != boxes)
  {
    this->Boxes = boxes;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const vtkBoundingBox& vtkOrderedCompositingHelper::GetBoundingBox(int index) const
{
  if (index >= 0 && index < static_cast<int>(this->Boxes.size()))
  {
    return this->Boxes[index];
  }

  return this->InvalidBox;
}

//------------------------------------------------------------------------------
std::vector<int> vtkOrderedCompositingHelper::ComputeSortOrder(vtkCamera* camera)
{
  assert(camera != nullptr);
  return camera->GetParallelProjection()
    ? this->ComputeSortOrderInViewDirection(camera->GetDirectionOfProjection())
    : this->ComputeSortOrderFromPosition(camera->GetPosition());
}

//------------------------------------------------------------------------------
std::vector<int> vtkOrderedCompositingHelper::ComputeSortOrderInViewDirection(const double dop[3])
{
  std::vector<BoxT> boxes(this->Boxes.size());
  int rank = 0;
  for (auto& box : boxes)
  {
    box.self = this;
    box.rank = rank++;
  }

  vtkBlockSortHelper::BackToFront<BoxT> sortBoxes(
    vtkVector3d(0.0), vtkVector3d(dop), /*is_parallel = */ true);
  vtkBlockSortHelper::Sort(boxes.begin(), boxes.end(), sortBoxes);
  std::vector<int> indexes(boxes.size());
  std::transform(
    boxes.rbegin(), boxes.rend(), indexes.begin(), [](const BoxT& box) { return box.rank; });
  return indexes;
}

//------------------------------------------------------------------------------
std::vector<int> vtkOrderedCompositingHelper::ComputeSortOrderFromPosition(const double pos[3])
{
  std::vector<BoxT> boxes(this->Boxes.size());
  int rank = 0;
  for (auto& box : boxes)
  {
    box.self = this;
    box.rank = rank++;
  }

  vtkBlockSortHelper::BackToFront<BoxT> sortBoxes(
    vtkVector3d(pos), vtkVector3d(0.0), /*is_parallel = */ false);
  vtkBlockSortHelper::Sort(boxes.begin(), boxes.end(), sortBoxes);
  std::vector<int> indexes(boxes.size());
  std::transform(
    boxes.rbegin(), boxes.rend(), indexes.begin(), [](const BoxT& box) { return box.rank; });
  return indexes;
}

//----------------------------------------------------------------------------
void vtkOrderedCompositingHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
