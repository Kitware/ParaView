/*=========================================================================

  Program:   ParaView
  Module:    vtkAppendArcLength.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAppendArcLength - appends Arc length for input poly lines.
// .SECTION Description
// vtkAppendArcLength is used for filter such as plot-over-line. In such cases,
// we need to add an attribute array that is the arc_length over the length of
// the probed line. That's when vtkAppendArcLength can be used. It adds a new
// point-data array named "arc_length" with the computed arc length for each of
// the polylines in the input. For all other cell types, the arc length is set
// to 0.
// .SECTION Caveats
// This filter assumes that cells don't share points.

#ifndef __vtkAppendArcLength_h
#define __vtkAppendArcLength_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkAppendArcLength : public vtkPolyDataAlgorithm
{
public:
  static vtkAppendArcLength* New();
  vtkTypeMacro(vtkAppendArcLength, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkAppendArcLength();
  ~vtkAppendArcLength();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
private:
  vtkAppendArcLength(const vtkAppendArcLength&); // Not implemented
  void operator=(const vtkAppendArcLength&); // Not implemented
//ETX
};

#endif

