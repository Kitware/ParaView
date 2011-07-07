/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQLineSource - create a line defined by two end points
// .SECTION Description
// vtkSQLineSource is a source object that creates a polyline defined by
// two endpoints. The number of segments composing the polyline is
// controlled by setting the object resolution.
//
// Modified to work with the dynamic load ballancing of the SQ Field Tracer.
//

#ifndef __vtkSQLineSource_h
#define __vtkSQLineSource_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkSQLineSource : public vtkPolyDataAlgorithm
{
public:
  static vtkSQLineSource *New();
  vtkTypeMacro(vtkSQLineSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set position of first end point.
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Set position of other end point.
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);

  // Description:
  // Divide line into resolution number of pieces.
  vtkSetClampMacro(Resolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Resolution,int);

protected:
  vtkSQLineSource(int res=1);
  ~vtkSQLineSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double Point1[3];
  double Point2[3];
  int Resolution;
private:
  vtkSQLineSource(const vtkSQLineSource&);  // Not implemented.
  void operator=(const vtkSQLineSource&);  // Not implemented.
};

#endif
