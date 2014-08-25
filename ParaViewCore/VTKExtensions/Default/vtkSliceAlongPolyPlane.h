/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceAlongPolyPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliceAlongPolyPlane - slice a dataset along a polyplane
// .SECTION Description

// vtkSliceAlongPolyPlane is a filter that slices its first input dataset
// along the surface defined by sliding the poly line in the second input
// dataset along a line parallel to the Z-axis.

// .SECTION See Also
// vtkCutter vtkPolyPlane

#ifndef __vtkSliceAlongPolyPlane_h
#define __vtkSliceAlongPolyPlane_h

#include "vtkPVVTKExtensionsDefaultModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkSliceAlongPolyPlane : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSliceAlongPolyPlane,vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkSliceAlongPolyPlane* New();

  // Description
  vtkSetMacro(Tolerance, double)
  vtkGetMacro(Tolerance, double)

protected:
  vtkSliceAlongPolyPlane();
  virtual ~vtkSliceAlongPolyPlane();

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int FillInputPortInformation( int port, vtkInformation* info);

private:
  vtkSliceAlongPolyPlane(const vtkSliceAlongPolyPlane&); // Not implemented
  void operator=(const vtkSliceAlongPolyPlane&); // Not implemented

  double Tolerance;

};

#endif
