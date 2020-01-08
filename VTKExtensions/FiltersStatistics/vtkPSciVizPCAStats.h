/*=========================================================================

  Program:   ParaView
  Module:    vtkPSciVizPCAStats.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPSciVizPCAStats
 * @brief   Perform PCA on data and/or project data into a subspace defined by the PCA.
 *
 * This filter either computes a statistical model of
 * a dataset or takes such a model as its second input.
 * Then, the model (however it is obtained) may
 * optionally be used to assess the input dataset.
 *
 * This filter performs additional analysis above
 * and beyond the vtkPSciVizMultiCorrelativeStats filter.
 * It computes the eigenvalues and eigenvectors of the
 * covariance matrix from the multicorrelative filter.
 * Data is then assessed by projecting the original tuples
 * into a possibly lower-dimensional space.
 *
 * Since the PCA filter uses the multicorrelative filter's analysis,
 * it shares the same raw covariance table specified in the
 * multicorrelative documentation.
 * The second table in the multiblock dataset comprising the model output
 * is an expanded version of the multicorrelative version.
 *
 * As with the multicorrlative filter, the second model table contains the
 * mean values, the upper-triangular portion of the symmetric covariance
 * matrix, and the non-zero lower-triangular portion of the Cholesky
 * decomposition of the covariance matrix.
 * Below these entries are the eigenvalues of the covariance matrix (in the
 * column labeled "Mean") and the eigenvectors (as row vectors) in an
 * additional NxN matrix.
*/

#ifndef vtkPSciVizPCAStats_h
#define vtkPSciVizPCAStats_h

#include "vtkPVVTKExtensionsFiltersStatisticsModule.h" //needed for exports
#include "vtkSciVizStatistics.h"

class VTKPVVTKEXTENSIONSFILTERSSTATISTICS_EXPORT vtkPSciVizPCAStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizPCAStats* New();
  vtkTypeMacro(vtkPSciVizPCAStats, vtkSciVizStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(NormalizationScheme, int);
  vtkGetMacro(NormalizationScheme, int);

  vtkSetMacro(BasisScheme, int);
  vtkGetMacro(BasisScheme, int);

  vtkSetMacro(FixedBasisSize, int);
  vtkGetMacro(FixedBasisSize, int);

  vtkSetClampMacro(FixedBasisEnergy, double, 0., 1.);
  vtkGetMacro(FixedBasisEnergy, double);

  vtkSetMacro(RobustPCA, bool);
  vtkGetMacro(RobustPCA, bool);
  vtkBooleanMacro(RobustPCA, bool);

protected:
  vtkPSciVizPCAStats();
  ~vtkPSciVizPCAStats() override;

  int LearnAndDerive(vtkMultiBlockDataSet* model, vtkTable* inData) override;
  int AssessData(
    vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model) override;

  int NormalizationScheme;
  int BasisScheme;
  int FixedBasisSize;
  double FixedBasisEnergy;
  bool RobustPCA;

private:
  vtkPSciVizPCAStats(const vtkPSciVizPCAStats&) = delete;
  void operator=(const vtkPSciVizPCAStats&) = delete;
};

#endif // vtkPSciVizPCAStats_h
