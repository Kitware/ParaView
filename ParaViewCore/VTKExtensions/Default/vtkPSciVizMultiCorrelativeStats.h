/*=========================================================================

  Program:   ParaView
  Module:    vtkPSciVizMultiCorrelativeStats.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPSciVizMultiCorrelativeStats
 * @brief   Fit a multivariate Gaussian to data and/or assess Mahalanobis distance of each datum
 * from the mean.
 *
 * This filter either computes a statistical model of
 * a dataset or takes such a model as its second input.
 * Then, the model (however it is obtained) may
 * optionally be used to assess the input dataset.
 *
 * This filter computes the covariance matrix for all
 * the arrays you select plus the mean of each array.
 * The model is thus a multivariate Gaussian distribution
 * with the mean vector and variances provided.
 * Data is assessed using this model by computing the
 * Mahalanobis distance for each input point.
 * This distance will always be positive.
 *
 * The learned model output format is rather dense and can be confusing,
 * so it is discussed here.
 * The first filter output is a multiblock dataset consisting of 2 tables:
 *
 * \li Raw covariance data.
 * \li Covariance matrix and its Cholesky decomposition.
 *
 * The raw covariance table has 3 meaningful columns: 2 titled "Column1"
 * and "Column2" whose entries generally refer to the N arrays you selected
 * when preparing the filter and 1 column titled "Entries" that contains
 * numeric values.
 * The first row will always contain the number of observations in the
 * statistical analysis.
 * The next N rows contain the mean for each of the N arrays you selected.
 * The remaining rows contain covariances of pairs of arrays.
 *
 * The second table (covariance matrix and Cholesky decomposition) contains
 * information derived from the raw covariance data of the first table.
 * The first N rows of the first column contain the name of one array you
 * selected for analysis.
 * These rows are followed by a single entry
 * labeled "Cholesky" for a total of N+1 rows.
 * The second column, Mean contains the mean of each variable in the first N
 * entries and the number of observations processed in the final (N+1) row.
 *
 * The remaining columns (there are N, one for each array)
 * contain 2 matrices in triangular format.
 * The upper right triangle contains the covariance matrix
 * (which is symmetric, so its lower triangle may be inferred).
 * The lower left triangle contains the Cholesky decomposition of the
 * covariance matrix (which is triangular, so its upper triangle is zero).
 * Because the diagonal must be stored for both matrices, an additional
 * row is required - hence the N+1 rows and the final entry of the column
 * named "Column".
*/

#ifndef vtkPSciVizMultiCorrelativeStats_h
#define vtkPSciVizMultiCorrelativeStats_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSciVizStatistics.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPSciVizMultiCorrelativeStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizMultiCorrelativeStats* New();
  vtkTypeMacro(vtkPSciVizMultiCorrelativeStats, vtkSciVizStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPSciVizMultiCorrelativeStats();
  ~vtkPSciVizMultiCorrelativeStats() override;

  int LearnAndDerive(vtkMultiBlockDataSet* model, vtkTable* inData) override;
  int AssessData(
    vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model) override;

private:
  vtkPSciVizMultiCorrelativeStats(const vtkPSciVizMultiCorrelativeStats&) = delete;
  void operator=(const vtkPSciVizMultiCorrelativeStats&) = delete;
};

#endif // vtkPSciVizMultiCorrelativeStats_h
