/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMaterialClusterAnalysisFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPMaterialClusterAnalysisFilter
 * @brief   Performs the analysis of the material cluster in an image.
 *
 * This filter produces a multiblock dataset that contains two blocks:
 *  i/ a table (block 2) that contains the volume (number of cells
 *    of every cluster) and barycenter of every cluster (set of connected cells
 *    that have the same point data material attribute) ;
 *  ii/ a copy of the input data image (block 1) with new point data arrays that
 *    correspond to the volume and barycenter of the material cluster it
 *    belongs to.
 *
 * Note that this filter has two levels of parallelization: it takes benefit of
 * data parallelism if it is enabled (eg. MPI), but it takes also benefit from
 * task parallelism using the SMP feature of VTK if enabled (OpenMP, TBB, etc.)
 * to perform faster.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Mathieu Westphal, Kitware 2017
 * This work was supported by Total SA.
*/

#ifndef vtkPMaterialClusterAnalysisFilter_h
#define vtkPMaterialClusterAnalysisFilter_h

#include "vtkDigitalRocksFiltersModule.h"
#include "vtkImageAlgorithm.h"

class VTKDIGITALROCKSFILTERS_EXPORT vtkPMaterialClusterAnalysisFilter : public vtkImageAlgorithm
{
public:
  static vtkPMaterialClusterAnalysisFilter* New();
  vtkTypeMacro(vtkPMaterialClusterAnalysisFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the label of the rockfill material. This material will be
   * ignored in the process. Default is 0.
   */
  vtkSetMacro(RockfillLabel, int);
  vtkGetMacro(RockfillLabel, int);
  //@}

protected:
  vtkPMaterialClusterAnalysisFilter();
  ~vtkPMaterialClusterAnalysisFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  int RockfillLabel;

private:
  vtkPMaterialClusterAnalysisFilter(const vtkPMaterialClusterAnalysisFilter&) = delete;
  void operator=(const vtkPMaterialClusterAnalysisFilter&) = delete;
};

#endif
