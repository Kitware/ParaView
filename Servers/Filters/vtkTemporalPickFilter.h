/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalPickFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalPickFilter - Picks point attributes over time.
// .SECTION Description
// On each animate callback, this filter records the input's first point or 
// cell attribute data and the current time. This filter is intended to take
// in the output of the vtkPickFilter and to feed the XYPlotActor, in order
// to show the transient behavior of a element over time.
// See also vtkTemporalProbeFilter.h

#ifndef __vtkTemporalPickFilter_h
#define __vtkTemporalPickFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkTemporalPickFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkTemporalPickFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkTemporalPickFilter *New();
 
  // Description:
  // Resets, and prepares to begin recording the input data in an animation.
  void AnimateInit();

  // Description:
  // Records the input data at this point in time.
  void AnimateTick(double TheTime);

    // Description:
  // Select whether you are probing point(0) or cell(1) data.
  // The default value of this flag is off (points).
  vtkSetMacro(PointOrCell,int);
  vtkGetMacro(PointOrCell,int);
  vtkBooleanMacro(PointOrCell,int);

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkTemporalPickFilter();
  ~vtkTemporalPickFilter();

  virtual int FillOutputPortInformation(int, vtkInformation *);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkUnstructuredGrid *History;

  //Controls which data RequestData passes through.
  bool Empty;

  int PointOrCell;

  vtkMultiProcessController* Controller;

  int HasAllData;
private:
  vtkTemporalPickFilter(const vtkTemporalPickFilter&);  // Not implemented.
  void operator=(const vtkTemporalPickFilter&);  // Not implemented.
};

#endif
