/*=========================================================================

  Program:   ParaView
  Module:    vtkPSciVizKMeans.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPSciVizKMeans
 * @brief   Find k cluster centers and/or assess the closest center and distance to it for each
 * datum.
 *
 * This filter either computes a statistical model of
 * a dataset or takes such a model as its second input.
 * Then, the model (however it is obtained) may
 * optionally be used to assess the input dataset.
 *
 * This filter iteratively computes the center of k clusters in a space
 * whose coordinates are specified by the arrays you select.
 * The clusters are chosen as local minima of the sum of square Euclidean
 * distances from each point to its nearest cluster center.
 * The model is then a set of cluster centers.
 * Data is assessed by assigning a cluster center and distance to the
 * cluster to each point in the input data set.
*/

#ifndef vtkPSciVizKMeans_h
#define vtkPSciVizKMeans_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSciVizStatistics.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPSciVizKMeans : public vtkSciVizStatistics
{
public:
  static vtkPSciVizKMeans* New();
  vtkTypeMacro(vtkPSciVizKMeans, vtkSciVizStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The number of cluster centers.
   * The initial centers will be chosen randomly.
   * In the future the filter will accept an input table of initial cluster positions.
   * The default value of \a K is 5.
   */
  vtkSetMacro(K, int);
  vtkGetMacro(K, int);
  //@}

  //@{
  /**
   * The maximum number of iterations to perform when converging on cluster centers.
   * The default value is 50 iterations.
   */
  vtkSetMacro(MaxNumIterations, int);
  vtkGetMacro(MaxNumIterations, int);
  //@}

  //@{
  /**
   * The relative tolerance on cluster centers that will cause early termination of the algorithm.
   * The default value is 0.01: a 1 percent change in cluster coordinates.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  //@}

protected:
  vtkPSciVizKMeans();
  ~vtkPSciVizKMeans() override;

  int LearnAndDerive(vtkMultiBlockDataSet* model, vtkTable* inData) override;
  int AssessData(
    vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model) override;

  int K;
  int MaxNumIterations;
  double Tolerance;

private:
  vtkPSciVizKMeans(const vtkPSciVizKMeans&) = delete;
  void operator=(const vtkPSciVizKMeans&) = delete;
};

#endif // vtkPSciVizKMeans_h
