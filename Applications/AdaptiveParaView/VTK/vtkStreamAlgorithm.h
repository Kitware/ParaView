/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamAlgorithm.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamAlgorithm - calculates progression of streamed pieces
// .SECTION Description

#ifndef __vtkStreamAlgorithm_h
#define __vtkStreamAlgorithm_h

#include "vtkObject.h"

class Internals;

class VTK_EXPORT vtkStreamAlgorithm : public vtkObject
{
public:
  vtkTypeMacro(vtkStreamAlgorithm,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkStreamAlgorithm *New();

//BTX
protected:
  vtkStreamAlgorithm();
  ~vtkStreamAlgorithm();

  Internals *Internal;

private:
  vtkStreamAlgorithm(const vtkStreamAlgorithm&);  // Not implemented.
  void operator=(const vtkStreamAlgorithm&);  // Not implemented.

//ETX
};

#endif
