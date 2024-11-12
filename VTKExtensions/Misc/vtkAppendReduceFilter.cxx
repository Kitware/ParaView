// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendReduceFilter.h"

#include "vtkAppendFilter.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkAppendReduceFilter);
//----------------------------------------------------------------------------
vtkAppendReduceFilter::vtkAppendReduceFilter()
{
  vtkNew<vtkAppendFilter> appendFilter;
  this->SetPostGatherHelper(appendFilter);
}

//----------------------------------------------------------------------------
vtkAppendReduceFilter::~vtkAppendReduceFilter() = default;

//-----------------------------------------------------------------------------
void vtkAppendReduceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Merge Points: " << (this->MergePoints ? "On" : "Off") << std::endl;
  os << indent << "Tolerance: " << this->Tolerance << std::endl;
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkAppendReduceFilter::Reduce(vtkDataObject* input, vtkDataObject* output)
{
  vtkAppendFilter* appendFilter = vtkAppendFilter::SafeDownCast(this->GetPostGatherHelper());
  if (!appendFilter)
  {
    vtkErrorMacro("PostGatherHelper must be a vtkAppendFilter. Please do not change "
                  "PostGatherHelper when using vtkAppendReduceFilter."
      << "If you wish to use a custom PostGatherHelper, please use vtkReductionFilter instead.");
    return;
  }
  appendFilter->SetMergePoints(this->MergePoints);
  appendFilter->SetTolerance(this->Tolerance);
  this->Superclass::Reduce(input, output);
}
