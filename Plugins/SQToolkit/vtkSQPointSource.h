/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQPointSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQPointSource - create a random cloud of points
// .SECTION Description
// vtkSQPointSource is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// By default location of the points is random within the sphere. It is
// also possible to generate random points only on the surface of the
// sphere.
// Modified so that downstream work can be parallelized on cell id.

#ifndef __vtkSQPointSource_h
#define __vtkSQPointSource_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkSQPointSource : public vtkPolyDataAlgorithm
{
public:
  static vtkSQPointSource *New();
  vtkTypeMacro(vtkSQPointSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,vtkIdType,1,VTK_LARGE_ID);
  vtkGetMacro(NumberOfPoints,vtkIdType);

  // Description:
  // Set the center of the point cloud.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set the radius of the point cloud.  If you are
  // generating a Gaussian distribution, then this is
  // the standard deviation for each of x, y, and z.
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);


protected:
  vtkSQPointSource();
  ~vtkSQPointSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkIdType NumberOfPoints;
  double Center[3];
  double Radius;

private:
  vtkSQPointSource(const vtkSQPointSource&);  // Not implemented.
  void operator=(const vtkSQPointSource&);  // Not implemented.
};

#endif
