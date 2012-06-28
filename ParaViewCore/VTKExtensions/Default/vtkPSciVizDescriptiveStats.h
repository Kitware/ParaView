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
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkPSciVizDescriptiveStats - Provide access to VTK descriptive statistics.
// .SECTION Description
// This filter provides access to the features of vtkDescriptiveStatistics.
// See VTK documentation for details
//
// .SECTION Thanks
// Thanks to David Thompson and Philippe Pebay from Sandia National Laboratories 
// for implementing this class.
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

  virtual int LearnAndDerive( vtkMultiBlockDataSet* model, vtkTable* inData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model );

  int SignedDeviations;

private:
  vtkPSciVizDescriptiveStats( const vtkPSciVizDescriptiveStats& ); // Not implemented.
  void operator = ( const vtkPSciVizDescriptiveStats& ); // Not implemented.
};

#endif // __vtkPSciVizDescriptiveStats_h
