/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalProbeFilter - Probes point attributes over time.
// .SECTION Description
// On each animate callback, this filter records the input point attribute 
// data. The output of this filter can then be plotted with a XYPlotActor
// to see how the values at that point changed over time.

#ifndef __vtkTemporalProbeFilter_h
#define __vtkTemporalProbeFilter_h

#include "vtkDataSetAlgorithm.h"

class VTK_PARALLEL_EXPORT vtkTemporalProbeFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkTemporalProbeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkTemporalProbeFilter *New();
 
  // Description:
  // Resets, and prepares to begin recording the input data in an animation.
  void AnimateInit();

  // Description:
  // Records the input data at this point in time.
  void AnimateTick(double TheTime);

protected:
  vtkTemporalProbeFilter();
  ~vtkTemporalProbeFilter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkUnstructuredGrid *History;

  //If empty, will simply pass input data through, otherwise will use recorded
  //data.
  bool Empty;
private:
  vtkTemporalProbeFilter(const vtkTemporalProbeFilter&);  // Not implemented.
  void operator=(const vtkTemporalProbeFilter&);  // Not implemented.
};

#endif
