/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVHyperTreeGridAnalysisDrivenRefinement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVHyperTreeGridAnalysisDrivenRefinement.h"

#include "vtkObjectFactory.h"

#include <limits>

vtkStandardNewMacro(vtkPVHyperTreeGridAnalysisDrivenRefinement);

//----------------------------------------------------------------------------
vtkPVHyperTreeGridAnalysisDrivenRefinement::vtkPVHyperTreeGridAnalysisDrivenRefinement()
{
  this->MaxCache = std::numeric_limits<double>::infinity();
  this->MinCache = -std::numeric_limits<double>::infinity();
}

//----------------------------------------------------------------------------
void vtkPVHyperTreeGridAnalysisDrivenRefinement::SetMaxState(bool state)
{
  if (!state)
  {
    this->MaxCache = this->Max;
    this->SetMaxToInfinity();
  }
  else
  {
    this->SetMax(std::min(this->MaxCache, this->Max));
  }
}

//----------------------------------------------------------------------------
void vtkPVHyperTreeGridAnalysisDrivenRefinement::SetMinState(bool state)
{
  if (!state)
  {
    this->MinCache = this->Min;
    this->SetMinToInfinity();
  }
  else
  {
    this->SetMin(std::max(this->MinCache, this->Min));
  }
}
