/*=========================================================================

  Program:   ParaView
  Module:    vtkSMWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMWriterProxy - proxy for a VTK writer on a server
// .SECTION Description
// vtkSMWriterProxy manages VTK writers that are created on a server using
// the proxy pattern.

#ifndef __vtkSMWriterProxy_h
#define __vtkSMWriterProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMWriterProxy : public vtkSMProxy
{
public:
  static vtkSMWriterProxy* New();
  vtkTypeRevisionMacro(vtkSMWriterProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects filters/sinks to an input. If the filter(s) is not
  // created, this will create it. If hasMultipleInputs is
  // true,  only one filter is created, even if the input has
  // multiple parts. All the inputs are added using the method
  // name provided. If hasMultipleInputs is not true, one filter
  // is created for each input. NOTE: The filter(s) is created
  // when SetInput is called the first and if the it wasn't already
  // created. If the filter has two inputs and one is multi-block
  // whereas the other one is not, SetInput() should be called with
  // the multi-block input first. Otherwise, it will create only
  // on filter and can not apply to the multi-block input.
  void AddInput(vtkSMSourceProxy* input, 
                const char* method,
                int portIdx,
                int hasMultipleInputs);

  // Description:
  // Push the properties on to the vtk objects.
  virtual void UpdateVTKObjects();

protected:
  vtkSMWriterProxy() {}
  ~vtkSMWriterProxy() {}

private:
  vtkSMWriterProxy(const vtkSMWriterProxy&); // Not implemented
  void operator=(const vtkSMWriterProxy&); // Not implemented
};

#endif
