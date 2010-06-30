/*=========================================================================

  Program:   ParaView
  Module:    vtkPSciVizDescriptiveStats.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPSciVizDescriptiveStats - Derive contingency tables and use them to assess the likelihood of associations.
// .SECTION Description
// This filter either computes a statistical model of
// a dataset or takes such a model as its second input.
// Then, the model (however it is obtained) may
// optionally be used to assess the input dataset.
//
// This filter computes the min, max, mean, raw moments M2 through M4,
// standard deviation, skewness, and kurtosis for each array you select.
//
// The model is simply a univariate Gaussian distribution with the mean
// and standard deviation provided. Data is assessed using this model by
// detrending the data (i.e., subtracting the mean) and then dividing by
// the standard deviation.
// Thus the assessment is an array whose entries are the number of standard
// deviations from the mean that each input point lies.

#ifndef __vtkPSciVizDescriptiveStats_h
#define __vtkPSciVizDescriptiveStats_h

#include "vtkSciVizStatistics.h"

class VTK_EXPORT vtkPSciVizDescriptiveStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizDescriptiveStats* New();
  vtkTypeMacro(vtkPSciVizDescriptiveStats,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkSetMacro(SignedDeviations,int);
  vtkGetMacro(SignedDeviations,int);

protected:
  vtkPSciVizDescriptiveStats();
  virtual ~vtkPSciVizDescriptiveStats();

  virtual int FitModel( vtkMultiBlockDataSet* model, vtkTable* trainingData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model );

  int SignedDeviations;

private:
  vtkPSciVizDescriptiveStats( const vtkPSciVizDescriptiveStats& ); // Not implemented.
  void operator = ( const vtkPSciVizDescriptiveStats& ); // Not implemented.
};

#endif // __vtkPSciVizDescriptiveStats_h
