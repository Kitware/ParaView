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
/**
 * @class   vtkPSciVizDescriptiveStats
 * @brief   Provide access to VTK descriptive statistics.
 *
 * This filter provides access to the features of vtkDescriptiveStatistics.
 * See VTK documentation for details
 *
 * @par Thanks:
 * Thanks to David Thompson and Philippe Pebay from Sandia National Laboratories
 * for implementing this class.
*/

#ifndef vtkPSciVizDescriptiveStats_h
#define vtkPSciVizDescriptiveStats_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSciVizStatistics.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPSciVizDescriptiveStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizDescriptiveStats* New();
  vtkTypeMacro(vtkPSciVizDescriptiveStats, vtkSciVizStatistics);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetMacro(SignedDeviations, int);
  vtkGetMacro(SignedDeviations, int);

protected:
  vtkPSciVizDescriptiveStats();
  virtual ~vtkPSciVizDescriptiveStats();

  virtual int LearnAndDerive(vtkMultiBlockDataSet* model, vtkTable* inData) VTK_OVERRIDE;
  virtual int AssessData(
    vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model) VTK_OVERRIDE;

  int SignedDeviations;

private:
  vtkPSciVizDescriptiveStats(const vtkPSciVizDescriptiveStats&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPSciVizDescriptiveStats&) VTK_DELETE_FUNCTION;
};

#endif // vtkPSciVizDescriptiveStats_h
