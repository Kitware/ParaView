/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingDriver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamingDriver - orchestrates progression of streamed pieces
// .SECTION Description

#ifndef __vtkStreamingDriver_h
#define __vtkStreamingDriver_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkCallbackCommand;
class vtkMapper;
class Internals;

class VTK_EXPORT vtkStreamingDriver : public vtkObject
{
public:
  vtkTypeMacro(vtkStreamingDriver,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkStreamingDriver *New();

  // Description:
  // Assign what window to automate streaming in.
  void SetRenderWindow(vtkRenderWindow *);

  // Description:
  // Control over the list of mappers that this renders in streaming fashion.
  void AddMapper(vtkMapper *);
  void RemoveMapper(vtkMapper *);
  void RemoveAllMappers();

  // Description:
  // For internal use, window events call back here.
  void RenderEvent();

//BTX
protected:
  vtkStreamingDriver();
  ~vtkStreamingDriver();

  Internals *Internal;

private:
  vtkStreamingDriver(const vtkStreamingDriver&);  // Not implemented.
  void operator=(const vtkStreamingDriver&);  // Not implemented.


//ETX
};

#endif
