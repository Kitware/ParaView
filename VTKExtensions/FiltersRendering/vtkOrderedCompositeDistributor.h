// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2005 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkOrderedCompositeDistributor
 *
 *
 *
 * This class distributes data for use with ordered compositing (i.e. with
 * IceT).  The composite distributor takes the same vtkPKdTree
 * that IceT and will use that to distribute the data.
 *
 * Input poly data will be converted back to poly data on the output.
 *
 * This class also has an optional pass through mode to make it easy to
 * turn ordered compositing on and off.
 *
 */

#ifndef vtkOrderedCompositeDistributor_h
#define vtkOrderedCompositeDistributor_h

#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

#include "vtkDataObjectAlgorithm.h"
#include "vtkNew.h" // needed for ivar

#include <vector> // for std::vector

class vtkBoundingBox;
class vtkMultiProcessController;
class vtkRedistributeDataSetFilter;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkOrderedCompositeDistributor
  : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkOrderedCompositeDistributor, vtkDataObjectAlgorithm);
  static vtkOrderedCompositeDistributor* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  void SetCuts(const std::vector<vtkBoundingBox>& boxes);
  void SetController(vtkMultiProcessController* controller);
  void SetBoundaryMode(int mode);
  ///@}

  // These are kept consistent with vtkRedistributeDataSetFilter::BoundaryModes
  enum BoundaryModes
  {
    ASSIGN_TO_ONE_REGION = 0,
    ASSIGN_TO_ALL_INTERSECTING_REGIONS = 1,
    SPLIT_BOUNDARY_CELLS = 2
  };

protected:
  vtkOrderedCompositeDistributor();
  ~vtkOrderedCompositeDistributor() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkOrderedCompositeDistributor(const vtkOrderedCompositeDistributor&) = delete;
  void operator=(const vtkOrderedCompositeDistributor&) = delete;

  vtkNew<vtkRedistributeDataSetFilter> RedistributeDataSetFilter;
};

#endif // vtkOrderedCompositeDistributor_h
