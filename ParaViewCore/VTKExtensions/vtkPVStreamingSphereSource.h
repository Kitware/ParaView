/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVStreamingSphereSource
// .SECTION Description

#ifndef __vtkPVStreamingSphereSource_h
#define __vtkPVStreamingSphereSource_h

#include "vtkSphereSource.h"

class VTK_EXPORT vtkPVStreamingSphereSource : public vtkSphereSource
{
public:
  static vtkPVStreamingSphereSource* New();
  vtkTypeMacro(vtkPVStreamingSphereSource, vtkSphereSource);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVStreamingSphereSource();
  ~vtkPVStreamingSphereSource();

  int RequestInformation(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

private:
  vtkPVStreamingSphereSource(const vtkPVStreamingSphereSource&); // Not implemented.
  void operator=(const vtkPVStreamingSphereSource&); // Not implemented.
//ETX
};

#endif
