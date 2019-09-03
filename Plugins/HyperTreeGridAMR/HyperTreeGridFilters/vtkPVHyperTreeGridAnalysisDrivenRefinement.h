/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVHyperTreeGridAnalysisDrivenRefinement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPVHyperTreeGridAnalysisDrivenRefinement
 * @brief   Paraview interface to convert vtkDataSet to vtkHyperTreeGrid
 *
 * Given an input vtkDataSet and a subdivision strategy, given an input
 * scalar field, produces an output vtkHyperTreeGrid representing the input
 * vtkDataSet.
 *
 * This class is an interface for Paraview to use vtkHyperTreeGridAnalysisDrivenRefinement
 *
 */

#ifndef vtkPVHyperTreeGridAnalysisDrivenRefinement_h
#define vtkPVHyperTreeGridAnalysisDrivenRefinement_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkHyperTreeGridAnalysisDrivenRefinement.h"

class VTKFILTERSGENERAL_EXPORT vtkPVHyperTreeGridAnalysisDrivenRefinement
  : public vtkHyperTreeGridAnalysisDrivenRefinement
{
public:
  static vtkPVHyperTreeGridAnalysisDrivenRefinement* New();
  vtkTypeMacro(
    vtkPVHyperTreeGridAnalysisDrivenRefinement, vtkHyperTreeGridAnalysisDrivenRefinement);

  vtkPVHyperTreeGridAnalysisDrivenRefinement();
  virtual ~vtkPVHyperTreeGridAnalysisDrivenRefinement() override = default;

  void SetMaxState(bool state);
  void SetMinState(bool state);

  double MaxCache, MinCache;

private:
  vtkPVHyperTreeGridAnalysisDrivenRefinement(
    const vtkPVHyperTreeGridAnalysisDrivenRefinement&) = delete;
  void operator=(const vtkPVHyperTreeGridAnalysisDrivenRefinement&) = delete;
};

#endif
