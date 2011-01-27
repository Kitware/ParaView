/*=========================================================================

  Program:   ParaView
  Module:    vtkParallelStreamHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParallelStreamHelper - interface class to allow parallel
// synchronization
// .SECTION Description
// This interface does nothing in practice, but it allows the VTK level
// library to compile without linking to ParaView. That is, we provide here an
// API, that another class implements via making calls to parallel communication
// features of ParaView.

#ifndef __vtkParallelStreamHelper_h
#define __vtkParallelStreamHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkParallelStreamHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkParallelStreamHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkParallelStreamHelper *New();

  //Description:
  //A command that is called in parallel to make all processors agree on
  //the value of flag.
  virtual void Reduce(bool &vtkNotUsed(flag)) {};

//BTX
protected:
  vtkParallelStreamHelper();
  ~vtkParallelStreamHelper();

private:
  vtkParallelStreamHelper(const vtkParallelStreamHelper&);  // Not implemented.
  void operator=(const vtkParallelStreamHelper&);  // Not implemented.

//ETX
};

#endif
