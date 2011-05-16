/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderLoopEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderLoopEvent -
// .SECTION Description
// vtkPVRenderLoopEvent

#ifndef __vtkPVRenderLoopEvent_h
#define __vtkPVRenderLoopEvent_h
// --------------------------------------------------------------------includes
#include "vtkObject.h"

// -----------------------------------------------------------------pre-defines
class vtkPVRenderLoopEventInternal;

// -----------------------------------------------------------------------class
class VTK_COMMON_EXPORT vtkPVRenderLoopEvent : public vtkObject
{
public:
  // ............................................................public-methods
  static vtkPVRenderLoopEvent* New();
  vtkTypeMacro(vtkPVRenderLoopEvent, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static void Handle();
protected:
  // ...........................................................protected-ivars

protected:
//BTX
  // .......................................................................BTX
  vtkPVRenderLoopEvent();
  ~vtkPVRenderLoopEvent();

private:
  vtkPVRenderLoopEventInternal *Internal;
  vtkPVRenderLoopEvent(const vtkPVRenderLoopEvent&); // Not implemented.
  void operator=(const vtkPVRenderLoopEvent&); // Not implemented.
//ETX
  // .......................................................................ETX


};

#endif // __vtkPVRenderLoopEvent_h
