/*=========================================================================

  Program:   ParaView
  Module:    vtkPSciVizContingencyStats.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPSciVizContingencyStats - Derive contingency tables and use them to assess the likelihood of associations.
// .SECTION Description
// This filter either computes a statistical model of
// a dataset or takes such a model as its second input.
// Then, the model (however it is obtained) may
// optionally be used to assess the input dataset.
//
// This filter computes contingency tables between pairs of attributes.
// This result is a tabular bivariate probability distribution
// which serves as a Bayesian-style prior model.
// Data is assessed by computing
// <ul>
// <li> the probability of observing both variables simultaneously;
// <li> the probability of each variable conditioned on the other
//      (the two values need not be identical); and
// <li> the pointwise mutual information (PMI).
// </ul>
// Finally, the summary statistics include the information entropy
// of the observations.

#ifndef __vtkPSciVizContingencyStats_h
#define __vtkPSciVizContingencyStats_h

#include "vtkSciVizStatistics.h"

class VTK_EXPORT vtkPSciVizContingencyStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizContingencyStats* New();
  vtkTypeMacro(vtkPSciVizContingencyStats,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

protected:
  vtkPSciVizContingencyStats();
  virtual ~vtkPSciVizContingencyStats();

  virtual int LearnAndDerive( vtkMultiBlockDataSet* model, vtkTable* inData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model );

private:
  vtkPSciVizContingencyStats( const vtkPSciVizContingencyStats& ); // Not implemented.
  void operator = ( const vtkPSciVizContingencyStats& ); // Not implemented.
};

#endif // __vtkPSciVizContingencyStats_h
