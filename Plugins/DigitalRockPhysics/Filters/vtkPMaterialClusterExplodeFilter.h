/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMaterialClusterExplodeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPMaterialClusterExplodeFilter
 * @brief   Create an exploded mesh of the material clusters removing the rockfill.
 *
 * This filter creates an exploded surface mesh of the clusters (set of connected
 * cells that have the same material attribute) described in the input image data.
 * User has to provide:
 *  - the point scalar data-array which contains the discrete material attributes.
 *  - the explode factor which is the factor by which the translation vector
 *    going from the grid center to the cluster center is multiplied.
 *  - the rockfill material label which corresponds to the material that will
 *    be removed from the output mesh.
 *
 * This filter requires an initial analysis of the input data with the
 * vtkPMaterialClusterAnalysisFilter. If input is not the output of this filter
 * (ie. it does not contain the metadata produced by this filter, the cluster
 * centers in particular), then the filter will be called internally first.
 *
 * Note that this filter has two levels of parallelization: it takes benefit of
 * data parallelism if it is enabled (eg. MPI), but it takes also benefit from
 * task parallelism using the SMP feature of VTK if enabled (OpenMP, TBB, etc.)
 * to perform faster.
 *
 * @sa vtkPMaterialClusterAnalysisFilter
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Mathieu Westphal, Kitware 2017
 * This work was supported by Total SA.
*/

#ifndef vtkPMaterialClusterExplodeFilter_h
#define vtkPMaterialClusterExplodeFilter_h

#include "vtkDigitalRocksFiltersModule.h"
#include "vtkPolyDataAlgorithm.h"

class vtkImageData;

class VTKDIGITALROCKSFILTERS_EXPORT vtkPMaterialClusterExplodeFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkPMaterialClusterExplodeFilter* New();
  vtkTypeMacro(vtkPMaterialClusterExplodeFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the explode factor.
   * This value determines how far the points will be moved.
   * The cluster points will be translated by this value times the vector
   * that goes from the dataset center to the cluster barycenter.
   * Default is 1.0.
   */
  vtkSetMacro(ExplodeFactor, double);
  vtkGetMacro(ExplodeFactor, double);
  //@}

  //@{
  /**
   * Set/Get the label of the rockfill material. This material will be
   * ignored in the process. Default is 0.
   */
  vtkSetMacro(RockfillLabel, int);
  vtkGetMacro(RockfillLabel, int);
  //@}

protected:
  vtkPMaterialClusterExplodeFilter();
  ~vtkPMaterialClusterExplodeFilter() override = default;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  static void InternalProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);

  double ExplodeFactor;
  int RockfillLabel;
  vtkImageData* CacheInput;
  vtkDataArray* CacheArray;
  vtkSmartPointer<vtkImageData> CacheAnalysis;
  vtkMTimeType CacheTime;

private:
  vtkPMaterialClusterExplodeFilter(const vtkPMaterialClusterExplodeFilter&) = delete;
  void operator=(const vtkPMaterialClusterExplodeFilter&) = delete;
};

#endif
