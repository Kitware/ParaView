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
// On each animate callback, this filter records the input's first point
// attribute data and the current time. This filter is intended to take
// in the output of the vtkPProbeFilter and to feed the XYPlotActor, in order
// to show the transient behavior of a element over time.
// See also vtkTemporalPickFilter.h

#ifndef __vtkTemporalProbeFilter_h
#define __vtkTemporalProbeFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkTemporalProbeFilter : public vtkDataSetAlgorithm
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

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkTemporalProbeFilter();
  ~vtkTemporalProbeFilter();

  virtual int FillOutputPortInformation(int, vtkInformation *);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkPolyData *History;

  //Controls which data RequestData passes through.
  bool Empty;

  vtkMultiProcessController* Controller;

private:
  vtkTemporalProbeFilter(const vtkTemporalProbeFilter&);  // Not implemented.
  void operator=(const vtkTemporalProbeFilter&);  // Not implemented.
};

#endif
