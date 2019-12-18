/*=========================================================================

  Program:   ParaView
  Module:    vtkTimeStepProgressFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTimeStepProgressFilter
 *
 * This filter can be attached to any filter/source/reader that supports time.
 * vtkTimeStepProgressFilter will generate a 1x1 vtkTable with
 * a progress rate between 0 and 1 that correspond to the actual time step/value
 * relatively to the number of timesteps/data time range.
*/

#ifndef vtkTimeStepProgressFilter_h
#define vtkTimeStepProgressFilter_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkTimeStepProgressFilter : public vtkTableAlgorithm
{
public:
  static vtkTimeStepProgressFilter* New();
  vtkTypeMacro(vtkTimeStepProgressFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTimeStepProgressFilter();
  ~vtkTimeStepProgressFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  double TimeRange[2];
  double* TimeSteps;
  int NumTimeSteps;
  bool UseTimeRange;

private:
  vtkTimeStepProgressFilter(const vtkTimeStepProgressFilter&) = delete;
  void operator=(const vtkTimeStepProgressFilter&) = delete;
};

#endif
