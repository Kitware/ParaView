/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiProcessControllerHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiProcessControllerHelper
// .SECTION Description
// This is a temporary toolbox. As API matures, these methods will move into
// vtkMultiProcessController.

#ifndef __vtkMultiProcessControllerHelper_h
#define __vtkMultiProcessControllerHelper_h

#include "vtkObject.h"

class vtkMultiProcessController;
class vtkMultiProcessStream;

class VTK_EXPORT vtkMultiProcessControllerHelper : public vtkObject
{
public:
  static vtkMultiProcessControllerHelper* New();
  vtkTypeMacro(vtkMultiProcessControllerHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Reduce the stream to all processes calling the (*operation) for reduction.
  // The operation is assumed to be commutative.
  static int ReduceToAll(
    vtkMultiProcessController* controller,
    vtkMultiProcessStream& data, 
    void (*operation)(vtkMultiProcessStream& A, vtkMultiProcessStream& B),
    int tag);
  //ETX

//BTX
protected:
  vtkMultiProcessControllerHelper();
  ~vtkMultiProcessControllerHelper();

private:
  vtkMultiProcessControllerHelper(const vtkMultiProcessControllerHelper&); // Not implemented
  void operator=(const vtkMultiProcessControllerHelper&); // Not implemented
//ETX
};

#endif

